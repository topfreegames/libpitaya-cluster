#include "doctest.h"
#include "trompeloeil.hpp"
#include <boost/optional.hpp>
#include <boost/optional/optional_io.hpp>
#include <memory>

#include "mock_rpc_client.h"
#include "mock_rpc_server.h"
#include "mock_service_discovery.h"
#include "pitaya/cluster.h"
#include "pitaya/constants.h"

extern template struct trompeloeil::reporter<trompeloeil::specialized>;

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
    Server server;
    server.frontend = false;
    server.id = "my-server-id";
    server.type = "connector";

    pitaya::Cluster::Instance().Initialize(server,
                                           std::unique_ptr<ServiceDiscovery>(mockSd),
                                           std::unique_ptr<RpcServer>(mockRpcSv),
                                           std::unique_ptr<RpcClient>(mockRpcClient));

    SUBCASE("RPCs can be done successfuly")
    {
        Server serverToReturn;
        serverToReturn.frontend = false;
        serverToReturn.hostname = "random-host";
        serverToReturn.id = "my-server-id";
        serverToReturn.type = "connector";

        trompeloeil::sequence seq;

        REQUIRE_CALL(*mockSd, GetServerById("my-server-id"))
            .RETURN(serverToReturn)
            .IN_SEQUENCE(seq);

        protos::Response internalRes;
        internalRes.set_data("ABACATE");

        protos::Response resToReturn;
        resToReturn.set_data(internalRes.SerializeAsString());

        REQUIRE_CALL(*mockRpcClient, Call(_, ANY(const protos::Request&)))
            .RETURN(resToReturn)
            .IN_SEQUENCE(seq);

        auto msg = new protos::Msg();
        msg->set_data("hello my friend");
        msg->set_route("someroute");

        protos::Request req;
        req.set_allocated_msg(msg);

        protos::Response res;

        optional<PitayaError> err =
            Cluster::Instance().RPC("my-server-id", "mytest.route", req, res);

        CHECK(!err);
    }

    SUBCASE("RPC fails when no server is found")
    {
        REQUIRE_CALL(*mockSd, GetServerById("my-server-id")).RETURN(boost::none);

        auto msg = new protos::Msg();
        msg->set_data("hello my friend");
        msg->set_route("someroute");

        protos::Request req;
        req.set_allocated_msg(msg);

        protos::Response res;

        optional<PitayaError> err =
            Cluster::Instance().RPC("my-server-id", "mytest.route", req, res);

        CHECK(err);

        PitayaError pErr = err.value();
        CHECK(pErr.code == constants::kCodeNotFound);
    }

    SUBCASE("RPC returns error when the call fails")
    {
        Server serverToReturn;
        serverToReturn.frontend = false;
        serverToReturn.hostname = "random-host";
        serverToReturn.id = "my-server-id";
        serverToReturn.type = "connector";

        trompeloeil::sequence seq;

        REQUIRE_CALL(*mockSd, GetServerById("my-server-id"))
            .RETURN(serverToReturn)
            .IN_SEQUENCE(seq);

        auto error = new protos::Error();
        error->set_allocated_code(new std::string(constants::kCodeInternalError));
        error->set_allocated_msg(new std::string("Horrible error"));

        protos::Response resToReturn;
        resToReturn.set_allocated_error(error);

        REQUIRE_CALL(*mockRpcClient, Call(_, ANY(const protos::Request&)))
            .RETURN(resToReturn)
            .IN_SEQUENCE(seq);

        auto msg = new protos::Msg();
        msg->set_data("hello my friend");
        msg->set_route("someroute");

        protos::Request req;
        req.set_allocated_msg(msg);

        protos::Response res;

        optional<PitayaError> err =
            Cluster::Instance().RPC("my-server-id", "mytest.route", req, res);

        CHECK(err);

        PitayaError pErr = err.value();
        CHECK(pErr.code == constants::kCodeInternalError);
        CHECK(pErr.msg == "Horrible error");
    }

    Cluster::Instance().Terminate();
    //    spdlog::drop_all();
}
