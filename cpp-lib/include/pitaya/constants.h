#ifndef PITAYA_CONSTANTS_H
#define PITAYA_CONSTANTS_H

#include <string>

namespace pitaya {
namespace constants {

static constexpr const char* kCodeNotFound = "PIT-404";
static constexpr const char* kCodeInternalError = "PIT-500";
static constexpr const char* kCodeTimeout = "PIT-504";
static constexpr const char* kPeerIdKey = "peer.id";
static constexpr const char* kPeerServiceKey = "peer.service";

} // namespace pitaya

#endif // PITAYA_CONSTANTS_H
