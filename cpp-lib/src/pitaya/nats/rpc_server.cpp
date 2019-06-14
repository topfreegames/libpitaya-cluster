#include "pitaya/nats/rpc_server.h"

#include "pitaya.h"
#include "pitaya/constants.h"
#include "pitaya/protos/request.pb.h"
#include "pitaya/protos/response.pb.h"
#include "pitaya/utils.h"

#include "spdlog/sinks/stdout_color_sinks.h"

#include "nats/nats.h"
#include <boost/format.hpp>
#include <cstdio>
#include <iostream>

using std::string;
using namespace pitaya;

static constexpr const char* kLogTag = "nats_rpc_server";

namespace pitaya {

struct NatsRpcServer::CallData : public pitaya::Rpc
{
    std::mutex mutex;
    NatsClient* natsClient;
    std::shared_ptr<NatsMsg> msg;
    utils::SyncVector<CallData*>* inProcessRpcs;
    std::shared_ptr<spdlog::logger> log;

    CallData(NatsClient* natsClient,
             std::shared_ptr<NatsMsg> msg,
             utils::SyncVector<CallData*>* inProcessRpcs,
             std::shared_ptr<spdlog::logger> log)
        : natsClient(natsClient)
        , msg(msg)
        , inProcessRpcs(inProcessRpcs)
        , log(log)
    {
        assert(this->natsClient);
        assert(this->msg);
        assert(this->inProcessRpcs);
        assert(this->log);
    }

