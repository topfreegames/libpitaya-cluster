#ifndef PITAYA_UTILS_GRPC_H
#define PITAYA_UTILS_GRPC_H

#include "pitaya.h"

namespace pitaya {
namespace utils {

std::string GetGrpcAddressFromServer(const Server& server);

} // namespace utils
} // namespace pitaya

#endif // PITAYA_UTILS_GRPC_H