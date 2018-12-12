#ifndef AFINA_THREADPOOL_H
#define AFINA_THREADPOOL_H

#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <chrono>

namespace Afina {

/**
 * # Thread pool
 */
class Executor {
    enum class State {
        // Threadpool is fully operational, tasks could be added and get executed
        kRun,

        // Threadpool is on the way to be shutdown, no ned task could be added, but existing will be
        // completed as requested
        kStopping,

        // Threadppol is stopped
        kStopped
    };

    Executor(std::string &_name, int _max_queue_size, int _lower = 4, int _higher = 8, int _idle_time = 1000):
              name(_name),
              max_queue_size(_max_queue_size),
              lower_watermark(_lower),
              higher_watermark(_higher),
              idle_time(_idle_time),
              state(State::kRun),
              idle_threads(0) {}

    ~Executor() {
        tasks.erase();
        threads.erase();
    }

    /**
     * Signal thread pool to stop, it will stop accepting new jobs and close threads just after each become
     * free. All enqueued jobs will be complete.
     *
     * In case if await flag is true, call won't return until all background jobs are done and all threads are stopped
     */
    void Stop(bool await = false) {
        state = State::kStopping;
        std::unique_lock<std::mutext> lock(mutex);
        while (tasks.size() > 0) {
            empty_condition.notify_one();
        }
        if (await) {
            while (state == State::kStopping) {}
        }
    };

    /**
     * Add function to be executed on the threadpool. Method returns true in case if task has been placed
     * onto execution queue, i.e scheduled for execution and false otherwise.
     *
     * That function doesn't wait for function result. Function could always be written in a way to notify caller about
     * execution finished by itself
     */
    template <typename F, typename... Types> bool Execute(F &&func, Types... args) {
        // Prepare "task"
        auto exec = std::bind(std::forward<F>(func), std::forward<Types>(args)...);

        std::unique_lock<std::mutex> lock(mutex);
        if (state != State::kRun) {
            return false;
        }

        // Enqueue new task
        if (idle_threads > 0) { // we have idle threads
            tasks.push_back(exec);
            empty_condition.notify_one();
        }
        else { // all threads are working
            if (threads.size() < higher_watermark) { // creating new thread
                tasks.push_back(exec);
                std::unique_ptr<std::thread> new_thread(&thread_run, this);
                threads.push_back(std::move(new_thread));
                threads.back().detach();
                return true;
            }
            else {
                if (tasks.size() < max_queue_size) {
                    tasks.push_back(exec);
                    return true;
                }
                return false;
            }
        }
        return true;
    }

private:
    std::string name;
    int max_queue_size;
    int lower_watermark;
    int higher_watermark;
    int idle_time;

    friend void thread_run(Executor* executor) {
        auto started = std::chrono::system_clock::now();
        while (state == State::kRun) {
            std::unique_lock<std::mutex> lock(executor->mutex);
            executor->idle_threads++;
            while (executor->tasks.empty()) {
                empty_condition.wait(lock);
            }
            executor->idle_threads--;
            auto new = std::chrono::system_clock::now();
            int elapsed = std::chrono::duration_cast<std::chrono::milliseconds> (end - start).count();
            if (executor->state == State::kStopping) {
                auto it = std::find_if(executor->thread.begin(),
                                        executor->threads.end(),
                                        [&](std::unique_ptr<thread>& obj){return obj->get_id() == this->id;});
                assert(it != executor->threads.end()); // if not, we are at the *woops* situation
                executor->threads.erase(it);
                if (executor->threads.size() == 0) {
                    executor->state = State::kStopped;
                }
                return;
            }
            if ((elapsed >= executor->idle_time) && (executor->threads.size() > executor->lower_watermark)) {
                auto it = std::find_if(executor->thread.begin(),
                                        executor->threads.end(),
                                        [&](std::unique_ptr<thread>& obj){return obj->get_id() == this->id;});
                assert(it != executor->threads.end()); // if not, we are at the *woops* situation
                executor->threads.erase(it);
                return;
            }
            auto task = executor->tasks.pop();
            lock.unlock();
            task();
            started = std::chrono::system_clock::now();
        }
    }

    // No copy/move/assign allowed
    Executor(const Executor &);            // = delete;
    Executor(Executor &&);                 // = delete;
    Executor &operator=(const Executor &); // = delete;
    Executor &operator=(Executor &&);      // = delete;

    /**
     * Mutex to protect state below from concurrent modification
     */
    std::mutex mutex;

    /**
     * Conditional variable to await new data in case of empty queue
     */
    std::condition_variable empty_condition;

    /**
     * Vector of actual threads that perorm execution
     */
    std::vector<std::unique_ptr<std::thread>> threads;

    /**
     * Task queue
     */
    std::deque<std::function<void()>> tasks;

    /**
     * Flag to stop bg threads
     */
    std::atomic<State> state = State::kRun;

    std::atomic<int> idle_threads;
};

} // namespace Afina

#endif // AFINA_THREADPOOL_H
