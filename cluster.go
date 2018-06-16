package main

import (
	"context"
	"fmt"
	"strings"
	"time"
	"unsafe"

	"github.com/gogo/protobuf/proto"
	"github.com/spf13/viper"
	"github.com/topfreegames/pitaya/cluster"
	"github.com/topfreegames/pitaya/config"
	"github.com/topfreegames/pitaya/logger"
	"github.com/topfreegames/pitaya/protos"
	"github.com/topfreegames/pitaya/router"
	"github.com/topfreegames/pitaya/service"
)

/*
#include "cluster.h"
*/
import "C"

var (
	sd          cluster.ServiceDiscovery
	rpcClient   cluster.RPCClient
	rpcServer   cluster.RPCServer
	remote      *service.RemoteService
	initialized bool
	log         = logger.Log
)

func main() {}

//export GetServer
func GetServer(id string) *CGetServerRes {
	checkInitialized()
	gsv, err := sd.GetServer(id)
	if err != nil {
		log.Errorf("error getting server: %s", err.Error())
		return &CGetServerRes{
			server:  (*_Ctype_struct_Server)(unsafe.Pointer(&CServer{})),
			success: 0,
		}
	}
	return &CGetServerRes{
		server:  (*_Ctype_struct_Server)(unsafe.Pointer(toCServer(gsv))),
		success: 1,
	}
}

//export GetServersByType
func GetServersByType(svType string) *CGetServersRes {
	checkInitialized()
	var res []*CServer
	servers, err := sd.GetServersByType(svType)
	if err != nil {
		log.Errorf("error getting servers by type: %s", err.Error())
		return &CGetServersRes{
			servers: nil,
			success: 0,
		}
	}
	for _, v := range servers {
		res = append(res, (*CServer)(unsafe.Pointer(toCServer(v))))
	}
	return &CGetServersRes{
		servers: unsafe.Pointer(&res),
		success: 1,
	}
}

// TODO put jaeger
//export SendRPC
func SendRPC(svID string, route CRoute, msg []byte) *CRPCRes {
	checkInitialized()
	r := fromCRoute(route)
	res, err := remote.DoRPC(context.Background(), svID, r, msg)
	if err != nil {
		log.Error(err.Error())
		return &CRPCRes{
			success: 0,
		}
	}
	resBytes, err := proto.Marshal(res)
	if err != nil {
		log.Error(err.Error())
		return &CRPCRes{
			success: 0,
		}
	}
	return &CRPCRes{
		data:    C.CBytes(resBytes),
		dataLen: C.int(int(len(resBytes))),
		success: 1,
	}
}

func checkInitialized() {
	if !initialized {
		panic("pitaya cluster module is not initialized, call Init first")
	}
}

func handleIncomingMessages(chMsg chan *protos.Request) {
	for msg := range chMsg {
		reply := msg.GetMsg().GetReply()
		log.Debugf("processing incoming message with route: %s", msg.GetMsg().GetRoute())
		ptr := bridgeRPCCb(msg)
		data := *(*[]byte)(ptr)
		err := rpcClient.Send(reply, data)
		C.free((unsafe.Pointer)(ptr))
		if err != nil {
			log.Errorf("failed to answer to rpc, err: %s\n", err.Error())
		}
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

	return config.NewConfig(cfg)
}

func createModules(conf *config.Config, sv *cluster.Server) error {
	var err error
	sd, err = cluster.NewEtcdServiceDiscovery(conf, sv)
	if err != nil {
		log.Error(err.Error())
		return err
	}

	rpcClient, err = cluster.NewNatsRPCClient(conf, sv, nil)
	if err != nil {
		log.Error(err.Error())
		return err
	}

	rpcServer, err = cluster.NewNatsRPCServer(conf, sv, nil)
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

//export Init
func Init(
	sdConfig CSDConfig,
	rpcClientConfig CNatsRPCClientConfig,
	rpcServerConfig CNatsRPCServerConfig,
	server CServer,
) bool {

	sv, err := fromCServer(server)
	if err != nil {
		fmt.Println(err.Error())
		return false
	}

	conf := getConfig(sdConfig, rpcClientConfig, rpcServerConfig)

	err = createModules(conf, sv)
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
		go handleIncomingMessages(rpcServer.GetUnhandledRequestsChannel())
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
		sv,
	)

	initialized = true

	log.Info("go module initialized")
	return true
}
