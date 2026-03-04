#include "mr/thread_pool.hpp"

#include <stdexcept>

namespace mr {

ThreadPool::ThreadPool(std::size_t num_threads)
{
    if (num_threads == 0) {
        num_threads = std::thread::hardware_concurrency();
        if (num_threads == 0) num_threads = 1;
    }

    workers_.reserve(num_threads);
    for (std::size_t i = 0; i < num_threads; ++i) {
        workers_.emplace_back([this]() {
            for (;;) {
                std::function<void()> task;
                {
                    std::unique_lock<std::mutex> lock(queue_mutex_);
                    cv_.wait(lock, [this]() {
                        return stop_ || !tasks_.empty();
                    });

                    if (stop_ && tasks_.empty()) return;

                    task = std::move(tasks_.front());
                    tasks_.pop();
                }

                try {
                    task();
                } catch (...) {
                    // Exceptions are surfaced through the future; swallow here.
                }

                {
                    std::unique_lock<std::mutex> lock(queue_mutex_);
                    --active_tasks_;
                }
                idle_cv_.notify_all();
            }
        });
    }
}

ThreadPool::~ThreadPool()
{
    {
        std::unique_lock<std::mutex> lock(queue_mutex_);
        stop_ = true;
    }
    cv_.notify_all();
    for (auto& w : workers_) w.join();
}

void ThreadPool::wait_all()
{
    std::unique_lock<std::mutex> lock(queue_mutex_);
    idle_cv_.wait(lock, [this]() {
        return active_tasks_ == 0 && tasks_.empty();
    });
}

} // namespace mr
