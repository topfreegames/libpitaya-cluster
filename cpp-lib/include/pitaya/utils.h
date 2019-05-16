#ifndef PITAYA_UTILS_H
#define PITAYA_UTILS_H

#include "pitaya.h"

#include "spdlog/logger.h"

#include <string>
#include <thread>

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

inline void SetThreadName(const char* name, std::shared_ptr<spdlog::logger> log)
{
#ifdef linux
    log->debug("Setting thread name for linux to {}", name);
    // Under linux we give the thread a name for debugging purposes.
    // There are no cross platform ways of doing that.
    char buf[16];
    size_t bufLen = 15;
    strncpy(buf, name, bufLen);
    buf[bufLen] = '\0';
    int res = pthread_setname_np(pthread_self(), buf);
    if (log && res) {
        log->error("Failed to set thread name");
    }
#elif _WIN32
    log->warn("Not setting thread id, not implemented on windows yet");
#else
    log->debug("Setting thread name for macosx to {}", name);
    pthread_setname_np(name);
#endif
}

} // namespace utils
} // namespace pitaya

#endif // PITAYA_UTILS_H
