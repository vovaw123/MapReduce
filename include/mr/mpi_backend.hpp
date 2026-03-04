#pragma once

// MPI backend – placeholder / future extension
//
// This header declares the MPI-based backend for MapReduce.
// The actual implementation requires MPI to be available at compile time.
// To enable it, build with -DMR_ENABLE_MPI and link against an MPI library.
//
// Example CMake usage:
//   find_package(MPI REQUIRED)
//   target_compile_definitions(my_target PRIVATE MR_ENABLE_MPI)
//   target_link_libraries(my_target PRIVATE MPI::MPI_CXX)

#include <cstddef>
#include <functional>
#include <map>
#include <string>
#include <utility>
#include <vector>

namespace mr {
namespace mpi {

#ifdef MR_ENABLE_MPI
#include <mpi.h>

/// Serialize a value into a byte buffer (must be specialized for custom types).
template <typename T>
std::vector<char> serialize(const T& value);

/// Deserialize a value from a byte buffer (must be specialized for custom types).
template <typename T>
T deserialize(const std::vector<char>& buffer);

/// MPI-aware MapReduce driver.
///
/// Call run() from *all* MPI ranks.  Rank 0 acts as the coordinator
/// (distributes input, collects results); all other ranks act as workers.
///
/// NOTE: This is the API stub.  Full implementation in src/mpi_backend.cpp
///       is gated behind MR_ENABLE_MPI.
template <typename K1, typename V1,
          typename K2, typename V2,
          typename V3>
class MpiMapReduce {
public:
    using InputPair      = std::pair<K1, V1>;
    using IntermPair     = std::pair<K2, V2>;
    using OutputPair     = std::pair<K2, V3>;
    using MapFunction    = std::function<std::vector<IntermPair>(const K1&, const V1&)>;
    using ReduceFunction = std::function<V3(const K2&, const std::vector<V2>&)>;

    MpiMapReduce(MapFunction map_fn, ReduceFunction reduce_fn,
                 MPI_Comm comm = MPI_COMM_WORLD)
        : map_fn_(std::move(map_fn))
        , reduce_fn_(std::move(reduce_fn))
        , comm_(comm)
    {}

    /// Execute the distributed MapReduce pipeline.
    /// Returns the result map on rank 0; returns an empty map on other ranks.
    std::map<K2, V3> run(const std::vector<InputPair>& input);

private:
    MapFunction    map_fn_;
    ReduceFunction reduce_fn_;
    MPI_Comm       comm_;
};

#else // !MR_ENABLE_MPI

// Provide a helpful compile-time error when MPI is not enabled.
template <typename K1, typename V1,
          typename K2, typename V2,
          typename V3>
class MpiMapReduce {
public:
    MpiMapReduce(...) = delete; // Rebuild with -DMR_ENABLE_MPI
};

#endif // MR_ENABLE_MPI

} // namespace mpi
} // namespace mr
