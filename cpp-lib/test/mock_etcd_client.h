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
    MOCK_METHOD1(Watch, void(std::function<void(pitaya::WatchResponse)>));

    MOCK_METHOD0(LeaseKeepAlive, void());
    MOCK_METHOD0(StopLeaseKeepAlive, void());
};

#endif // PITAYA_MOCK_ETCD_CLIENT_H
