#ifndef TFG_NATS_H
#define TFG_NATS_H

#include <string>
#include "nats/nats.h"
#include "pitaya.h"

namespace pitaya_nats {
    struct NATSConfig
    {
        std::string nats_addr;
        int reconnection_retries;
    };

    class RPCServer{
        public:
            RPCServer(std::unique_ptr<pitaya::Server> server, const NATSConfig &config);

        private:
            natsConnection *nc = NULL;
            natsSubscription *sub = NULL;
            natsMsg *msg = NULL;
    };
} // namespace nats

#endif // TFG_NATS_H