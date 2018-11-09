#include "doctest.h"
#include "trompeloeil.hpp"
#include <memory>
#include <boost/optional.hpp>

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

    pitaya::Cluster cluster{ std::unique_ptr<ServiceDiscovery>(mockSd),
                             std::unique_ptr<RpcServer>(mockRpcSv),
                             std::unique_ptr<RpcClient>(mockRpcClient) };


    SUBCASE("RPCs can be done successfuly")
    {
        Server serverToReturn;
        serverToReturn.frontend = false;
        serverToReturn.hostname = "random-host";
        serverToReturn.id = "my-server-id";
        serverToReturn.type = "connector";

        REQUIRE_CALL(*mockSd, GetServerById("my-server-id")).RETURN(serverToReturn);

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

    SUBCASE("RPC fails when no server is found")
    {
        REQUIRE_CALL(*mockSd, GetServerById("my-server-id")).RETURN(boost::none);

        auto msg = new protos::Msg();
        msg->set_data("hello my friend");
        msg->set_route("someroute");

        protos::Request req;
        req.set_allocated_msg(msg);

        protos::Response res;

        optional<PitayaError> err = cluster.RPC("my-server-id", "mytest.route", req, res);

        CHECK(err.has_value());

        PitayaError pErr = err.value();
        CHECK(pErr.code == pitaya::kCodeNotFound);
    }

    SUBCASE("RPC returns error when the call fails")
    {
        Server serverToReturn;
        serverToReturn.frontend = false;
        serverToReturn.hostname = "random-host";
        serverToReturn.id = "my-server-id";
        serverToReturn.type = "connector";

        REQUIRE_CALL(*mockSd, GetServerById("my-server-id")).RETURN(serverToReturn);

        auto error = new protos::Error();
        error->set_allocated_code(new std::string(kCodeInternalError));
        error->set_allocated_msg(new std::string("Horrible error"));

        protos::Response resToReturn;
        resToReturn.set_allocated_error(error);

        REQUIRE_CALL(*mockRpcClient, Call(_, ANY(const protos::Request&))).RETURN(resToReturn);

        auto msg = new protos::Msg();
        msg->set_data("hello my friend");
        msg->set_route("someroute");

        protos::Request req;
        req.set_allocated_msg(msg);

        protos::Response res;

        optional<PitayaError> err = cluster.RPC("my-server-id", "mytest.route", req, res);

        CHECK(err.has_value());

        PitayaError pErr = err.value();
        CHECK(pErr.code == pitaya::kCodeInternalError);
        CHECK(pErr.msg == "Horrible error");
    }

    SUBCASE("RPC returns error if it cannot parse the protobuf message")
    {
        Server serverToReturn;
        serverToReturn.frontend = false;
        serverToReturn.hostname = "random-host";
        serverToReturn.id = "my-server-id";
        serverToReturn.type = "connector";

        REQUIRE_CALL(*mockSd, GetServerById("my-server-id")).RETURN(serverToReturn);

        auto error = new protos::Error();
        error->set_allocated_code(new std::string(kCodeInternalError));
        error->set_allocated_msg(new std::string("Horrible error"));

        protos::Response resToReturn;
        resToReturn.set_data("woiqdjoi2jd8398u320f9u23f");

        REQUIRE_CALL(*mockRpcClient, Call(_, ANY(const protos::Request&))).RETURN(resToReturn);

        auto msg = new protos::Msg();
        msg->set_data("hello my friend");
        msg->set_route("someroute");

        protos::Request req;
        req.set_allocated_msg(msg);

        protos::Response res;

        optional<PitayaError> err = cluster.RPC("my-server-id", "mytest.route", req, res);

        CHECK(err.has_value());
        CHECK(err.value().code == kCodeInternalError);
        CHECK(err.value().msg == "error parsing protobuf");
    }

    spdlog::drop_all();
}
