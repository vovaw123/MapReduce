# mapreduce-cpp

A header-mostly **C++17 MapReduce** library with a thread-pool backend.

[![CI](https://github.com/vovaw123/MapReduce/actions/workflows/ci.yml/badge.svg)](https://github.com/vovaw123/MapReduce/actions/workflows/ci.yml)

---

## Features

- Generic `mr::MapReduce<K1,V1, K2,V2, V3>` template – bring your own key/value types.
- Built-in `mr::ThreadPool` for parallel map and reduce phases.
- Header-only for the MapReduce engine; only the thread pool has a compiled `.cpp`.
- Example programs: word count, histogram.
- Future: optional MPI backend for distributed execution.

---

## Directory structure

```
mapreduce-cpp/
├─ CMakeLists.txt
├─ README.md
├─ LICENSE
├─ docs/
│  ├─ design.md          ← architecture & internals
│  └─ usage.md           ← build instructions & API reference
├─ include/
│  └─ mr/
│     ├─ mapreduce.hpp   ← public API (templates)
│     ├─ thread_pool.hpp ← thread pool
│     └─ mpi_backend.hpp ← (future) MPI backend API
├─ src/
│  ├─ thread_pool.cpp
│  └─ mpi_backend.cpp    ← (future) MPI implementation
├─ examples/
│  ├─ word_count.cpp
│  └─ histogram.cpp
├─ tests/
│  ├─ CMakeLists.txt
│  ├─ test_basic.cpp
│  └─ test_threadpool.cpp
└─ .github/
   └─ workflows/
      └─ ci.yml
```

---

## Quick start

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
cd build && ctest --output-on-failure
```

See [docs/usage.md](docs/usage.md) for full details.

---

## License

[MIT](LICENSE)