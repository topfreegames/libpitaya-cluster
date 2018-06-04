package main

/*
#include <stdlib.h>

struct Server {
    char* id;
    char* svType;
    char* metadata;
		int frontend; };
*/
import "C"

import (
	"encoding/json"
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

// Result is a sugar for int
type Result int32

// CServer represents c server type
type CServer C.struct_Server

const (
	// Ok success result
	Ok Result = iota
	// Fail failure result
	Fail
)

func main() {}

func fromCServer(csv C.struct_Server) (*cluster.Server, error) {
	mtd := []byte(C.GoString(csv.metadata))
	metadataMap := make(map[string]string)
	err := json.Unmarshal(mtd, &metadataMap)
	if err != nil {
		return nil, err
	}
	res := cluster.NewServer(C.GoString(csv.id), C.GoString(csv.svType), csv.frontend == 1, metadataMap)
	return res, nil
}

func toCServer(sv *cluster.Server) *CServer {
	frontend := 0
	if sv.Frontend {
		frontend = 1
	}
	meta, _ := json.Marshal(sv.Metadata)
	csv := &CServer{
		id:       C.CString(sv.ID),
		svType:   C.CString(sv.Type),
		frontend: C.int(frontend),
		metadata: C.CString(string(meta)),
	}
	return csv
}

//export FreeServer
//remember that C.CString and C.CBytes allocs memory, we must free them
func FreeServer(sv *C.struct_Server) {
	C.free(unsafe.Pointer(sv.id))
	C.free(unsafe.Pointer(sv.svType))
	C.free(unsafe.Pointer(sv.metadata))
}

//export GetServer
func GetServer(id string) (*C.struct_Server, Result) {
	gsv, err := sd.GetServer(id)
	if err != nil {
		return (*C.struct_Server)(unsafe.Pointer(&CServer{})), Fail
	}
	return (*C.struct_Server)(unsafe.Pointer(toCServer(gsv))), Ok
}

//export GetServersByType
func GetServersByType(svType string) (*[]*C.struct_Server, Result) {
	var res []*C.struct_Server
	servers, err := sd.GetServersByType(svType)
	if err != nil {
		return &res, Fail
	}
	for _, v := range servers {
		res = append(res, (*C.struct_Server)(unsafe.Pointer(toCServer(v))))
	}
	return &res, Ok
}

//export Init
func Init(
	endpoints []string,
	etcdDialTimeoutSec int32,
	etcdPrefix string,
	heartbeatTTLSec int32,
	logHeartbeat bool,
	syncServersIntervalSec int32,
	server C.struct_Server,
) Result {
	cfg := viper.New()
	cfg.Set("pitaya.cluster.sd.etcd.endpoints", endpoints)
	cfg.Set("pitaya.cluster.sd.etcd.dialtimeout", time.Duration(etcdDialTimeoutSec)*time.Second)
	cfg.Set("pitaya.cluster.sd.etcd.prefix", etcdPrefix)
	cfg.Set("pitaya.cluster.sd.etcd.heartbeat.ttl", time.Duration(heartbeatTTLSec)*time.Second)
	cfg.Set("pitaya.cluster.sd.etcd.heartbeat.log", logHeartbeat)
	cfg.Set("pitaya.cluster.sd.etcd.syncservers.interval", time.Duration(syncServersIntervalSec)*time.Second)

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
