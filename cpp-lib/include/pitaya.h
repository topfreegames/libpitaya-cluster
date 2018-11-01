#ifndef TFG_PITAYA_H
#define TFG_PITAYA_H

#include "protos/request.pb.h"
#include "protos/response.pb.h"
#include <boost/algorithm/string.hpp>
#import <string>

namespace pitaya {
using RPCHandlerFunc = std::function<protos::Response(protos::Request)>;

class PitayaException : public std::exception
{
public:
    PitayaException(const std::string& msg)
        : msg(msg)
    {}

    const char* what() const throw() { return msg.c_str(); }

private:
    const std::string msg;
};

struct Route
{
    std::string server_type;
    std::string handler;
    std::string method;

    Route(const std::string& sv_type, const std::string& handler, const std::string& method)
        : server_type(sv_type)
        , handler(handler)
        , method(method){};

    Route(const std::string& route_str)
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
};

struct Server
{
    std::string id;
    std::string type;
    std::string metadata;
    std::string hostname;
    bool frontend;

    Server() {}
    Server(const std::string& id,
           const std::string& type,
           const std::string& metadata,
           const std::string& hostname,
           bool frontend)
        : id(id)
        , type(type)
        , metadata(metadata)
        , hostname(hostname)
        , frontend(frontend)
    {}
    Server(const std::string& id, const std::string& type)
        : id(id)
        , type(type)
        , frontend(false)
    {}
};

} // namespace pitaya
#endif
