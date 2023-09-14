#include "pitaya/utils/grpc.h"

#include "pitaya/constants.h"

#include "spdlog/fmt/fmt.h"

#include <cpprest/json.h>

namespace pitaya {
namespace utils {

std::string
GetGrpcAddressFromServer(const Server& server)
{
    if (server.Metadata() == "") {
        throw PitayaException(
            fmt::format("Ignoring server {}, since it does not support gRPC", server.Id()));
    }

    try {
        auto metadataJson = web::json::value::parse(server.Metadata());
        if (!metadataJson.is_object()) {
            throw PitayaException(
                fmt::format("Metadata from server is not a json object: {}", server.Metadata()));
        }

        auto metadataObj = metadataJson.as_object();

        auto hasHost = metadataObj.find(utility::conversions::to_string_t(constants::kGrpcHostKey)) != metadataObj.cend() &&
                       metadataObj[utility::conversions::to_string_t(constants::kGrpcHostKey)].as_string() != U("");

        if (!hasHost) {
            throw PitayaException("Did not receive a host on server metadata");
        }

        auto hasPort = metadataObj.find(utility::conversions::to_string_t(constants::kGrpcPortKey)) != metadataObj.cend() &&
                       metadataObj[utility::conversions::to_string_t(constants::kGrpcPortKey)].as_string() != U("");

        if (!hasPort) {
            throw PitayaException("Did not receive a port on server metadata");
        }

        auto host = metadataObj[utility::conversions::to_string_t(constants::kGrpcHostKey)].as_string();
        auto port = metadataObj[utility::conversions::to_string_t(constants::kGrpcPortKey)].as_string();

        return utility::conversions::to_utf8string(host + U(":") + port);
    } catch (const web::json::json_exception& exc) {
        throw PitayaException(
            fmt::format("Failed to parse metadata json from server: error = {}, json string = {}",
                        exc.what(),
                        server.Metadata()));
    }
}

} // namespace utils
} // namespace pitaya
