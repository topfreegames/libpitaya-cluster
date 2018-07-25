package main

import (
	"strings"
	"time"

	"github.com/spf13/viper"

	"github.com/topfreegames/pitaya/cluster"
	"github.com/topfreegames/pitaya/config"
)

/*
#include "cluster.h"
*/
import "C"

//export SetSDEtcd
func SetSDEtcd(sdConfig CSDConfig) bool {
	if server == nil {
		log.Error("Server not initialized!")
		return false
	}

	cfg := viper.New()
	setSdEtcdConfig(cfg, sdConfig)
	conf := config.NewConfig(cfg)

	if err := createEtcdSdModule(conf, server, dieChan); err != nil {
		log.Error(err.Error())
		return false
	}

	return true
}

func setSdEtcdConfig(
	cfg *viper.Viper,
	sdConfig CSDConfig,
) {
	logHeartbeat := int(sdConfig.logHeartbeat) == 1

	// configure service discovery
	cfg.Set("pitaya.cluster.sd.etcd.endpoints", strings.Split(C.GoString(sdConfig.endpoints), ","))
	cfg.Set("pitaya.cluster.sd.etcd.dialtimeout", time.Duration(int(sdConfig.etcdDialTimeoutSec))*time.Second)
	cfg.Set("pitaya.cluster.sd.etcd.prefix", C.GoString(sdConfig.etcdPrefix))
	cfg.Set("pitaya.cluster.sd.etcd.heartbeat.ttl", time.Duration(int(sdConfig.heartbeatTTLSec))*time.Second)
	cfg.Set("pitaya.cluster.sd.etcd.heartbeat.log", logHeartbeat)
	cfg.Set("pitaya.cluster.sd.etcd.syncservers.interval", time.Duration(int(sdConfig.syncServersIntervalSec))*time.Second)
}

func createEtcdSdModule(conf *config.Config, sv *cluster.Server, dieChan chan bool) error {
	var err error
	sd, err = cluster.NewEtcdServiceDiscovery(conf, sv)
	if err != nil {
		log.Error(err.Error())
		return err
	}

	return nil
}
