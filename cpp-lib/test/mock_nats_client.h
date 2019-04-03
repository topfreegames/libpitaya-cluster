#ifndef PITAYA_MOCK_NATS_CLIENT_H
#define PITAYA_MOCK_NATS_CLIENT_H

#include "pitaya/nats_client.h"

#include <gmock/gmock.h>

class MockNatsClient : public pitaya::NatsClient
{
public:
    MOCK_METHOD4(Request,
                 pitaya::NatsStatus(std::shared_ptr<pitaya::NatsMsg>* natsMsg,
                                    const std::string& topic,
                                    const std::vector<uint8_t>& data,
                                    std::chrono::milliseconds timeout));
};

class MockNatsMsg : public pitaya::NatsMsg
{
public:
    MOCK_CONST_METHOD0(GetData, const uint8_t*());
    MOCK_CONST_METHOD0(GetSize, size_t());
};

#endif // PITAYA_MOCK_NATS_CLIENT_H
