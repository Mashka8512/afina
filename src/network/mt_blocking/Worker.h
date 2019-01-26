#ifndef AFINA_NETWORK_MTBLOCKING_WORKER_H
#define AFINA_NETWORK_MTBLOCKING_WORKER_H

#include <sys/socket.h>

#include <atomic>
#include <memory>
#include <thread>
#include <mutex>
#include <condition_variable>

#include <cassert>
#include <cstring>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <algorithm>

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

namespace spdlog {
class logger;
}

namespace Afina {

// Forward declaration, see afina/Storage.h
class Storage;
namespace Logging {
class Service;
}

namespace Network {
namespace MTblocking {

class Worker {
public:
    Worker(Afina::Storage *ps);
    ~Worker();

    Worker(Worker &&);
    Worker &operator=(Worker &&);

    void Start(int worker_id, int client_socket, struct sockaddr& client_addr);

    void Stop();

    void Detach();

    int id() {
        return _worker_id;
    }

protected:
    void OnRun();

private:
    Worker(Worker &) = delete;
    Worker &operator=(Worker &) = delete;

    Afina::Storage *_pStorage;

    std::atomic<bool> isRunning;

    std::thread _thread;

    int _worker_id;

    int _client_socket;

    struct sockaddr _client_addr;

    socklen_t _client_addr_len;
};

} // namespace MTblocking
} // namespace Network
} // namespace Afina
#endif // AFINA_NETWORK_MTBLOCKING_WORKER_H
