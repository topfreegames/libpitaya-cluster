package main

import (
	"context"
	"fmt"
	"time"
	"unsafe"

	"github.com/gogo/protobuf/proto"
	"github.com/spf13/viper"
	"github.com/topfreegames/pitaya/cluster"
	"github.com/topfreegames/pitaya/config"
	"github.com/topfreegames/pitaya/protos"
	"github.com/topfreegames/pitaya/router"
	"github.com/topfreegames/pitaya/service"
)

import "C"

// TODO ensure that its inited
var (
	sd        cluster.ServiceDiscovery
	rpcClient cluster.RPCClient
	rpcServer cluster.RPCServer
	remote    *service.RemoteService
)

func main() {}

//export GetServer
func GetServer(id string) (*CServer, Result) {
	gsv, err := sd.GetServer(id)
	if err != nil {
		return (*CServer)(unsafe.Pointer(&CServer{})), Fail
	}
	return (*CServer)(unsafe.Pointer(toCServer(gsv))), Ok
}

//export GetServersByType
func GetServersByType(svType string) (*[]*CServer, Result) {
	var res []*CServer
	servers, err := sd.GetServersByType(svType)
	if err != nil {
		return &res, Fail
	}
	for _, v := range servers {
		res = append(res, (*CServer)(unsafe.Pointer(toCServer(v))))
	}
	fmt.Printf("from go im returning %s\n", res)
	return &res, Ok
}

// TODO put jaeger
//export SendRPC
func SendRPC(svId string, route CRoute, msg []byte) (*[]byte, Result) {
	r := fromCRoute(route)
	fmt.Printf("rpc to svId:%s, route: %s, msg: %v\n", svId, r, msg)
	res, err := remote.DoRPC(context.Background(), svId, r, msg)
	// TODO return error msg?
	var arr []byte
	if err != nil {
		fmt.Printf("rpc failed: %s\n", err.Error())
		return &arr, Fail
	}
	fmt.Printf("res from go rpc: %s\n", res)
	resBytes, err := proto.Marshal(res)
	if err != nil {
		fmt.Printf("serialization failed: %s\n", err.Error())
		return &arr, Fail
	}
	return &resBytes, Ok
}

func handleIncomingMessages(chMsg chan *protos.Request) {
	for msg := range chMsg {
		bridgeRPCCb(msg)
	}
}

//export Init
func Init(
	sdConfig CSDConfig,
	rpcClientConfig CNatsRPCClientConfig,
	rpcServerConfig CNatsRPCServerConfig,
	server CServer,
) Result {
	var cEndpoints []*C.char
	fromCArray(uintptr(unsafe.Pointer(&sdConfig.endpoints)), int(sdConfig.endpointsLen), uintptr(unsafe.Pointer(&cEndpoints)))
	endpoints := fromCStringSliceToGoStringSlice(cEndpoints)
	logHeartbeat := int(sdConfig.logHeartbeat) == 1
	cfg := viper.New()

	// configure service discovery
	cfg.Set("pitaya.cluster.sd.etcd.endpoints", endpoints)
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

	sv, err := fromCServer(server)
	if err != nil {
		fmt.Println(err.Error())
		return Fail
	}

	conf := config.NewConfig(cfg)

	sd, err = cluster.NewEtcdServiceDiscovery(conf, sv)
	if err != nil {
		fmt.Println(err.Error())
		return Fail
	}

	rpcClient, err = cluster.NewNatsRPCClient(conf, sv, nil)
	if err != nil {
		fmt.Println(err.Error())
		return Fail
	}

	rpcServer, err = cluster.NewNatsRPCServer(conf, sv, nil)
	if err != nil {
		fmt.Println(err.Error())
		return Fail
	}

	//TODO use modules? init better

	err = sd.Init()
	if err != nil {
		fmt.Println(err.Error())
		return Fail
	}

	err = rpcClient.Init()
	if err != nil {
		fmt.Println(err.Error())
		return Fail
	}

	err = rpcServer.Init()
	if err != nil {
		fmt.Println(err.Error())
		return Fail
	}

	// TODO get chan handle rpc
	// TODO concurrently? many goroutines?
	go handleIncomingMessages(rpcServer.GetUnhandledRequestsChannel())

	remote = service.NewRemoteService(
		rpcClient,
		nil,
		sd,
		nil,
		nil,
		router.New(),
		nil,
		sv,
	)

	return Ok
}
