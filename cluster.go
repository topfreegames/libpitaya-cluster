package main

import "C"

import (
	"fmt"
	"time"
	"unsafe"

	"github.com/spf13/viper"
	"github.com/topfreegames/pitaya/cluster"
	"github.com/topfreegames/pitaya/config"
)

var (
	sd cluster.ServiceDiscovery
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

//export Init
func Init(
	sdConfig CSDConfig,
	server CServer,
) Result {
	var cEndpoints []*C.char
	fromCArray(uintptr(unsafe.Pointer(&sdConfig.endpoints)), int(sdConfig.endpointsLen), uintptr(unsafe.Pointer(&cEndpoints)))
	endpoints := fromCStringSliceToGoStringSlice(cEndpoints)
	logHeartbeat := int(sdConfig.logHeartbeat) == 1
	cfg := viper.New()
	cfg.Set("pitaya.cluster.sd.etcd.endpoints", endpoints)
	cfg.Set("pitaya.cluster.sd.etcd.dialtimeout", time.Duration(int(sdConfig.etcdDialTimeoutSec))*time.Second)
	cfg.Set("pitaya.cluster.sd.etcd.prefix", C.GoString(sdConfig.etcdPrefix))
	cfg.Set("pitaya.cluster.sd.etcd.heartbeat.ttl", time.Duration(int(sdConfig.heartbeatTTLSec))*time.Second)
	cfg.Set("pitaya.cluster.sd.etcd.heartbeat.log", logHeartbeat)
	cfg.Set("pitaya.cluster.sd.etcd.syncservers.interval", time.Duration(int(sdConfig.syncServersIntervalSec))*time.Second)

	sv, err := fromCServer(server)
	if err != nil {
		fmt.Println(err.Error())
		return Fail
	}

	sd, err = cluster.NewEtcdServiceDiscovery(config.NewConfig(cfg), sv)
	if err != nil {
		fmt.Println(err.Error())
		return Fail
	}

	err = sd.Init()
	if err != nil {
		fmt.Println(err.Error())
		return Fail
	}

	return Ok
}
