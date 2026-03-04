# Usage Guide

## Requirements

| Tool | Minimum version |
|------|----------------|
| C++ compiler | GCC 8 / Clang 7 / MSVC 19.14 (C++17) |
| CMake | 3.14 |
| (optional) MPI | OpenMPI 4 / MPICH 3 |

---

## Building

```bash
# 1. Clone the repository
git clone https://github.com/vovaw123/MapReduce.git
cd MapReduce

# 2. Configure
cmake -B build \
      -DCMAKE_BUILD_TYPE=Release \
      -DMR_BUILD_EXAMPLES=ON \
      -DMR_BUILD_TESTS=ON

# 3. Build
cmake --build build --parallel

# 4. Run tests
cd build && ctest --output-on-failure
```

To enable the (future) MPI backend:

```bash
cmake -B build -DMR_ENABLE_MPI=ON ..
cmake --build build --parallel
```

---

## Quick Start

### Word Count

```cpp
#include "mr/mapreduce.hpp"
#include <sstream>
#include <string>
#include <vector>
#include <utility>

int main() {
    std::vector<std::pair<int, std::string>> docs = {
        {1, "hello world"},
        {2, "hello mapreduce"},
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

    mr::MapReduce<int, std::string,   // input  key / value
                  std::string, int,   // intermediate key / value
                  int>                // output value
        engine(map_fn, reduce_fn, /*num_threads=*/4);

    auto result = engine.run(docs);
    // result["hello"] == 2, result["world"] == 1, result["mapreduce"] == 1
}
```

---

## API Reference

### `mr::MapReduce<K1, V1, K2, V2, V3>`

| Member | Description |
|--------|-------------|
| `MapReduce(MapFunction, ReduceFunction, size_t threads=0)` | Construct the engine.  `threads=0` uses `hardware_concurrency`. |
| `std::map<K2,V3> run(const std::vector<std::pair<K1,V1>>&)` | Execute the full pipeline and return the result. |

**Type aliases inside the class:**

```cpp
using MapFunction    = std::function<std::vector<std::pair<K2,V2>>(const K1&, const V1&)>;
using ReduceFunction = std::function<V3(const K2&, const std::vector<V2>&)>;
```

---

### `mr::ThreadPool`

| Member | Description |
|--------|-------------|
| `ThreadPool(size_t n=0)` | Create pool with `n` workers (`0` → hardware concurrency). |
| `auto submit(F&&, Args&&...)` | Submit a callable; returns `std::future<ReturnType>`. |
| `void wait_all()` | Block until all submitted tasks have finished. |
| `size_t size()` | Return the number of worker threads. |

---

## Running the Examples

```bash
./build/word_count
./build/histogram
```

---

## Thread Safety

- `MapReduce::run()` is **not** thread-safe with respect to the same
  `MapReduce` instance.  Create separate instances or synchronise externally.
- `ThreadPool::submit()` is thread-safe; it can be called concurrently from
  multiple threads.
