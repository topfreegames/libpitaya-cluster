#include "pitaya.h"

#include <boost/algorithm/string.hpp>
#include <cpprest/json.h>

namespace json = web::json;

pitaya::Server&
pitaya::Server::AddMetadata(const std::string& key, const std::string& val)
{
    json::value metadataJson;
    if (_metadata.empty()) {
        metadataJson = json::value::object();
    } else {
        metadataJson = json::value::parse(_metadata);
    }

    if (!metadataJson.is_object()) {
        throw new PitayaException("Server metadata is not an object");
    }

    metadataJson[key] = json::value::string(val);

    _metadata = metadataJson.serialize();

    return *this;
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
