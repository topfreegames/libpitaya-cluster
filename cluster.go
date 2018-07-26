package main

import (
	"context"
	"os"
	"reflect"
	"unsafe"

	"github.com/gogo/protobuf/proto"
	"github.com/topfreegames/pitaya/cluster"
	"github.com/topfreegames/pitaya/errors"
	"github.com/topfreegames/pitaya/logger"
	"github.com/topfreegames/pitaya/protos"
	"github.com/topfreegames/pitaya/router"
	"github.com/topfreegames/pitaya/service"
	"github.com/topfreegames/pitaya/tracing/jaeger"
)

/*
#include "cluster.h"
*/
import "C"

var (
	initialized bool
	log         = logger.Log
	remote      *service.RemoteService
	rpcClient   cluster.RPCClient
	rpcServer   cluster.RPCServer
	pServer     *pitayaServer
	sd          cluster.ServiceDiscovery
	server      *cluster.Server
	dieChan     chan bool
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

func initModules() error {
	if err := sd.Init(); err != nil {
		return err
	}
	if err := rpcClient.Init(); err != nil {
		return err
	}
	if err := rpcServer.Init(); err != nil {
		return err
	}
	return nil
}

func afterInitModules() error {
	sd.AfterInit()
	rpcClient.AfterInit()
	rpcServer.AfterInit()
	return nil
}

func initDefault(
	sdConfig CSDConfig,
	rpcClientConfig CNatsRPCClientConfig,
	rpcServerConfig CNatsRPCServerConfig,
) bool {
	if ok := SetRPCNats(rpcClientConfig, rpcServerConfig); !ok {
		return false
	}

	if ok := SetSDEtcd(sdConfig); !ok {
		return false
	}

	return true
}

//export InitServer
func InitServer(sv CServer) bool {
	if server != nil {
		return true
	}

	dieChan = make(chan bool)
	var err error
	server, err = fromCServer(sv)
	if err != nil {
		log.Error(err.Error())
		return false
	}
	return true
}

//export StartDefault
func StartDefault(
	sdConfig CSDConfig,
	rpcClientConfig CNatsRPCClientConfig,
	rpcServerConfig CNatsRPCServerConfig,
	sv CServer,
) bool {
	if ok := InitServer(sv); !ok {
		return false
	}

	ok := initDefault(sdConfig, rpcClientConfig, rpcServerConfig)
	if !ok {
		return false
	}

	return Start()
}

//export Start
func Start() bool {
	if server == nil || rpcClient == nil || rpcServer == nil || sd == nil {
		log.Error("Server not initialized!")
		return false
	}

	pServer = &pitayaServer{}
	rpcServer.SetPitayaServer(pServer)

	if err := initModules(); err != nil {
		log.Error(err.Error())
		return false
	}

	if err := afterInitModules(); err != nil {
		log.Error(err.Error())
		return false
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

	go wait(dieChan)

	initialized = true
	log.Info("go module initialized")

	return true
}

func beforeShutdownModules() error {
	rpcServer.BeforeShutdown()
	rpcClient.BeforeShutdown()
	sd.BeforeShutdown()
	return nil
}

func shutdownModules() error {
	if err := rpcServer.Shutdown(); err != nil {
		return err
	}
	if err := rpcClient.Shutdown(); err != nil {
		return err
	}
	if err := sd.Shutdown(); err != nil {
		return err
	}
	return nil
}

//export Shutdown
func Shutdown() bool {
	if err := beforeShutdownModules(); err != nil {
		log.Error(err.Error())
		return false
	}
	if err := shutdownModules(); err != nil {
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
