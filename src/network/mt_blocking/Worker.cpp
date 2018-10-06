#include "Worker.h"

#include <cassert>
#include <functional>
#include <iostream>

#include <sys/socket.h>
#include <sys/types.h>

#include <spdlog/logger.h>

#include <afina/logging/Service.h>

namespace Afina {
namespace Network {
namespace MTblocking {

Worker::Worker(std::shared_ptr<Afina::Storage> ps, std::shared_ptr<Afina::Logging::Service> pl, std::vector<std::unique_ptr<Worker>> *workers, std::mutex *workers_mutex, std::condition_variable *serv_lock)
    : _pStorage(ps), _pLogging(pl), isRunning(false), _worker_id(-1), _workers(workers), _workers_mutex(workers_mutex), _serv_lock(serv_lock) {}

Worker::~Worker() {}

Worker::Worker(Worker &&other) { *this = std::move(other); }

Worker &Worker::operator=(Worker &&other) {
    _pStorage = std::move(other._pStorage);
    _pLogging = std::move(other._pLogging);
    _logger = std::move(other._logger);
    _thread = std::move(other._thread);
    _worker_id = other._worker_id;

    other._worker_id = -1;
    return *this;
}

void Worker::Start(int worker_id, int client_socket, struct sockaddr& client_addr) {
    if (isRunning.exchange(true) == false) {
        assert(_worker_id == -1);
        _worker_id = worker_id;
        _logger = _pLogging->select("network.worker");
        _client_socket = client_socket;
        _client_addr = client_addr;
        _client_addr_len = sizeof(_client_addr);
        
        _thread = std::thread(&Worker::OnRun, this);
    }
}

void Worker::Stop() {
    auto it = find_if(_workers->begin(), _workers->end(), [&](unique_ptr<Worker>& obj){return obj->id() == _worker_id;});
    assert(it != _workers->end()); // if not, we are at the *woops* situation
    { // erasing myself
        std::unique_lock<std::mutex> lc(*_workers_mutex);
        _workers->erase(it);
    }
    _serv_lock->notify_all();
    isRunning.exchange(false);
}

void Worker::Join() {
    assert(_thread.joinable());
    _thread.join();
}

void Worker::OnRun() {
    _logger->trace("OnRun");
    assert(_worker_id >= 0);
    std::size_t arg_remains;
    Protocol::Parser parser;
    std::string argument_for_command;
    std::unique_ptr<Execute::Command> command_to_execute;
    if (isRunning.load()) { // copy-paste from here onwards
        _logger->debug("Processing new connection");

        // Process new connection:
        // - read commands until socket alive
        // - execute each command
        // - send response
        try {
            int readed_bytes = -1;
            char client_buffer[4096];
            while ((readed_bytes = read(client_socket, client_buffer, sizeof(client_buffer))) > 0) {
                _logger->debug("Got {} bytes from socket", readed_bytes);

                // Single block of data readed from the socket could trigger inside actions a multiple times,
                // for example:
                // - read#0: [<command1 start>]
                // - read#1: [<command1 end> <argument> <command2> <argument for command 2> <command3> ... ]
                while (readed_bytes > 0) {
                    _logger->debug("Process {} bytes", readed_bytes);
                    // There is no command yet
                    if (!command_to_execute) {
                        std::size_t parsed = 0;
                        if (parser.Parse(client_buffer, readed_bytes, parsed)) {
                            // There is no command to be launched, continue to parse input stream
                            // Here we are, current chunk finished some command, process it
                            _logger->debug("Found new command: {} in {} bytes", parser.Name(), parsed);
                            command_to_execute = parser.Build(arg_remains);
                            if (arg_remains > 0) {
                                arg_remains += 2;
                            }
                        }

                        // Parsed might fails to consume any bytes from input stream. In real life that could happens,
                        // for example, because we are working with UTF-16 chars and only 1 byte left in stream
                        if (parsed == 0) {
                            break;
                        } else {
                            std::memmove(client_buffer, client_buffer + parsed, readed_bytes - parsed);
                            readed_bytes -= parsed;
                        }
                    }

                    // There is command, but we still wait for argument to arrive...
                    if (command_to_execute && arg_remains > 0) {
                        _logger->debug("Fill argument: {} bytes of {}", readed_bytes, arg_remains);
                        // There is some parsed command, and now we are reading argument
                        std::size_t to_read = std::min(arg_remains, std::size_t(readed_bytes));
                        argument_for_command.append(client_buffer, to_read);

                        std::memmove(client_buffer, client_buffer + to_read, readed_bytes - to_read);
                        arg_remains -= to_read;
                        readed_bytes -= to_read;
                    }

                    // Thre is command & argument - RUN!
                    if (command_to_execute && arg_remains == 0) {
                        _logger->debug("Start command execution");

                        std::string result;
                        command_to_execute->Execute(*pStorage, argument_for_command, result);

                        // Send response
                        if (send(client_socket, result.data(), result.size(), 0) <= 0) {
                            throw std::runtime_error("Failed to send response");
                        }

                        // Prepare for the next command
                        command_to_execute.reset();
                        argument_for_command.resize(0);
                        parser.Reset();
                    }
                } // while (readed_bytes)
            }

            if (readed_bytes == 0) {
                _logger->debug("Connection closed");
            } else {
                throw std::runtime_error(std::string(strerror(errno)));
            }
        } catch (std::runtime_error &ex) {
            _logger->error("Failed to process connection on descriptor {}: {}", client_socket, ex.what());
        }

        // We are done with this connection
        close(client_socket);
    }
    
    Stop();
    
    _logger->warn("Worker stopped");
}

} // namespace MTblocking
} // namespace Network
} // namespace Afina
