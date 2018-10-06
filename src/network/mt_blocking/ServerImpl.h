#ifndef AFINA_NETWORK_MT_BLOCKING_SERVER_H
#define AFINA_NETWORK_MT_BLOCKING_SERVER_H

#include <atomic>
#include <thread>

#include <afina/network/Server.h>

#include "Worker.h"

namespace spdlog {
class logger;
}

namespace Afina {
namespace Network {
namespace MTblocking {
class Worker {
public:
    Worker(std::shared_ptr<Afina::Storage> ps, std::shared_ptr<Afina::Logging::Service> pl, std::vector<std::unique_ptr<Worker>>& workers, std::mutex& workers_mutex, std::condition_variable& serv_lock);
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
    
    struct sockaddr& _client_addr;
    
    socklen_t _client_addr_len;
    
    std::vector<std::unique_ptr<Worker>>& _workers;
    std::mutex& _workers_mutex;
    std::condition_variable& _serv_lock;
};

/**
 * # Network resource manager implementation
 * Server that is spawning a separate thread for each connection
 */
class ServerImpl : public Server {
public:
    ServerImpl(std::shared_ptr<Afina::Storage> ps, std::shared_ptr<Logging::Service> pl);
    ~ServerImpl();

    // See Server.h
    void Start(uint16_t port, uint32_t, uint32_t) override;

    // See Server.h
    void Stop() override;

    // See Server.h
    void Join() override;

protected:
    /**
     * Method is running in the connection acceptor thread
     */
    void OnRun();

private:
    // Logger instance
    std::shared_ptr<spdlog::logger> _logger;

    // Atomic flag to notify threads when it is time to stop. Note that
    // flag must be atomic in order to safely publisj changes cross thread
    // bounds
    std::atomic<bool> running;

    // Server socket to accept connections on
    int _server_socket;

    // Thread to run network on
    std::thread _thread;
    
    std::size_t _max_workers = 100;
    int _wid = 0;
    std::vector<std::unique_ptr<Worker>> _workers;
    std::mutex _workers_mutex;
    std::condition_variable _serv_lock;
};

} // namespace MTblocking
} // namespace Network
} // namespace Afina

#endif // AFINA_NETWORK_MT_BLOCKING_SERVER_H
