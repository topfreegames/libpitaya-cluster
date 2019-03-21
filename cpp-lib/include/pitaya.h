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

class Server
{
public:
    enum Kind
    {
        Backend = 0,
        Frontend = 1,
    };

    std::string Id() const { return _id; }
    std::string Type() const { return _type; }
    std::string Metadata() const { return _metadata; }
    std::string Hostname() const { return _hostname; }
    bool IsFrontend() const { return _frontend; }

    Server() = default;

    Server(Kind kind, std::string id, std::string type, std::string hostname = "")
        : _id(std::move(id))
        , _type(std::move(type))
        , _hostname(std::move(hostname))
        , _frontend(static_cast<int>(kind))
    {}

    void AddMetadata(const std::string& key, const std::string& val);
    Server& WithRawMetadata(std::string metadata)
    {
        _metadata = std::move(metadata);
        return *this;
    }

    bool operator==(const Server& sv) const
    {
        return _id == sv._id && _type == sv._type && _metadata == sv._metadata &&
               _hostname == sv._hostname && _frontend == sv._frontend;
    }

private:
    std::string _id;
    std::string _type;
    std::string _metadata;
    std::string _hostname;
    bool _frontend;
};

} // namespace pitaya

inline std::ostream&
operator<<(std::ostream& out, const pitaya::Server& s)
{
    if (out.good()) {
        out << "Server { id = " << s.Id() << ", type = " << s.Type()
            << ", metadata = " << s.Metadata() << ", hostname = " << s.Hostname()
            << ", frontend = " << (s.IsFrontend() ? "true" : "false") << " }";
    }
    return out;
}

#endif // PITAYA_H
