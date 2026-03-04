cmake_minimum_required(VERSION 3.14)
project(mapreduce-cpp
    VERSION      0.1.0
    DESCRIPTION  "Header-mostly C++ MapReduce library with a thread-pool backend"
    LANGUAGES    CXX
)

# ── C++ standard ─────────────────────────────────────────────────────────────
set(CMAKE_CXX_STANDARD          17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS        OFF)

# ── Options ──────────────────────────────────────────────────────────────────
option(MR_BUILD_EXAMPLES  "Build example programs"  ON)
option(MR_BUILD_TESTS     "Build unit tests"         ON)
option(MR_ENABLE_MPI      "Enable the MPI backend"   OFF)

# ── Interface / header library ───────────────────────────────────────────────
# The MapReduce engine itself is header-only; only thread_pool has a .cpp file.
add_library(mr_headers INTERFACE)
target_include_directories(mr_headers INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

# ── Thread-pool library ───────────────────────────────────────────────────────
add_library(mr_threadpool STATIC
    src/thread_pool.cpp
)
target_include_directories(mr_threadpool PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
find_package(Threads REQUIRED)
target_link_libraries(mr_threadpool PUBLIC Threads::Threads)

# ── MPI backend (optional) ────────────────────────────────────────────────────
if(MR_ENABLE_MPI)
    find_package(MPI REQUIRED)
    add_library(mr_mpi STATIC src/mpi_backend.cpp)
    target_compile_definitions(mr_mpi PUBLIC MR_ENABLE_MPI)
    target_include_directories(mr_mpi PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    )
    target_link_libraries(mr_mpi PUBLIC MPI::MPI_CXX mr_threadpool)
endif()

# ── Examples ──────────────────────────────────────────────────────────────────
if(MR_BUILD_EXAMPLES)
    add_executable(word_count   examples/word_count.cpp)
    add_executable(histogram    examples/histogram.cpp)

    foreach(_ex word_count histogram)
        target_link_libraries(${_ex} PRIVATE mr_threadpool)
    endforeach()
endif()

# ── Tests ─────────────────────────────────────────────────────────────────────
if(MR_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