    void Finish(protos::Response res) override
    {
        // NOTE: the whole code is indented here since the mutex
        // unlocked by lock guard has to be done before we call
        // 'delete this'.
        {
            std::lock_guard<decltype(mutex)> selfLock(mutex);

            // In process rpcs will be set to null whenever the server is
            // shutdown. Therefore we need to check it so that we know
            // the server is still valid.
            if (inProcessRpcs) {
                // If the server is still valid, lock the mutex
                // related with this rpc and finish it.
                std::lock_guard<decltype(*inProcessRpcs)> lock(*inProcessRpcs);

                std::vector<uint8_t> buf(res.ByteSizeLong());
                res.SerializeToArray(buf.data(), buf.size());

                NatsStatus status = natsClient->Publish(msg->GetReply(), std::move(buf));
                if (status != NatsStatus::Ok) {
                    log->error("Failed to publish RPC response");
                }

                // the RPC response was published, now remove it from the in process rpcs
                auto it = std::find(inProcessRpcs->begin(), inProcessRpcs->end(), this);
                if (it != inProcessRpcs->end()) {
                    inProcessRpcs->Erase(it);
                    log->debug("Decreasing number of in process rpcs: {}", inProcessRpcs->Size());
                }
            }
        }

        delete this;
    }
};

std::atomic_int NatsRpcServer::_cnt;

NatsRpcServer::NatsRpcServer(const Server& server, const NatsConfig& config, const char* loggerName)
    : NatsRpcServer(
          server,
          config,
          std::unique_ptr<NatsClient>(new NatsClientImpl(NatsApiType::Asynchronous, config, loggerName)),
          loggerName)
{}

NatsRpcServer::NatsRpcServer(const Server& server,
                             const NatsConfig& config,
                             std::unique_ptr<NatsClient> natsClient,
                             const char* loggerName)
    : _log(utils::CloneLoggerOrCreate(loggerName, kLogTag))
    , _config(config)
    , _natsClient(std::move(natsClient))
    , _server(server)
{}

NatsRpcServer::~NatsRpcServer()
{
    _log->info("Stopping rpc server");
    _log->flush();
}

void
NatsRpcServer::Start(RpcHandlerFunc handler)
{
    using std::placeholders::_1;
    _handlerFunc = handler;

    auto topic = utils::GetTopicForServer(_server.Id(), _server.Type());
    NatsStatus status =
        _natsClient->Subscribe(topic, std::bind(&NatsRpcServer::OnNewMessage, this, _1));

    if (status != NatsStatus::Ok) {
        throw PitayaException(
            fmt::format("Failed to create a new subscription at topic {}", topic));
    }

    _log->debug("Subscription at topic {} was created", topic);
    _log->info("Nats rpc server started!");
}

void
NatsRpcServer::Shutdown()
{
    assert(_handlerFunc);

    // Check if we need to wait for RPCs to finish.
    bool shouldWait = false;
    {
        std::lock_guard<decltype(_inProcessRpcs)> lock(_inProcessRpcs);
        if (_inProcessRpcs.Size() > 0) {
            // There are still rpcs being processed, so we signal
            // the thread to wait until the deadline.
            shouldWait = true;
        }
    }

    if (shouldWait) {
        std::this_thread::sleep_for(_config.serverShutdownDeadline);
    }

    // Invalidate RPCs that are still being processed
    std::lock_guard<decltype(_inProcessRpcs)> lock(_inProcessRpcs);
    if (_inProcessRpcs.Size() > 0) {
        _log->warn("Server is shutting down with {} rpcs being processed", _inProcessRpcs.Size());
        for (auto rpc : _inProcessRpcs) {
            std::lock_guard<decltype(rpc->mutex)> rpcLock(rpc->mutex);
            rpc->inProcessRpcs = nullptr;
        }
        _inProcessRpcs.Clear();
    }

    // Call the handler function signaling that no more RPCs will be called
    _handlerFunc(protos::Request(), nullptr);

    _log->info("Nats rpc server was shutdown!");
}

void
NatsRpcServer::OnNewMessage(std::shared_ptr<NatsMsg> msg)
{
    if (_cnt == 10000) {
        // TODO: print stuff
        // PrintSubStatus(sub);
        _cnt = 0;
    }
    _cnt++;

    auto req = protos::Request();
    bool decoded = req.ParseFromArray(msg->GetData(), msg->GetSize());
    if (!decoded) {
        _log->error("unable to decode msg from nats");
        return;
    }

    auto callData = new CallData(_natsClient.get(), msg, &_inProcessRpcs, _log);

    // Check wether the rpc should be processed or not.
    bool shouldProcessRpc;
    {
        std::lock_guard<decltype(_inProcessRpcs)> lock(_inProcessRpcs);
        if (_inProcessRpcs.Size() == _config.serverMaxNumberOfRpcs) {
            _log->debug("Will NOT process rpc");
            shouldProcessRpc = false;
        } else {
            shouldProcessRpc = true;
            _log->info("Saving new call data to queue: {}", (void*)callData);
            _inProcessRpcs.PushBack(callData);
        }
    }

    if (shouldProcessRpc) {
        _handlerFunc(req, callData);
    } else {
        auto error = new protos::Error();
        error->set_code(constants::kCodeServiceUnavailable);
        error->set_msg("The server is already processing the maximum amount of RPC's");

        protos::Response res;
        res.set_allocated_error(error);

        std::vector<uint8_t> buf(res.ByteSizeLong());
        res.SerializeToArray(buf.data(), buf.size());

        NatsStatus status = _natsClient->Publish(msg->GetReply(), std::move(buf));
        if (status != NatsStatus::Ok) {
            _log->error("Failed to publish RPC response");
        }
    }
}

void
NatsRpcServer::OnRpcFinished(const char* reply,
                             std::vector<uint8_t> responseBuf,
                             CallData* callData)
{
    NatsStatus status = _natsClient->Publish(reply, std::move(responseBuf));
    if (status != NatsStatus::Ok) {
        _log->error("Failed to publish RPC response");
    }

    // the RPC response was published, now remove it from the in process rpcs
    std::lock_guard<decltype(_inProcessRpcs)> lock(_inProcessRpcs);
    auto it = std::find(_inProcessRpcs.begin(), _inProcessRpcs.end(), callData);
    if (it != _inProcessRpcs.end()) {
        _inProcessRpcs.Erase(it);
        _log->debug("Decreasing number of in process rpcs: {}", _inProcessRpcs.Size());
    }
}

// void
// NatsRpcServer::PrintSubStatus(natsSubscription* subscription)
// {
//     int pendingMsgs;
//     int maxPendingMsgs;
//     int64_t deliveredMsgs;
//     int64_t droppedMsgs;
//     auto s = natsSubscription_GetStats(subscription,
//                                        &pendingMsgs,
//                                        nullptr,
//                                        &maxPendingMsgs,
//                                        nullptr,
//                                        &deliveredMsgs,
//                                        &droppedMsgs);
//     _log->debug("nats server status: pending:{}, max_pending:{}, delivered:{}, dropped:{}",
//                 pendingMsgs,
//                 maxPendingMsgs,
//                 deliveredMsgs,
//                 droppedMsgs);
// }

} // namespace pitaya
