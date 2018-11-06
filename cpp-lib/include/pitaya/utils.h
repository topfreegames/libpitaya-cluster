#ifndef PITAYA_UTILS_H
#define PITAYA_UTILS_H

#include "pitaya.h"
#include <string>

namespace pitaya {
namespace utils {

std::string GetTopicForServer(const Server& server);
Server RandomServer(const std::vector<Server>& vec);

} // namespace utils
} // namespace pitaya

#endif // PITAYA_UTILS_H