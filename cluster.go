package main

import (
	"context"
	"fmt"
	"os"
	"reflect"
	"strings"
	"time"
	"unsafe"

	"github.com/gogo/protobuf/proto"
	"github.com/spf13/viper"
	"github.com/topfreegames/pitaya/cluster"
	"github.com/topfreegames/pitaya/config"
	"github.com/topfreegames/pitaya/errors"
	"github.com/topfreegames/pitaya/logger"
	"github.com/topfreegames/pitaya/protos"
	"github.com/topfreegames/pitaya/router"
	"github.com/topfreegames/pitaya/service"
	"github.com/topfreegames/pitaya/tracing"
	"github.com/topfreegames/pitaya/tracing/jaeger"
	"github.com/topfreegames/pitaya/util"
)

/*
#include "cluster.h"
*/
import "C"

var (
	server      *cluster.Server
	sd          cluster.ServiceDiscovery
	rpcClient   cluster.RPCClient
	rpcServer   cluster.RPCServer
	remote      *service.RemoteService
	initialized bool
	log         = logger.Log
)

func main() {}

//export GetServer
func GetServer(id string, res *CServer) bool {
	checkInitialized()
	gsv, err := sd.GetServer(id)
	if err != nil {
		log.Errorf("error getting server: %s", err.Error())
		return false
	}
	toCServer(gsv, res)
	return true
}

//export SendRPC
func SendRPC(svID string, route CRoute, msg []byte, ret *CRPCRes) bool {
	checkInitialized()
	r := fromCRoute(route)
	res, err := remote.DoRPC(context.Background(), svID, r, msg)
	if err != nil {
		log.Error(err.Error())
		return false
	}
	resBytes, err := proto.Marshal(res)
	if err != nil {
		log.Error(err.Error())
		return false
	}
	ret.data = C.CBytes(resBytes)
	ret.dataLen = C.int(int(len(resBytes)))
	return true
}

func checkInitialized() {
	if !initialized {
		panic("pitaya cluster module is not initialized, call Init first")
	}
}

func replyWithError(reply string, err error) {
	res := &protos.Response{
		Error: &protos.Error{
			Code: errors.ErrInternalCode,
			Msg:  err.Error(),
		},
	}
	data, _ := proto.Marshal(res)
	e := rpcClient.Send(reply, data)
	if e != nil {
		log.Errorf("failed to answer to rpc, err: %s\n", e.Error())
	}
}

func handleIncomingMessages(chMsg chan *protos.Request) {
	for msg := range chMsg {
		reply := msg.GetMsg().GetReply()
		ctx, err := util.GetContextFromRequest(msg, server.ID)
		if err != nil {
			log.Errorf("failed to get context from request: %s", err.Error())
			replyWithError(reply, err)
			continue
		}
		log.Debugf("processing incoming message with route: %s", msg.GetMsg().GetRoute())
		ptr := bridgeRPCCb(msg)
		data := *(*[]byte)(ptr)
		err = rpcClient.Send(reply, data)
		C.free((unsafe.Pointer)(ptr))
		if err != nil {
			log.Errorf("failed to answer to rpc, err: %s\n", err.Error())
		}
		tracing.FinishSpan(ctx, err)
	}
}

func getConfig(
	sdConfig CSDConfig,
	rpcClientConfig CNatsRPCClientConfig,
	rpcServerConfig CNatsRPCServerConfig,
) *config.Config {
	logHeartbeat := int(sdConfig.logHeartbeat) == 1
	cfg := viper.New()

	// configure service discovery
	cfg.Set("pitaya.cluster.sd.etcd.endpoints", strings.Split(C.GoString(sdConfig.endpoints), ","))
	cfg.Set("pitaya.cluster.sd.etcd.dialtimeout", time.Duration(int(sdConfig.etcdDialTimeoutSec))*time.Second)
	cfg.Set("pitaya.cluster.sd.etcd.prefix", C.GoString(sdConfig.etcdPrefix))
	cfg.Set("pitaya.cluster.sd.etcd.heartbeat.ttl", time.Duration(int(sdConfig.heartbeatTTLSec))*time.Second)
	cfg.Set("pitaya.cluster.sd.etcd.heartbeat.log", logHeartbeat)
	cfg.Set("pitaya.cluster.sd.etcd.syncservers.interval", time.Duration(int(sdConfig.syncServersIntervalSec))*time.Second)

	// configure rpc client
	cfg.Set("pitaya.cluster.rpc.client.nats.connect", C.GoString(rpcClientConfig.endpoint))
	cfg.Set("pitaya.cluster.rpc.client.nats.maxreconnectionretries", int(rpcClientConfig.maxConnectionRetries))
	cfg.Set("pitaya.cluster.rpc.client.nats.requesttimeout", time.Duration(int(rpcClientConfig.requestTimeoutMs))*time.Millisecond)

	// configure rpc server
	cfg.Set("pitaya.cluster.rpc.server.nats.connect", C.GoString(rpcServerConfig.endpoint))
	cfg.Set("pitaya.cluster.rpc.server.nats.maxreconnectionretries", int(rpcServerConfig.maxConnectionRetries))
	cfg.Set("pitaya.buffer.cluster.rpc.server.messages", int(rpcServerConfig.messagesBufferSize))
	// hack for stopping nats rpc server from processing messages
	cfg.Set("pitaya.concurrency.remote.service", 0)

	return config.NewConfig(cfg)
}

