#ifndef TFG_PITAYA_H
#define TFG_PITAYA_H

#import <string>
#include "protos/request.pb.h"
#include "protos/response.pb.h"

using ServerId = std::string;

namespace pitaya
{
using rpc_handler_func = std::function<std::shared_ptr<protos::Response>(std::unique_ptr<protos::Request>)>;

struct Server
{
    std::string id;
    std::string type;
    std::string metadata;
    std::string hostname;
    bool frontend;

    Server(){};
    Server(const std::string &id, const std::string &type):
    id(id),
    type(type){};

    std::string GetKey() const
    {
        // TODO: Add fmt library to improve this code.
        return std::string("servers") + std::string("/") + type + std::string("/") + id;
    }
};

struct RPCReq{
    const char * data;
    int data_len;
    const char * route;
};

class PitayaException: public std::exception {
public:
    PitayaException(const std::string &msg): msg(msg){}

    const char *what() const throw()
    {
        return msg.c_str();
    }
    const std::string msg;
};

std::string GetTopicForServer(std::shared_ptr<Server> server);

} // namespace pitaya
#endif
