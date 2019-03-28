#ifndef PITAYA_UTILS_H
#define PITAYA_UTILS_H

#include "pitaya.h"

#include <string>

namespace pitaya {
namespace utils {

std::string GetTopicForServer(const Server& server);
Server RandomServer(const std::vector<Server>& vec);

bool ParseEtcdKey(const std::string& key,
                  const std::string& etcdPrefix,
                  std::string& serverType,
                  std::string& serverId);

std::size_t get_thread_id() noexcept;

} // namespace utils
} // namespace pitaya

#endif // PITAYA_UTILS_H
