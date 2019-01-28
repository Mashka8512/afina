#include "Connection.h"

#include <cassert>
#include <cstring>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <sys/uio.h>

#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <pthread.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <spdlog/logger.h>

#include <afina/Storage.h>
#include <afina/execute/Command.h>
#include <afina/logging/Service.h>

#include "protocol/Parser.h"

namespace Afina {
namespace Network {
namespace STnonblock {

// See Connection.h
void Connection::Start() {
    _logger = pLogging->select("network.connection");
    _logger->info("Connection starts");
    state = State::Alive;
    command_to_execute.reset();
    argument_for_command.resize(0);
    parser.Reset();
    results_to_write.clear();
    write_position = 0;
    _event.events = Masks::read;
    client_buffer.reset();
}

// See Connection.h
void Connection::OnError() {
    _logger->info("Connection error");
    this->state = State::Dead;
    results_to_write.clear();
}

// See Connection.h
void Connection::OnClose() {
    _logger->info("Connection closing");
    this->state = State::Dead;
    results_to_write.clear();
}

// See Connection.h
void Connection::DoRead() {
    _logger->info("Connection reading");
    try {
        int read_bytes = -1;
        while (read_bytes = read(_socket, client_buffer.read_ptr(), client_buffer.read_size()) > 0) {
            client_buffer.read(read_bytes);
            while (client_buffer.parse_size() > 0) {
                std::size_t parsed = 0;
                if (!command_to_execute) {
                    if (parser.Parse(client_buffer.parse_ptr(), client_buffer.parse_size(), parsed)) {
                        command_to_execute = parser.Build(arg_remains);
                        if (arg_remains > 0) {
                            arg_remains += 2;
                        }
                    }
                    if (parsed == 0) {
                        break;
                    } else {
                        client_buffer.parsed(parsed);
                    }
                }
                if (command_to_execute && arg_remains > 0) {
                    std::size_t to_read = std::min(arg_remains, std::size_t(client_buffer.parse_size()));
                    argument_for_command.append(client_buffer.parse_ptr(), to_read);

                    arg_remains -= to_read;
                    client_buffer.parsed(to_read);
                }

                if (command_to_execute && arg_remains == 0) {
                    std::string result_to_write;
                    command_to_execute->Execute(*_storage, argument_for_command, result_to_write);
                    result_to_write += "\r\n";
                    results_to_write.push_back(result_to_write);
                    _logger->debug("Pushed back");
                    _logger->debug(result_to_write);

                    command_to_execute.reset();
                    argument_for_command.resize(0);
                    parser.Reset();

                    _event.events = Masks::read_write;
                }
            }
            client_buffer.conditional_reset();
        }
        if (read_bytes > 0) {
            throw std::runtime_error(std::string(strerror(errno)));
        }
    }
    catch (std::runtime_error &ex) {
        _logger->error("Failed to process connection on descriptor {}: {}", _socket, ex.what());
    }
}

// See Connection.h
void Connection::DoWrite() {
    _logger->info("Connection writing");
    struct iovec iovecs[results_to_write.size()];
    _logger->info(results_to_write.size());
    for (int i = 0; i < results_to_write.size(); i++) {
        iovecs[i].iov_len = results_to_write[i].size();
        iovecs[i].iov_base = &(results_to_write[i][0]);
    }
    iovecs[0].iov_base = static_cast<char*>(iovecs[0].iov_base) + write_position;
    iovecs[0].iov_len -= write_position;

    int written;
    if ((written = writev(_socket, iovecs, client_buffer.size())) <= 0) {
        _logger->error("Failed to send response");
        _logger->info(written);
    }
    write_position += written;

    int i = 0;
    for (; i < results_to_write.size() && (write_position - iovecs[i].iov_len) >= 0; i++) {
        write_position -= iovecs[i].iov_len;
    }

    results_to_write.erase(results_to_write.begin(), results_to_write.begin() + i);
    if (results_to_write.empty()) {
        _event.events = Masks::read;
    }
    else {
        _event.events = Masks::read_write;
    }
}

} // namespace STnonblock
} // namespace Network
} // namespace Afina
