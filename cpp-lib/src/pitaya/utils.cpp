#include "pitaya/utils.h"

#include "pitaya.h"
#include "pitaya/utils/string_utils.h"

#include <boost/format.hpp>
#include <random>
#include <string>
#include <assert.h>

using std::string;

namespace pitaya {
namespace utils {

string
GetTopicForServer(const Server& server)
{
    return boost::str(boost::format("pitaya/servers/%1%/%2%") % server.Type() % server.Id());
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
ParseEtcdKey(const string& key, const string& etcdPrefix, string& serverType, string& serverId)
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
    if ((prefix + "/") != etcdPrefix) {
        return false;
    }

    if (comps.size() < 4) {
        return false;
    }

    auto serversLiteral = comps[1];
    if (serversLiteral != "servers") {
        return false;
    }

    // If it got here, it means that the key starts with <etcdPrefix>/servers/
    serverType = comps[2];
    serverId = comps[3];

    if (serverType.empty() || serverId.empty()) {
        return false;
    }

    return true;
}

} // namespace utils
} // namespace pitaya
