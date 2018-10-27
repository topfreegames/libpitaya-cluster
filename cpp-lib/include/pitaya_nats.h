#ifndef TFG_NATS_H
#define TFG_NATS_H

#include <string>
#include "nats/nats.h"
#include "pitaya.h"
#include "protos/response.pb.h"

namespace pitaya_nats {
    struct NATSConfig
    {
        // TODO make more nats configs in both client and server
        // like buffer size, connection retries, etc
        std::string nats_addr;
        int reconnection_retries;
        int request_timeout_ms;

        NATSConfig(const std::string &addr, const int &request_timeout_ms): 
        nats_addr(addr),
        request_timeout_ms(request_timeout_ms) {};
    };

    class NATSRPCServer{
        public:
            NATSRPCServer(std::shared_ptr<pitaya::Server> server, std::shared_ptr<NATSConfig> config);

        private:
            natsConnection *nc = NULL;
            natsSubscription *sub = NULL;
            static void handleMsg(natsConnection *nc, natsSubscription *sub, natsMsg *msg, void *closure);
    };

    class NATSRPCClient{
        public:
            NATSRPCClient(std::shared_ptr<pitaya::Server> server, std::shared_ptr<NATSConfig> config);
            std::shared_ptr<protos::Response> Call(std::shared_ptr<pitaya::Server> target, const string &route, const char * msg, const int msg_size);

        private:
            natsConnection *nc = NULL;
            natsSubscription *sub = NULL;
            int timeout_ms;
    };
} // namespace nats

#endif // TFG_NATS_H