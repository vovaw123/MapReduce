cmake_minimum_required(VERSION 3.14)

# ── test_threadpool ───────────────────────────────────────────────────────────
add_executable(test_threadpool test_threadpool.cpp)
target_link_libraries(test_threadpool PRIVATE mr_threadpool)
add_test(NAME threadpool COMMAND test_threadpool)

# ── test_basic ────────────────────────────────────────────────────────────────
add_executable(test_basic test_basic.cpp)
target_link_libraries(test_basic PRIVATE mr_threadpool)
add_test(NAME basic COMMAND test_basic)
