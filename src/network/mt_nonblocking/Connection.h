#ifndef AFINA_NETWORK_MT_NONBLOCKING_CONNECTION_H
#define AFINA_NETWORK_MT_NONBLOCKING_CONNECTION_H

#include <cstring>
#include <afina/Storage.h>
#include <afina/execute/Command.h>
#include <afina/logging/Service.h>

#include "protocol/Parser.h"
#include "ClientBuffer.h"
#include "Worker.h"

#include <sys/epoll.h>

namespace Afina {
namespace Network {
namespace MTnonblock {

class Connection {
public:
    enum class State {
        Embryo,
        Alive,
        Dead
    };

    struct Masks {
        static const int read = ((EPOLLIN | EPOLLRDHUP) | EPOLLERR);
        static const int read_write = (((EPOLLIN | EPOLLRDHUP) | EPOLLERR) | EPOLLOUT);
    };

    Connection(int s, std::shared_ptr<Afina::Storage> ps, std::shared_ptr<Logging::Service> pl) :
                _socket(s),
                _storage(ps),
                pLogging(pl) {
        std::memset(&_event, 0, sizeof(struct epoll_event));
        _event.data.ptr = this;
        client_buffer = ClientBuffer();
    }

    inline bool isAlive() const {
        if (state == State::Alive) {
            return true;
        }
        return false;
    }

    void Start();

protected:
    void OnError();
    void OnClose();
    void DoRead();
    void DoWrite();

private:

    friend class ServerImpl;
    friend class Worker;

    int _socket;
    struct epoll_event _event;
    State state = State::Embryo;
    std::size_t arg_remains;
    Protocol::Parser parser;
    std::string argument_for_command;
    std::unique_ptr<Execute::Command> command_to_execute;
    std::shared_ptr<Afina::Storage> _storage;
    std::shared_ptr<Logging::Service> pLogging;
    std::shared_ptr<spdlog::logger> _logger;
    std::vector<std::string> results_to_write;
    ClientBuffer client_buffer;
    std::size_t write_position = 0;
    std::mutex lock;
};

} // namespace MTnonblock
} // namespace Network
} // namespace Afina

#endif // AFINA_NETWORK_MT_NONBLOCKING_CONNECTION_H
