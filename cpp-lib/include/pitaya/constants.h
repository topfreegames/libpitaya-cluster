#ifndef PITAYA_CONSTANTS_H
#define PITAYA_CONSTANTS_H

#include <string>

namespace pitaya {
namespace constants {

static constexpr const char* kCodeNotFound = "PIT-404";
static constexpr const char* kCodeNotImplemented = "PIT-501";
static constexpr const char* kCodeInternalError = "PIT-500";
static constexpr const char* kCodeTimeout = "PIT-504";
static constexpr const char* kCodeUnprocessableEntity = "PIT-422";
static constexpr const char* kPeerIdKey = "peer.id";
static constexpr const char* kPeerServiceKey = "peer.service";

static constexpr const char* kGrpcHostKey = "grpc-host";
static constexpr const char* kGrpcPortKey = "grpc-port";

} // namespace constants
} // namespace pitaya

#endif // PITAYA_CONSTANTS_H
