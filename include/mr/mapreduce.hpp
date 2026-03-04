#pragma once

#include <algorithm>
#include <functional>
#include <map>
#include <mutex>
#include <stdexcept>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include "mr/thread_pool.hpp"

namespace mr {

/// Generic MapReduce engine.
///
/// Type parameters
/// ───────────────
/// K1, V1  – input  key / value types
/// K2, V2  – intermediate key / value types (emitted by the map step)
/// V3      – final output value type (produced by the reduce step)
///
/// The map function receives one input (K1, V1) pair and returns zero or more
/// intermediate (K2, V2) pairs.
///
/// The reduce function receives an intermediate key K2 and the *sorted* list
/// of all values V2 that were associated with that key, and returns a single
/// aggregated value V3.
template <typename K1, typename V1,
          typename K2, typename V2,
          typename V3>
class MapReduce {
public:
    using InputPair      = std::pair<K1, V1>;
    using IntermPair     = std::pair<K2, V2>;
    using OutputPair     = std::pair<K2, V3>;
    using MapFunction    = std::function<std::vector<IntermPair>(const K1&, const V1&)>;
    using ReduceFunction = std::function<V3(const K2&, const std::vector<V2>&)>;

    /// @param map_fn       User-supplied map function.
    /// @param reduce_fn    User-supplied reduce function.
    /// @param num_threads  Worker thread count (0 → hardware concurrency).
    MapReduce(MapFunction    map_fn,
              ReduceFunction reduce_fn,
              std::size_t    num_threads = 0)
        : map_fn_(std::move(map_fn))
        , reduce_fn_(std::move(reduce_fn))
        , pool_(num_threads == 0
                    ? std::thread::hardware_concurrency()
                    : num_threads)
    {}

    /// Execute the MapReduce pipeline over \p input and return the result.
    ///
    /// The pipeline is:
    ///   1. **Map**     – each input pair is processed by map_fn (in parallel).
    ///   2. **Shuffle** – intermediate pairs are grouped by K2.
    ///   3. **Reduce**  – each group is processed by reduce_fn (in parallel).
    std::map<K2, V3> run(const std::vector<InputPair>& input)
    {
        // ── 1. Map phase ──────────────────────────────────────────────────
        const std::size_t n = input.size();
        std::vector<std::future<std::vector<IntermPair>>> map_futures;
        map_futures.reserve(n);

        for (const auto& kv : input) {
            map_futures.push_back(
                pool_.submit([this, &kv]() {
                    return map_fn_(kv.first, kv.second);
                }));
        }

        // ── 2. Shuffle phase ─────────────────────────────────────────────
        std::map<K2, std::vector<V2>> groups;
        for (auto& fut : map_futures) {
            for (auto& pair : fut.get()) {
                groups[pair.first].push_back(std::move(pair.second));
            }
        }

        // ── 3. Reduce phase ──────────────────────────────────────────────
        std::vector<std::future<OutputPair>> reduce_futures;
        reduce_futures.reserve(groups.size());

        for (auto& [key, vals] : groups) {
            reduce_futures.push_back(
                pool_.submit([this, &key, &vals]() -> OutputPair {
                    return {key, reduce_fn_(key, vals)};
                }));
        }

        std::map<K2, V3> result;
        for (auto& fut : reduce_futures) {
            auto [k, v] = fut.get();
            result.emplace(std::move(k), std::move(v));
        }
        return result;
    }

private:
    MapFunction    map_fn_;
    ReduceFunction reduce_fn_;
    ThreadPool     pool_;
};

} // namespace mr