func createModules(conf *config.Config, sv *cluster.Server, dieChan chan bool) error {
	var err error
	sd, err = cluster.NewEtcdServiceDiscovery(conf, sv)
	if err != nil {
		log.Error(err.Error())
		return err
	}

	rpcClient, err = cluster.NewNatsRPCClient(conf, sv, nil, dieChan)
	if err != nil {
		log.Error(err.Error())
		return err
	}

	rpcServer, err = cluster.NewNatsRPCServer(conf, sv, nil, dieChan)
	if err != nil {
		log.Error(err.Error())
		return err
	}
	return nil
}

func initModules() error {
	err := sd.Init()
	if err != nil {
		log.Error(err.Error())
		return err
	}

	err = rpcClient.Init()
	if err != nil {
		log.Error(err.Error())
		return err
	}

	err = rpcServer.Init()
	if err != nil {
		log.Error(err.Error())
		return err
	}
	return nil
}

//export Shutdown
func Shutdown() bool {
	err := rpcServer.Shutdown()
	if err != nil {
		log.Error(err.Error())
		return false
	}
	err = rpcClient.Shutdown()
	if err != nil {
		log.Error(err.Error())
		return false
	}
	err = sd.Shutdown()
	if err != nil {
		log.Error(err.Error())
		return false
	}
	log.Info("pitaya-cluster go lib shutdown complete")
	return true
}

func wait(dieChan chan bool) {
	<-dieChan
	os.Exit(1)
}

func copyStr(s string) string {
	var b []byte
	h := (*reflect.SliceHeader)(unsafe.Pointer(&b))
	h.Data = (*reflect.StringHeader)(unsafe.Pointer(&s)).Data
	h.Len = len(s)
	h.Cap = len(s)
	return string(b)
}

//export ConfigureJaeger
func ConfigureJaeger(probability float64, serviceName string) {
	opts := jaeger.Options{
		Disabled:    false,
		Probability: probability,
		ServiceName: copyStr(serviceName),
	}

	_, err := jaeger.Configure(opts)
	log.Infof("jaeger activated with probability: %f and name: %s", probability, serviceName)
	if err != nil {
		log.Error("failed to configure jaeger")
	}
}

//export Init
func Init(
	sdConfig CSDConfig,
	rpcClientConfig CNatsRPCClientConfig,
	rpcServerConfig CNatsRPCServerConfig,
	sv CServer,
) bool {

	var err error
	server, err = fromCServer(sv)
	if err != nil {
		fmt.Println(err.Error())
		return false
	}

	conf := getConfig(sdConfig, rpcClientConfig, rpcServerConfig)

	dieChan := make(chan bool)

	err = createModules(conf, server, dieChan)
	if err != nil {
		log.Error(err.Error())
		return false
	}

	err = initModules()
	if err != nil {
		log.Error(err.Error())
		return false
	}

	for i := 0; i < int(rpcServerConfig.rpcHandleWorkerNum); i++ {
		log.Debug("started handle rpc routine")
		go handleIncomingMessages(rpcServer.(*cluster.NatsRPCServer).GetUnhandledRequestsChannel())
	}

	r := router.New()
	r.SetServiceDiscovery(sd)

	remote = service.NewRemoteService(
		rpcClient,
		nil,
		sd,
		nil,
		nil,
		r,
		nil,
		server,
	)

	initialized = true

	log.Info("go module initialized")

	go wait(dieChan)

	return true
}
