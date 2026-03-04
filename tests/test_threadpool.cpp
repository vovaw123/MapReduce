/// test_threadpool.cpp – Unit tests for the ThreadPool.

#include "mr/thread_pool.hpp"

#include <atomic>
#include <cassert>
#include <chrono>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

// ── Minimal test harness (same as test_basic.cpp) ────────────────────────────

static int g_tests_run    = 0;
static int g_tests_failed = 0;

#define TEST(name)                                                           \
    static void name();                                                      \
    namespace { struct _reg_##name {                                         \
        _reg_##name() { run_test(#name, name); }                            \
    } _inst_##name; }                                                        \
    static void name()

static void run_test(const char* name, void (*fn)())
{
    ++g_tests_run;
    try {
        fn();
        std::cout << "[PASS] " << name << "\n";
    } catch (const std::exception& e) {
        ++g_tests_failed;
        std::cout << "[FAIL] " << name << ": " << e.what() << "\n";
    } catch (...) {
        ++g_tests_failed;
        std::cout << "[FAIL] " << name << ": unknown exception\n";
    }
}

#define ASSERT_EQ(a, b)                                                      \
    do {                                                                     \
        if (!((a) == (b))) {                                                 \
            std::ostringstream _oss;                                         \
            _oss << "ASSERT_EQ failed at " __FILE__ ":" << __LINE__         \
                 << "  lhs=" << (a) << "  rhs=" << (b);                    \
            throw std::runtime_error(_oss.str());                           \
        }                                                                   \
    } while (false)

#define ASSERT_TRUE(expr)                                                    \
    do {                                                                     \
        if (!(expr)) {                                                       \
            std::ostringstream _oss;                                         \
            _oss << "ASSERT_TRUE failed: " #expr                            \
                 << " at " __FILE__ ":" << __LINE__;                        \
            throw std::runtime_error(_oss.str());                           \
        }                                                                   \
    } while (false)

// ── Tests ────────────────────────────────────────────────────────────────────

TEST(default_size_at_least_one)
{
    mr::ThreadPool pool; // 0 → hardware_concurrency ≥ 1
    ASSERT_TRUE(pool.size() >= 1);
}

TEST(explicit_size)
{
    mr::ThreadPool pool(3);
    ASSERT_EQ(pool.size(), std::size_t(3));
}

TEST(submit_returns_correct_value)
{
    mr::ThreadPool pool(2);
    auto fut = pool.submit([]() { return 42; });
    ASSERT_EQ(fut.get(), 42);
}

TEST(multiple_tasks_all_complete)
{
    constexpr int N = 100;
    mr::ThreadPool pool(4);
    std::vector<std::future<int>> futs;
    futs.reserve(N);

    for (int i = 0; i < N; ++i) {
        futs.push_back(pool.submit([i]() { return i * i; }));
    }

    for (int i = 0; i < N; ++i) {
        ASSERT_EQ(futs[i].get(), i * i);
    }
}

TEST(atomic_counter_incremented_by_all_tasks)
{
    constexpr int N = 200;
    mr::ThreadPool pool(4);
    std::atomic<int> counter{0};

    std::vector<std::future<void>> futs;
    futs.reserve(N);
    for (int i = 0; i < N; ++i) {
        futs.push_back(pool.submit([&counter]() { ++counter; }));
    }
    for (auto& f : futs) f.get();

    ASSERT_EQ(counter.load(), N);
}

TEST(wait_all_blocks_until_done)
{
    mr::ThreadPool pool(2);
    std::atomic<int> counter{0};

    for (int i = 0; i < 50; ++i) {
        pool.submit([&counter]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            ++counter;
        });
    }

    pool.wait_all();
    ASSERT_EQ(counter.load(), 50);
}

TEST(submit_lambda_with_captures)
{
    mr::ThreadPool pool(2);
    int x = 7, y = 3;
    auto fut = pool.submit([x, y]() { return x + y; });
    ASSERT_EQ(fut.get(), 10);
}

TEST(exception_propagates_through_future)
{
    mr::ThreadPool pool(1);
    auto fut = pool.submit([]() -> int {
        throw std::runtime_error("test error");
        return 0;
    });

    bool caught = false;
    try {
        fut.get();
    } catch (const std::runtime_error& e) {
        caught = (std::string(e.what()) == "test error");
    }
    ASSERT_TRUE(caught);
}

// ── main ─────────────────────────────────────────────────────────────────────

int main()
{
    std::cout << "\nRunning ThreadPool tests...\n\n";
    if (g_tests_failed > 0) {
        std::cout << "\n" << g_tests_failed << "/" << g_tests_run << " tests FAILED.\n";
        return 1;
    }
    std::cout << "\nAll " << g_tests_run << " tests passed.\n";
    return 0;
}
