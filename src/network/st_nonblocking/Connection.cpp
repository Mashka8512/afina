#include "Connection.h"

#include <cassert>
#include <cstring>
#include <iostream>
#include <memory>
#include <stdexcept>

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
void Connection::Start(std::shared_ptr<Afina::Storage> ps) {
    state = State::Alive;
    pStorate = ps;
    command_to_execute.reset();
    argument_for_command.resize(0);
    parser.Reset();
    results_to_write.clear();
}

// See Connection.h
void Connection::OnError() {
    this->state = State::Dead;
    results_to_write.clear();
}

// See Connection.h
void Connection::OnClose() {
    this->state = State::Dead;
    results_to_write.clear();
}

// See Connection.h
void Connection::DoRead() {
    try {
        int read_bytes = -1;
        char client_buffer[4096];
        while ((read_bytes = read(client_socket, client_buffer, sizeof(client_buffer))) > 0) {
            // Single block of data read from the socket could trigger inside actions a multiple times,
            // for example:
            // - read#0: [<command1 start>]
            // - read#1: [<command1 end> <argument> <command2> <argument for command 2> <command3> ... ]
            while (read_bytes > 0) {
                // There is no command yet
                if (!command_to_execute) {
                    std::size_t parsed = 0;
                    if (parser.Parse(client_buffer, read_bytes, parsed)) {
                        // There is no command to be launched, continue to parse input stream
                        // Here we are, current chunk finished some command, process it
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
                        std::memmove(client_buffer, client_buffer + parsed, read_bytes - parsed);
                        read_bytes -= parsed;
                    }
                }

                // There is command, but we still wait for argument to arrive...
                if (command_to_execute && arg_remains > 0) {
                    // There is some parsed command, and now we are reading argument
                    std::size_t to_read = std::min(arg_remains, std::size_t(read_bytes));
                    argument_for_command.append(client_buffer, to_read);

                    std::memmove(client_buffer, client_buffer + to_read, read_bytes - to_read);
                    arg_remains -= to_read;
                    read_bytes -= to_read;
                }

                // There is command & argument - RUN!
                if (command_to_execute && arg_remains == 0) {
                    std::string result_to_write;
                    command_to_execute->Execute(*pStorage, argument_for_command, result_to_write);
                    result_to_write += "\r\n";
                    // Save results for better time
                    results_to_write.push_back(result_to_write);

                    // Prepare for the next command
                    command_to_execute.reset();
                    argument_for_command.resize(0);
                    parser.Reset();

                    _event.events = EPOLLOUT;
                }
            } // while (read_bytes)
        }
        if (read_bytes > 0) {
            throw std::runtime_error(std::string(strerror(errno)));
        }
    }
    catch (std::runtime_error &ex) {

    }
}

// See Connection.h
void Connection::DoWrite() {
    while (!results_to_write.empty()) {
        auto result = results_to_write.back();
        if (send(_socket, result.data(), result.size(), 0) <= 0) {
            throw std::runtime_error("Failed to send response");
        }
        results_to_write.pop_back();
    }

    _event.events = EPOLLIN;
}

} // namespace STnonblock
} // namespace Network
} // namespace Afina
