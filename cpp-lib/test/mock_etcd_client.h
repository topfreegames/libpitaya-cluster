#ifndef PITAYA_MOCK_ETCD_CLIENT_H
#define PITAYA_MOCK_ETCD_CLIENT_H

#include "pitaya/etcd_client.h"

#include <chrono>
#include <gmock/gmock.h>
#include <stdlib.h>

class MockEtcdClient : public pitaya::EtcdClient
{
public:
    MOCK_METHOD1(LeaseGrant, pitaya::LeaseGrantResponse(std::chrono::seconds));
    MOCK_METHOD1(LeaseRevoke, pitaya::LeaseRevokeResponse(int64_t));
    MOCK_METHOD3(Set, pitaya::SetResponse(const std::string&, const std::string&, int64_t));
    MOCK_METHOD1(Get, pitaya::GetResponse(const std::string&));
    MOCK_METHOD1(List, pitaya::ListResponse(const std::string&));

    void Watch(std::function<void(pitaya::WatchResponse)> onWatch) override
    {
        this->onWatch = std::move(onWatch);
    }

    MOCK_METHOD0(CancelWatch, void());

    MOCK_METHOD2(LeaseKeepAlive,
                 void(int64_t, std::function<void(pitaya::EtcdLeaseKeepAliveStatus)>));
    MOCK_METHOD0(StopLeaseKeepAlive, void());

    std::function<void(pitaya::WatchResponse)> onWatch;
};

#endif // PITAYA_MOCK_ETCD_CLIENT_H
