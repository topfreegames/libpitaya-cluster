package main

import (
	"fmt"
	"strings"
	"time"

	"github.com/spf13/viper"
	"github.com/topfreegames/pitaya/cluster"
	"github.com/topfreegames/pitaya/config"
	"github.com/topfreegames/pitaya/modules"
)

/*
#include "cluster.h"
*/
import "C"

//export SetRPCGrpc
func SetRPCGrpc(
	rpcClientConfig CGrpcRPCClientConfig,
	rpcServerConfig CGrpcRPCServerConfig,
) bool {
	if server == nil {
		log.Error("Server not initialized!")
		return false
	}

	cfg := viper.New()
	setRPCGrpcConfig(cfg, rpcClientConfig, rpcServerConfig)
	conf := config.NewConfig(cfg)

	if err := createGrpcRPCModules(conf, server, dieChan); err != nil {
		log.Error(err.Error())
		return false
	}

	return true
}

func setRPCGrpcConfig(
	cfg *viper.Viper,
	rpcClientConfig CGrpcRPCClientConfig,
	rpcServerConfig CGrpcRPCServerConfig,
) {
	// configure rpc client
	cfg.Set("pitaya.cluster.rpc.client.grpc.requesttimeout", time.Duration(int(rpcClientConfig.requestTimeoutMs))*time.Millisecond)
	cfg.Set("pitaya.cluster.rpc.client.grpc.dialtimeout", time.Duration(int(rpcClientConfig.dialTimeoutMs))*time.Millisecond)
	cfg.Set("pitaya.modules.bindingstorage.etcd.dialtimeout", time.Duration(int(rpcClientConfig.etcdDialTimeoutMs))*time.Millisecond)
	cfg.Set("pitaya.modules.bindingstorage.etcd.endpoints", strings.Split(C.GoString(rpcClientConfig.etcdEndpoints), ","))
	cfg.Set("pitaya.modules.bindingstorage.etcd.prefix", C.GoString(rpcClientConfig.etcdPrefix))
	cfg.Set("pitaya.modules.bindingstorage.etcd.leasettl", time.Duration(int(rpcClientConfig.etcdLeaseTTLS))*time.Second)

	// configure rpc server
	cfg.Set("pitaya.cluster.rpc.server.grpc.port", int(rpcServerConfig.port))
}

func createGrpcRPCModules(conf *config.Config, sv *cluster.Server, dieChan chan bool) error {
	var err error
	if sd == nil {
		return fmt.Errorf("Initialize service discovery before grpc!")
	}

	bs := modules.NewETCDBindingStorage(server, conf)
	rpcClient, err = cluster.NewGRPCClient(conf, sv, nil, bs)
	if err != nil {
		log.Error(err.Error())
		return err
	}

	rpcServer, err = cluster.NewGRPCServer(conf, sv, nil)
	if err != nil {
		log.Error(err.Error())
		return err
	}

	return bs.Init()
}
