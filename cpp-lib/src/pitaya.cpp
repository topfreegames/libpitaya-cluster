#include "pitaya.h"

#include <boost/algorithm/string.hpp>
#include <cpprest/json.h>

namespace json = web::json;

pitaya::Server::Server(const std::string& id,
                       const std::string& type,
                       const std::unordered_map<std::string, std::string>& metadata,
                       const std::string& hostname,
                       bool frontend)
    : id(id)
    , type(type)
    , hostname(hostname)
    , frontend(frontend)
{
    auto obj = json::value::object();
    for (const auto& entry : metadata) {
        obj[entry.first] = json::value::string(entry.second);
    }
    this->metadata = obj.serialize();
}

pitaya::Route::Route(const std::string& route_str)
{
    std::vector<std::string> strs;
    boost::split(strs, route_str, boost::is_any_of("."));
    if (strs.size() < 3) {
        throw PitayaException("error parsing route");
    }
    server_type = strs[0];
    handler = strs[1];
    method = strs[2];
};
