#ifndef AFINA_NETWORK_MTBLOCKING_WORKER_H
#define AFINA_NETWORK_MTBLOCKING_WORKER_H

#include <sys/socket.h>

#include <atomic>
#include <memory>
#include <thread>
#include <mutex>
#include <condition_variable>

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
    Worker(std::shared_ptr<Afina::Storage> ps, std::shared_ptr<Afina::Logging::Service> pl, std::vector<std::unique_ptr<Worker>> *workers, std::mutex *workers_mutex, std::condition_variable *serv_lock);
    ~Worker();

    Worker(Worker &&);
    Worker &operator=(Worker &&);

    void Start(int worker_id, int client_socket, struct sockaddr& client_addr);

    void Stop();

    void Join();
    
    int id() {
        return _worker_id;
    }

protected:
    void OnRun();

private:
    Worker(Worker &) = delete;
    Worker &operator=(Worker &) = delete;

    std::shared_ptr<Afina::Storage> _pStorage;

    std::shared_ptr<Afina::Logging::Service> _pLogging;

    std::shared_ptr<spdlog::logger> _logger;

    std::atomic<bool> isRunning;

    std::thread _thread;

    int _worker_id;
    
    int _client_socket;
    
    struct sockaddr _client_addr;
    
    socklen_t _client_addr_len;
    
    std::vector<std::unique_ptr<Worker>> *_workers;
    std::mutex *_workers_mutex;
    std::condition_variable *_serv_lock;
};

} // namespace MTblocking
} // namespace Network
} // namespace Afina
#endif // AFINA_NETWORK_MTBLOCKING_WORKER_H
