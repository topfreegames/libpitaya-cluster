#ifndef TFG_PITAYA_H
#define TFG_PITAYA_H

#import <string>

using ServerId = std::string;
using std::string;

namespace pitaya
{
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
    PitayaException(const string &msg): msg(msg){}

    virtual const char *what() const throw()
    {
        return msg.c_str();
    }
    const string msg;
};

string GetTopicForServer(std::shared_ptr<Server> server);

} // namespace pitaya
#endif
