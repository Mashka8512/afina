#ifndef AFINA_NETWORK_ST_NONBLOCKING_CONNECTION_H
#define AFINA_NETWORK_ST_NONBLOCKING_CONNECTION_H

#include <cstring>

#include <sys/epoll.h>

namespace Afina {
namespace Network {
namespace STnonblock {

class Connection {
public:
    Connection(int s, std::shared_ptr<Afina::Storage> ps) : _socket(s), pStorage(ps) {
        std::memset(&_event, 0, sizeof(struct epoll_event));
        _isAlive = true;
    }
    inline bool isAlive() const { return _isAlive; }

    void Start(std::shared_ptr<spdlog::logger> logger);

protected:
    void OnError();
    void OnClose();
    void DoRead();
    void DoWrite();

private:
    friend class Worker;
    friend class ServerImpl;

    static const int mask_read = EPOLLIN | EPOLLRDHUP | EPOLLERR;
    static const int mask_read_write = EPOLLIN | EPOLLRDHUP | EPOLLERR | EPOLLOUT;
    
    std::shared_ptr<spdlog::logger> _logger;
    std::shared_ptr<Afina::Storage> pStorage;

    std::mutex _mutex;
    std::atomic<bool> _DoRead;

    int _socket;
    struct epoll_event _event;
    std::atomic<bool> _isAlive;


    std::size_t arg_remains;
    Protocol::Parser parser;
    std::string argument_for_command;
    std::unique_ptr<Execute::Command> command_to_execute;
    
    int readed_bytes = 0;
    char client_buffer[4096];
    
    std::vector<std::string> _answers;
    int _position = 0;
};

} // namespace STnonblock
} // namespace Network
} // namespace Afina

#endif // AFINA_NETWORK_ST_NONBLOCKING_CONNECTION_H
