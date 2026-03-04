# Design

## Overview

`mapreduce-cpp` is a header-mostly C++17 library that provides a generic,
in-process MapReduce pipeline backed by a configurable thread pool.

---

## Architecture

```
Input pairs  →  [Map phase]  →  Intermediate pairs
                                      ↓
                             [Shuffle / Group]
                                      ↓
                             [Reduce phase]  →  Output map
```

### Components

| Component | Location | Role |
|-----------|----------|------|
| `mr::MapReduce<K1,V1,K2,V2,V3>` | `include/mr/mapreduce.hpp` | Public API – orchestrates map, shuffle, reduce |
| `mr::ThreadPool` | `include/mr/thread_pool.hpp` + `src/thread_pool.cpp` | Fixed-size worker thread pool |
| `mr::mpi::MpiMapReduce` | `include/mr/mpi_backend.hpp` + `src/mpi_backend.cpp` | (Future) distributed MPI backend |

---

## MapReduce Pipeline

### Map Phase
Each input `(K1, V1)` pair is submitted as an independent task to the thread
pool.  The user-supplied `map_fn` transforms it into zero or more intermediate
`(K2, V2)` pairs.

### Shuffle Phase
All intermediate pairs are collected (single-threaded) and grouped into a
`std::map<K2, std::vector<V2>>`.  The values within each group preserve
insertion order.

### Reduce Phase
Each `(K2, std::vector<V2>)` group is submitted as an independent task to the
thread pool.  The user-supplied `reduce_fn` collapses the value list into a
single `V3`.

---

## Thread Pool

`mr::ThreadPool` maintains a fixed set of `std::thread` workers and a
`std::queue` of `std::function<void()>` tasks protected by a `std::mutex`.

- `submit()` wraps the callable in a `std::packaged_task`, pushes it onto the
  queue, and returns the associated `std::future`.
- Workers block on `std::condition_variable::wait` until a task is available or
  the pool is stopped.
- `wait_all()` blocks the caller until the task queue is empty and all active
  tasks have finished.
- The destructor sets a `stop_` flag, notifies all workers, and joins them.

---

## MPI Backend (future)

The `mr::mpi::MpiMapReduce` class will distribute the map phase across MPI
ranks and perform an MPI-based shuffle.  The reduce phase will execute on each
rank that receives a portion of the intermediate keys.

Build with:
```
cmake -DMR_ENABLE_MPI=ON ..
```

---

## Template Instantiation

Because the library is largely header-only, template instantiation happens in
the translation unit of the caller.  There is no exported symbol table for the
`MapReduce` class itself – only `ThreadPool` is compiled into a static library
(`mr_threadpool`).
