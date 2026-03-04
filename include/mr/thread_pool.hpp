#pragma once

#include <condition_variable>
#include <functional>
#include <future>
#include <mutex>
#include <queue>
#include <stdexcept>
#include <thread>
#include <vector>

namespace mr {

/// A simple thread pool that executes submitted tasks asynchronously.
class ThreadPool {
public:
    /// Construct a pool with \p num_threads worker threads.
    /// Passing 0 uses std::thread::hardware_concurrency() (at least 1).
    explicit ThreadPool(std::size_t num_threads = 0);

    /// Destructor – waits for all pending tasks to finish before joining.
    ~ThreadPool();

    // Non-copyable, non-movable
    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;

    /// Submit a callable with arguments; returns a future for its result.
    template <typename F, typename... Args>
    auto submit(F&& f, Args&&... args)
        -> std::future<std::invoke_result_t<F, Args...>>;

    /// Block until all currently-queued tasks have completed.
    void wait_all();

    /// Return the number of worker threads.
    std::size_t size() const noexcept { return workers_.size(); }

private:
    std::vector<std::thread> workers_;
    std::queue<std::function<void()>> tasks_;

    std::mutex              queue_mutex_;
    std::condition_variable cv_;
    bool                    stop_ = false;

    std::size_t             active_tasks_ = 0;
    std::condition_variable idle_cv_;
};

// ─────────────────────────────────────────────────────────────────────────────
// Template implementation
// ─────────────────────────────────────────────────────────────────────────────

template <typename F, typename... Args>
auto ThreadPool::submit(F&& f, Args&&... args)
    -> std::future<std::invoke_result_t<F, Args...>>
{
    using RetType = std::invoke_result_t<F, Args...>;

    auto task = std::make_shared<std::packaged_task<RetType()>>(
        std::bind(std::forward<F>(f), std::forward<Args>(args)...));

    std::future<RetType> result = task->get_future();

    {
        std::unique_lock<std::mutex> lock(queue_mutex_);
        if (stop_) {
            throw std::runtime_error("ThreadPool is stopped");
        }
        ++active_tasks_;
        tasks_.emplace([task]() { (*task)(); });
    }
    cv_.notify_one();
    return result;
}

} // namespace mr
