#include "doctest.h"
#include "trompeloeil.hpp"
#include <memory>

#include "mock_rpc_client.h"
#include "mock_rpc_server.h"
#include "mock_service_discovery.h"
#include "pitaya/cluster.h"

using namespace pitaya;
using boost::optional;
using pitaya::service_discovery::ServiceDiscovery;
using trompeloeil::_;

protos::Response RpcFunc(protos::Request)
{
    return protos::Response();
}

TEST_CASE("Cluster can be created normally")
{
    auto mockSd = new MockServiceDiscovery();
    auto mockRpcSv = new MockRpcServer(RpcFunc);
    auto mockRpcClient = new MockRpcClient();

    pitaya::Cluster cluster{ std::unique_ptr<ServiceDiscovery>(mockSd),
                             std::unique_ptr<RpcServer>(mockRpcSv),
                             std::unique_ptr<RpcClient>(mockRpcClient) };

    SUBCASE("RPCs can be done")
    {
        Server serverToReturn;
        serverToReturn.frontend = false;
        serverToReturn.hostname = "random-host";
        serverToReturn.id = "my-server-id";
        serverToReturn.type = "connector";

        REQUIRE_CALL(*mockSd, GetServerById(ANY(std::string))).RETURN(serverToReturn);

        protos::Response internalRes;
        internalRes.set_data("ABACATE");

        protos::Response resToReturn;
        resToReturn.set_data(internalRes.SerializeAsString());

        REQUIRE_CALL(*mockRpcClient, Call(_, ANY(const protos::Request&))).RETURN(resToReturn);

        auto msg = new protos::Msg();
        msg->set_data("hello my friend");
        msg->set_route("someroute");

        protos::Request req;
        req.set_allocated_msg(msg);

        protos::Response res;

        optional<PitayaError> err = cluster.RPC("my-server-id", "mytest.route", req, res);

        CHECK(!err.has_value());
    }
}
