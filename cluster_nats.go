package main

import (
	"time"

	"github.com/spf13/viper"
	"github.com/topfreegames/pitaya/cluster"
	"github.com/topfreegames/pitaya/config"
)

/*
#include "cluster.h"
*/
import "C"

//export SetRPCNats
func SetRPCNats(
	rpcClientConfig CNatsRPCClientConfig,
	rpcServerConfig CNatsRPCServerConfig,
) bool {
	if server == nil {
		log.Error("Server not initialized!")
		return false
	}

	cfg := viper.New()
	setRPCNatsConfig(cfg, rpcClientConfig, rpcServerConfig)
	conf := config.NewConfig(cfg)

	if err := createNatsRPCModules(conf, server, dieChan); err != nil {
		log.Error(err.Error())
		return false
	}

	return true
}

func setRPCNatsConfig(
	cfg *viper.Viper,
	rpcClientConfig CNatsRPCClientConfig,
	rpcServerConfig CNatsRPCServerConfig,
) {
	// configure rpc client
	cfg.Set("pitaya.cluster.rpc.client.nats.connect", C.GoString(rpcClientConfig.endpoint))
	cfg.Set("pitaya.cluster.rpc.client.nats.maxreconnectionretries", int(rpcClientConfig.maxConnectionRetries))
	cfg.Set("pitaya.cluster.rpc.client.nats.requesttimeout", time.Duration(int(rpcClientConfig.requestTimeoutMs))*time.Millisecond)

	// configure rpc server
	cfg.Set("pitaya.cluster.rpc.server.nats.connect", C.GoString(rpcServerConfig.endpoint))
	cfg.Set("pitaya.cluster.rpc.server.nats.maxreconnectionretries", int(rpcServerConfig.maxConnectionRetries))
	cfg.Set("pitaya.buffer.cluster.rpc.server.messages", int(rpcServerConfig.messagesBufferSize))
	cfg.Set("pitaya.concurrency.remote.service", int(rpcServerConfig.rpcHandleWorkerNum))
}

func createNatsRPCModules(conf *config.Config, sv *cluster.Server, dieChan chan bool) error {
	var err error
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
