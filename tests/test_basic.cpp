/// test_basic.cpp – Unit tests for the MapReduce pipeline.
///
/// Uses a minimal hand-rolled test harness (no external framework required).

#include "mr/mapreduce.hpp"

#include <cassert>
#include <cstddef>
#include <iostream>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

// ── Helpers ──────────────────────────────────────────────────────────────────

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

TEST(word_count_basic)
{
    std::vector<std::pair<int, std::string>> docs = {
        {1, "a b c"},
        {2, "a b"},
        {3, "a"},
    };

    auto map_fn = [](const int&, const std::string& text)
        -> std::vector<std::pair<std::string, int>>
    {
        std::vector<std::pair<std::string, int>> out;
        std::istringstream iss(text);
        std::string w;
        while (iss >> w) out.emplace_back(w, 1);
        return out;
    };

    auto reduce_fn = [](const std::string&, const std::vector<int>& vs) -> int {
        int s = 0; for (int v : vs) s += v; return s;
    };

    mr::MapReduce<int, std::string, std::string, int, int> mr(map_fn, reduce_fn, 2);
    auto res = mr.run(docs);

    ASSERT_EQ(res.at("a"), 3);
    ASSERT_EQ(res.at("b"), 2);
    ASSERT_EQ(res.at("c"), 1);
}

TEST(empty_input_returns_empty_map)
{
    std::vector<std::pair<int, std::string>> docs;

    auto map_fn = [](const int&, const std::string&)
        -> std::vector<std::pair<std::string, int>> { return {}; };
    auto reduce_fn = [](const std::string&, const std::vector<int>&) -> int { return 0; };

    mr::MapReduce<int, std::string, std::string, int, int> mr(map_fn, reduce_fn, 2);
    auto res = mr.run(docs);

    ASSERT_TRUE(res.empty());
}

TEST(sum_values_per_key)
{
    std::vector<std::pair<std::string, int>> input = {
        {"x", 10}, {"y", 20}, {"x", 30}, {"y", 5},
    };

    auto map_fn = [](const std::string& k, const int& v)
        -> std::vector<std::pair<std::string, int>>
    {
        return {{k, v}};
    };

    auto reduce_fn = [](const std::string&, const std::vector<int>& vs) -> int {
        int s = 0; for (int v : vs) s += v; return s;
    };

    mr::MapReduce<std::string, int, std::string, int, int> mr(map_fn, reduce_fn, 2);
    auto res = mr.run(input);

    ASSERT_EQ(res.at("x"), 40);
    ASSERT_EQ(res.at("y"), 25);
}

TEST(single_thread_correctness)
{
    std::vector<std::pair<int, int>> input = {{1, 1}, {2, 2}, {3, 3}};

    auto map_fn = [](const int& k, const int& v)
        -> std::vector<std::pair<int, int>> { return {{k, v * v}}; };

    auto reduce_fn = [](const int&, const std::vector<int>& vs) -> int {
        return vs[0];
    };

    mr::MapReduce<int, int, int, int, int> mr(map_fn, reduce_fn, 1);
    auto res = mr.run(input);

    ASSERT_EQ(res.at(1), 1);
    ASSERT_EQ(res.at(2), 4);
    ASSERT_EQ(res.at(3), 9);
}

TEST(map_emits_multiple_pairs_per_input)
{
    // Each input emits two intermediate keys
    std::vector<std::pair<int, int>> input = {{0, 1}, {0, 2}};

    auto map_fn = [](const int&, const int& v)
        -> std::vector<std::pair<std::string, int>>
    {
        return {{"even", v % 2 == 0 ? v : 0}, {"odd", v % 2 != 0 ? v : 0}};
    };

    auto reduce_fn = [](const std::string&, const std::vector<int>& vs) -> int {
        int s = 0; for (int v : vs) s += v; return s;
    };

    mr::MapReduce<int, int, std::string, int, int> mr(map_fn, reduce_fn, 2);
    auto res = mr.run(input);

    // "even" bucket gets 2 (from input 2) + 0 (from input 1) = 2
    // "odd"  bucket gets 0 (from input 2) + 1 (from input 1) = 1
    ASSERT_EQ(res.at("even"), 2);
    ASSERT_EQ(res.at("odd"),  1);
}

// ── main ─────────────────────────────────────────────────────────────────────

int main()
{
    std::cout << "\nRunning MapReduce basic tests...\n\n";
    if (g_tests_failed > 0) {
        std::cout << "\n" << g_tests_failed << "/" << g_tests_run << " tests FAILED.\n";
        return 1;
    }
    std::cout << "\nAll " << g_tests_run << " tests passed.\n";
    return 0;
}
