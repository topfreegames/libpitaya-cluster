#include "pitaya/utils.h"

#include "pitaya.h"
#include "pitaya/utils/string_utils.h"
#include "spdlog/spdlog.h"

#include "spdlog/sinks/stdout_color_sinks.h"

#include <assert.h>
#include <boost/format.hpp>
#include <mutex>
#include <random>
#include <string>
#include <thread>
#include <unordered_map>

using std::string;

namespace pitaya {
namespace utils {

std::string
GetUserKickTopic(const std::string& userId, const std::string& serverType)
{
    return boost::str(boost::format("pitaya/%1%/user/%2%/kick") % serverType % userId);
}

std::string
GetUserMessagesTopic(const std::string& userId, const std::string& serverType)
{
    return boost::str(boost::format("pitaya/%1%/user/%2%/push") % serverType % userId);
}

string
GetTopicForServer(const std::string& serverId, const std::string& serverType)
{
    return boost::str(boost::format("pitaya/servers/%1%/%2%") % serverType % serverId);
}

pitaya::Server
RandomServer(const std::vector<Server>& vec)
{
    std::random_device random_device;
    std::mt19937 engine{ random_device() };
    std::uniform_int_distribution<int> dist(0, vec.size() - 1);
    Server random_element = vec[dist(engine)];
    return random_element;
}

bool
ParseEtcdKey(const string& key,
             const string& etcdPrefix,
             const std::vector<string>& serverTypeFilters,
             string& outServerType,
             string& outServerId)
{
    assert(!etcdPrefix.empty());
    assert(!key.empty());

    if (std::find(key.begin(), key.end(), '/') == key.end()) {
        // If the key does not contain a '/', it means it is not in a format
        // that is supported by the program, therefore just return NotAServer.
        return false;
    }

    auto comps = string_utils::Split(key, '/');

    auto prefix = comps[0];
    
    // Fail if the key does not start with "<etcdPrefix>/"
    if ((prefix + "/") != etcdPrefix) {
        return false;
    }

    if (comps.size() < 4) {
        return false;
    }

    // Fail if the key does not start with "<etcdPrefix>/servers"
    auto serversLiteral = comps[1];
    if (serversLiteral != "servers") {
        return false;
    }

    // If it got here, it means that the key starts with <etcdPrefix>/servers/
    outServerType = comps[2];
    outServerId = comps[3];

    // Neither the server type nor the server id should be empty, therefore we check for that.
    if (outServerType.empty() || outServerId.empty()) {
        outServerType = "";
        outServerId = "";
        return false;
    }
    
    // If there are filters in the array, we try to find the server type in the array,
    // otherwise we return with an error.
    if (!serverTypeFilters.empty()) {
        auto it = std::find(serverTypeFilters.begin(), serverTypeFilters.end(), outServerType);
        if (it == serverTypeFilters.end()) {
            outServerType = "";
            outServerId = "";
            return false;
        }
    }

    return true;
}

std::shared_ptr<spdlog::logger>
CloneLoggerOrCreate(const char* loggerNameToClone, const char* loggerName)
{
    assert(loggerName);
    // there is a logger name, so we just clone it into the new loggerName.
    if (loggerNameToClone && spdlog::get(loggerNameToClone)) {
        return spdlog::get(loggerNameToClone)->clone(loggerName);
    }

    auto sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    auto logger = std::make_shared<spdlog::logger>(loggerName, sink);
    logger->set_level(spdlog::default_logger()->level());
    return logger;
}

} // namespace utils
} // namespace pitaya
