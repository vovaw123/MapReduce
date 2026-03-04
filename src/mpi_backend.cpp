// MPI backend implementation stub.
//
// This file is compiled only when MR_ENABLE_MPI is defined and an MPI
// library is linked.  Full implementation is deferred to a future milestone.

#ifdef MR_ENABLE_MPI

#include "mr/mpi_backend.hpp"

#include <mpi.h>
#include <stdexcept>

namespace mr {
namespace mpi {

// Specializations of serialize/deserialize for built-in types (int, double,
// std::string) and the actual MpiMapReduce::run() body would go here once
// the MPI milestone is reached.

// TODO: implement MpiMapReduce::run() for each template instantiation needed.

} // namespace mpi
} // namespace mr

#endif // MR_ENABLE_MPI
