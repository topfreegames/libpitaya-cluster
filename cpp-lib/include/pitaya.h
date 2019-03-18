#ifndef PITAYA_H
#define PITAYA_H

#include "protos/request.pb.h"
#include "protos/response.pb.h"

#include <functional>
#include <ostream>
#include <string>
#include <unordered_map>

namespace pitaya {

using RpcHandlerFunc = std::function<protos::Response(const protos::Request&)>;

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

    Route(const std::string& route_str);
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
           const std::unordered_map<std::string, std::string>& metadata,
           const std::string& hostname = "",
           bool frontend = false);

    Server(const std::string& id,
           const std::string& type,
           const std::string& metadata,
           const std::string& hostname = "",
           bool frontend = false)
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

    bool operator==(const Server& sv) const
    {
        return id == sv.id && type == sv.type && metadata == sv.metadata &&
               hostname == sv.hostname && frontend == sv.frontend;
    }
};

} // namespace pitaya

inline std::ostream&
operator<<(std::ostream& out, const pitaya::Server& s)
{
    if (out.good()) {
        out << "Server { id = " << s.id << ", type = " << s.type << ", metadata = " << s.metadata
            << ", hostname = " << s.hostname << ", frontend = " << (s.frontend ? "true" : "false")
            << " }";
    }
    return out;
}

#endif // PITAYA_H
