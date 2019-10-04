#include "test_common.h"

#include "pitaya.h"
#include "pitaya/constants.h"
#include "pitaya/etcd_config.h"
#include "pitaya/etcdv3_service_discovery.h"
#include "pitaya/utils.h"
#include "pitaya/utils/grpc.h"

#include "mock_etcd_client.h"

using namespace pitaya::constants;
using namespace pitaya::utils;
using namespace ::testing;

TEST(ParseEtcdKeyTest, ReturnsFalseWhenKeyIsInvalid)
{
    static struct
    {
        std::string key;
        std::string prefix;
    } arr[] = {
        { "sniper3d/servers/room/server-id", "myPrefix/" },
        { "sniper3d/servers/room/server-id", "myPrefix" },
        { "sniper3dserversroomserver-id", "myPrefix" },
        { "sniper3dserversroomserver-id", "myPrefix/" },
        { "myPrefix", "myPrefix/" },
        { "myPrefix/", "myPrefix/" },
        { "myPrefix/servers", "myPrefix/" },
        { "myPrefix/servers/mytype/", "myPrefix/" },
        { "myPrefix//mytype/", "myPrefix/" },
        { "m", "myPrefix/" },
        { "myPrefix//mytype/myid", "myPrefix/" },
        { "myPrefix/servers/mytype/myid", "myPrefix" },
        { "sniper3d/no-servers/room/server-id", "sniper3d/" },
        { "sniper3d/servers//server-id", "sniper3d/" },
        { "sniper3d/servers/room/", "sniper3d/" },
    };

    for (const auto& el : arr) {
        std::string serverType, serverId;
        bool ok = ParseEtcdKey(el.key, el.prefix, std::vector<std::string>(), serverType, serverId);
        ASSERT_FALSE(ok) << "Expecting false for key " << el.key << " and prefix " << el.prefix;
        EXPECT_EQ(serverType, "");
        EXPECT_EQ(serverId, "");
    }
}

TEST(ParseEtcdKeyTest, ReturnsTrueWhenKeyIsValid)
{
    static struct
    {
        std::string key;
        std::string prefix;
        std::string retType;
        std::string retId;
    } arr[] = {
        { "sniper3d/servers/room/server-id", "sniper3d/", "room", "server-id" },
        { "sniper3d/servers/tutu/baba", "sniper3d/", "tutu", "baba" },
        { "pit/servers/tutu/baba", "pit/", "tutu", "baba" },
    };

    for (const auto& el : arr) {
        std::string serverType, serverId;
        bool ok = ParseEtcdKey(el.key, el.prefix, std::vector<std::string>(), serverType, serverId);
        ASSERT_TRUE(ok);
        EXPECT_EQ(serverType, el.retType);
        EXPECT_EQ(serverId, el.retId);
    }
}

TEST(ParseEtcdKeyTest, CanFilterKeysByServerType)
{
    static struct
    {
        std::string key;
        std::string prefix;
        std::vector<std::string> filters;
        bool ok;
    } arr[] = {
        { "sniper3d/servers/server-type1/server-id", "sniper3d/", { "server-type1" }, true },
        { "sniper3d/servers/server-type2/server-id", "sniper3d/", { "server-type1" }, false },
        { "sniper3d/servers/server-type2/server-id", "sniper3d/", {}, true },
        { "sniper3d/servers/server-type3/server-id", "sniper3d/", { "server-type1", "server-type2" }, false },
        { "sniper3d/servers/server-type3/server-id", "sniper3d/", { "server-type1", "server-type3", "server-type2" }, true },
    };
    
    for (const auto& el : arr) {
        std::string serverType, serverId;
        bool ok = ParseEtcdKey(el.key, el.prefix, el.filters, serverType, serverId);
        EXPECT_EQ(ok, el.ok) << "ParseEtcdKey for key " << el.key << " should have returned " << el.ok;
    }
}

TEST(GetGrpcAddressFromServerTest, ThrowsOnFailure)
{
    pitaya::Server arr[] = {
        pitaya::Server(pitaya::Server::Kind::Backend, "id", "type"),
        pitaya::Server(pitaya::Server::Kind::Backend, "id", "type")
            .WithRawMetadata("{\"broken-json"),
        pitaya::Server(pitaya::Server::Kind::Backend, "id", "type")
            .WithMetadata(kGrpcHostKey, "random-host"),
        pitaya::Server(pitaya::Server::Kind::Backend, "id", "type")
            .WithMetadata(kGrpcPortKey, "random-port"),
        pitaya::Server(pitaya::Server::Kind::Backend, "id", "type").WithRawMetadata("[\"array\"]"),
    };

    for (const auto& server : arr) {
        EXPECT_THROW(GetGrpcAddressFromServer(server), pitaya::PitayaException);
    }
}

TEST(GetGrpcAddressFromServerTest, ReturnsTheFullAddress)
{
    struct
    {
        std::string host;
        std::string port;
        pitaya::Server server;
    } arr[] = {
        { "random-host",
          "3030",
          pitaya::Server(pitaya::Server::Kind::Backend, "id", "type")
              .WithMetadata(kGrpcHostKey, "random-host")
              .WithMetadata(kGrpcPortKey, "3030") },
        { "tututu",
          "3030",
          pitaya::Server(pitaya::Server::Kind::Backend, "id", "type")
              .WithMetadata(kGrpcHostKey, "tututu")
              .WithMetadata(kGrpcPortKey, "3030") },
    };

    for (const auto& entry : arr) {
        auto address = GetGrpcAddressFromServer(entry.server);
        EXPECT_EQ(address, entry.host + ":" + entry.port);
    }
}
