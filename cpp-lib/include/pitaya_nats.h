#ifndef TFG_NATS_H
#define TFG_NATS_H

#include <string>
#include "nats/nats.h"
#include "pitaya.h"
#include "protos/response.pb.h"
#include "protos/request.pb.h"
#include "spdlog/spdlog.h"

namespace pitaya_nats
{
    struct NATSConfig
    {
        // TODO make more nats configs in both client and server
        // like buffer size, connection retries, etc
        std::string nats_addr;
        int64_t connection_timeout_ms;
        int request_timeout_ms;
        int max_reconnection_attempts;
        int max_pending_msgs;

        NATSConfig(
            const std::string &addr
            , const int request_timeout_ms
            , const int64_t connection_timeout_ms
            , const int max_reconnection_attempts
            , const int max_pending_msgs)
            : nats_addr(addr)
            , request_timeout_ms(request_timeout_ms)
            , connection_timeout_ms(connection_timeout_ms)
            , max_reconnection_attempts(max_reconnection_attempts)
            , max_pending_msgs(max_pending_msgs)
        {
        }
    };

    class NATSRPCServer
    {
    public:
        NATSRPCServer(
            std::shared_ptr<pitaya::Server> server,
            std::shared_ptr<NATSConfig> config,
            rpc_handler_func handler);

    private:
        void print_sub_status(natsSubscription* sub);
        std::shared_ptr<spdlog::logger> _log;
        static rpc_handler_func handler;
        natsConnection *nc = NULL;
        natsSubscription *sub = NULL;
        static void handle_msg(natsConnection *nc, natsSubscription *sub, natsMsg *msg, void *closure);
        static void err_handler(natsConnection *nc, natsSubscription *subscription, natsStatus err, void *closure);
        static void closed_cb(natsConnection *nc, void *closure); // called when all reconnection requests failed
        static void disconnected_cb(natsConnection *nc, void *closure); // called when the connection is lost
        static void reconnected_cb(natsConnection *nc, void *closure); // called when the connection is repaired
    };

    class NATSRPCClient
    {
    public:
        NATSRPCClient(std::shared_ptr<pitaya::Server> server, std::shared_ptr<NATSConfig> config);
        std::shared_ptr<protos::Response> Call(std::shared_ptr<pitaya::Server> target, std::unique_ptr<protos::Request> req);

    private:
        std::shared_ptr<spdlog::logger> _log;
        natsConnection *nc;
        natsSubscription *sub;
        int timeout_ms;
        static void closed_cb(natsConnection *nc, void *closure); // called when all reconnection requests failed
        static void disconnected_cb(natsConnection *nc, void *closure); // called when the connection is lost
        static void reconnected_cb(natsConnection *nc, void *closure); // called when the connection is repaired
    };
} // namespace pitaya_nats

#endif // TFG_NATS_H
