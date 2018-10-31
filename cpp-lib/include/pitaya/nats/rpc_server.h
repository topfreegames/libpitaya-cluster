#ifndef TFG_NATS_H
#define TFG_NATS_H

#include "nats/nats.h"
#include "pitaya.h"
#include "pitaya/nats/config.h"
#include "protos/request.pb.h"
#include "protos/response.pb.h"
#include "spdlog/spdlog.h"
#include <string>

namespace pitaya {
namespace nats {

class NATSRPCServer
{
public:
    NATSRPCServer(const pitaya::Server& server,
                  const NATSConfig& config,
                  pitaya::rpc_handler_func handler);

private:
    void print_sub_status(natsSubscription* sub);
    std::shared_ptr<spdlog::logger> _log;
    static pitaya::rpc_handler_func handler;
    natsConnection* nc = NULL;
    natsSubscription* sub = NULL;
    static void handle_msg(natsConnection* nc,
                           natsSubscription* sub,
                           natsMsg* msg,
                           void* closure);
    static void err_handler(natsConnection* nc,
                            natsSubscription* subscription,
                            natsStatus err,
                            void* closure);
    static void closed_cb(natsConnection* nc,
                          void* closure); // called when all reconnection requests failed
    static void disconnected_cb(natsConnection* nc,
                                void* closure); // called when the connection is lost
    static void reconnected_cb(natsConnection* nc,
                               void* closure); // called when the connection is repaired
};

class NATSRPCClient
{
public:
    NATSRPCClient(const pitaya::Server& server, const NATSConfig& config);
    std::shared_ptr<protos::Response> Call(const pitaya::Server& target,
                                           std::unique_ptr<protos::Request> req);

private:
    std::shared_ptr<spdlog::logger> _log;
    natsConnection* nc;
    natsSubscription* sub;
    int timeout_ms;
    static void closed_cb(natsConnection* nc,
                          void* closure); // called when all reconnection requests failed
    static void disconnected_cb(natsConnection* nc,
                                void* closure); // called when the connection is lost
    static void reconnected_cb(natsConnection* nc,
                               void* closure); // called when the connection is repaired
};

}
}

#endif // TFG_NATS_H
