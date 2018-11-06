#include "pitaya.h"
#include <boost/format.hpp>
#include <random>
#include <string>

using std::string;

namespace pitaya {
namespace utils {

string
GetTopicForServer(const Server& server)
{
    return boost::str(boost::format("pitaya/servers/%1%/%2%") % server.type % server.id);
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

} // namespace utils
} // namespace pitaya