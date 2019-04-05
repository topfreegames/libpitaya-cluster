#ifndef PITAYA_UTILS_H
#define PITAYA_UTILS_H

#include "pitaya.h"

#include "spdlog/logger.h"

#include <string>

namespace pitaya {
namespace utils {

std::string GetUserKickTopic(const std::string& userId, const std::string& serverType);

std::string GetUserMessagesTopic(const std::string& userId, const std::string& serverType);

std::string GetTopicForServer(const std::string& serverId, const std::string& serverType);

Server RandomServer(const std::vector<Server>& vec);

bool ParseEtcdKey(const std::string& key,
                  const std::string& etcdPrefix,
                  std::string& serverType,
                  std::string& serverId);

std::shared_ptr<spdlog::logger> CloneLoggerOrCreate(const char* loggerNameToClone,
                                                    const char* loggerName);

std::size_t get_thread_id() noexcept;

} // namespace utils
} // namespace pitaya

#endif // PITAYA_UTILS_H
