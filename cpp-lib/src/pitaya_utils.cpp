#include "pitaya.h"
#include <boost/format.hpp>
#include <string>

using std::string;

string pitaya::GetTopicForServer(std::shared_ptr<pitaya::Server> server){
    return boost::str(boost::format("pitaya/servers/%1%/%2%") % 
        server->type % server->id);
}