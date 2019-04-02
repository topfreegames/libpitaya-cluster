#include "pitaya/nats/rpc_client.h"

#include "pitaya.h"
#include "pitaya/constants.h"
#include "pitaya/nats/config.h"
#include "pitaya/protos/kick.pb.h"
#include "pitaya/protos/request.pb.h"
#include "pitaya/protos/response.pb.h"
#include "pitaya/utils.h"

#include "spdlog/sinks/stdout_color_sinks.h"

#include <iostream>
#include <string>

using std::string;
using namespace pitaya;
using boost::optional;

static constexpr const char* kLogTag = "nats_rpc_client";

namespace pitaya {

NatsRpcClient::NatsRpcClient(const NatsConfig& config, const char* loggerName)
    : NatsRpcClient(config, std::unique_ptr<NatsClient>(new NatsClientImpl(config)), loggerName)
{
}

NatsRpcClient::NatsRpcClient(const NatsConfig& config, std::unique_ptr<NatsClient> natsClient, const char* loggerName)
    : _log(loggerName ? spdlog::get(loggerName)->clone(kLogTag) : spdlog::stdout_color_mt(kLogTag))
    , _natsClient(std::move(natsClient))
    , _requestTimeout(config.requestTimeoutMs)
{
    _log->info("nats rpc client configured!");
}

NatsRpcClient::~NatsRpcClient()
{
    _log->info("Stopping rpc client");
    _log->flush();
    spdlog::drop(kLogTag);
}

protos::Response
NatsRpcClient::Call(const pitaya::Server& target, const protos::Request& req)
{
    auto topic = utils::GetTopicForServer(target.Id(), target.Type());

    std::vector<uint8_t> buffer(req.ByteSizeLong());
    req.SerializeToArray(buffer.data(), buffer.size());

    std::shared_ptr<NatsMsg> reply;
    NatsStatus status = _natsClient->Request(&reply, topic, buffer, _requestTimeout);

    protos::Response res;

    if (status != NatsStatus::Ok) {
        auto err = new protos::Error();
        if (status == NatsStatus::Timeout) {
            err->set_code(constants::kCodeTimeout);
            err->set_msg("nats timeout");
        } else {
            err->set_code(constants::kCodeInternalError);
            err->set_msg("nats error");
        }
        res.set_allocated_error(err);
    } else {
        res.ParseFromArray(reply->GetData(), reply->GetSize());
    }

    return res;
}

optional<PitayaError>
NatsRpcClient::SendKickToUser(const std::string& serverId,
                              const std::string& serverType,
                              const protos::KickMsg& kick)
{
    // NOTE: we ignore the server id, since it is not necessary to create the topic.
    (void)serverId;

    if (kick.userid().empty()) {
        return PitayaError(constants::kCodeInternalError, "Received an empty user id");
    }

    if (serverType.empty()) {
        return PitayaError(constants::kCodeInternalError,
                           "SendKickToUser received an empty server type");
    }

    std::string topic = utils::GetUserKickTopic(kick.userid(), serverType);

    std::vector<uint8_t> buffer(kick.ByteSizeLong());
    kick.SerializeToArray(buffer.data(), buffer.size());

    std::shared_ptr<NatsMsg> reply;
    NatsStatus status = _natsClient->Request(&reply, topic, buffer, _requestTimeout);

    optional<PitayaError> error;

    if (status != NatsStatus::Ok) {
        if (status == NatsStatus::Timeout) {
            error = PitayaError(constants::kCodeTimeout, "nats timeout");
        } else {
            error = PitayaError(constants::kCodeInternalError, "nats error");
        }
    } else {
        error = boost::none;
    }

    return error;
}

optional<PitayaError>
NatsRpcClient::SendPushToUser(const std::string& serverId,
                              const std::string& serverType,
                              const protos::Push& push)
{
    // NOTE: we ignore the server id, since it is not necessary to create the topic.
    (void)serverId;

    if (push.uid().empty()) {
        return PitayaError(constants::kCodeInternalError, "Received an empty user id");
    }

    if (serverType.empty()) {
        return PitayaError(constants::kCodeInternalError,
                           "SendKickToUser received an empty server type");
    }

    std::string topic = utils::GetUserMessagesTopic(push.uid(), serverType);

    std::vector<uint8_t> buffer(push.ByteSizeLong());
    push.SerializeToArray(buffer.data(), buffer.size());

    std::shared_ptr<NatsMsg> reply;
    NatsStatus status = _natsClient->Request(&reply, topic, buffer, _requestTimeout);

    optional<PitayaError> error;

    if (status != NatsStatus::Ok) {
        if (status == NatsStatus::Timeout) {
            error = PitayaError(constants::kCodeTimeout, "nats timeout");
        } else {
            error = PitayaError(constants::kCodeInternalError, "nats error");
        }
    } else {
        error = boost::none;
    }

    return error;
}

} // namespace pitaya
