package main

import (
	"encoding/json"
	"reflect"
	"unsafe"

	"github.com/topfreegames/pitaya/cluster"
	"github.com/topfreegames/pitaya/protos"
	"github.com/topfreegames/pitaya/route"
)

/*
#include "cluster.h"
*/
import "C"

// CServer represents c server type
type CServer C.struct_Server

// CSDConfig is the C config for service discovery
type CSDConfig C.struct_SDConfig

// CNatsRPCClientConfig is the C nats rpc client config
type CNatsRPCClientConfig C.struct_NatsRPCClientConfig

// CNatsRPCServerConfig is the C nats rpc server config
type CNatsRPCServerConfig C.struct_NatsRPCServerConfig

// CRoute is the C route
type CRoute C.struct_Route

// CRPCMsg struct for representing rpc requests
type CRPCMsg C.struct_RPCMsg

// CRPCRes struct for sending rpc response to C
type CRPCRes C.struct_RPCRes

// CGetServerRes is returned by getServer
type CGetServerRes C.struct_GetServerRes

// CGetServersRes is returned by getServersByType
type CGetServersRes C.struct_GetServersRes

var rpcCbFunc C.rpcCbFunc

func bridgeRPCCb(req *protos.Request) unsafe.Pointer {
	data := req.GetMsg().GetData()
	route := req.GetMsg().GetRoute()

	r := C.struct_RPCReq{
		data:    C.CBytes(data),
		dataLen: C.int(int32(len(data))),
		route:   C.CString(route),
	}

	defer C.free(unsafe.Pointer(r.data))
	defer C.free(unsafe.Pointer(r.route))

	log.Debugf("calling cb func @ %p", rpcCbFunc)
	ptrRes := C.bridgeRPCFunc(rpcCbFunc, r)
	return unsafe.Pointer(ptrRes)
}

//export SetRPCCallback
func SetRPCCallback(funcPtr C.rpcCbFunc) {
	rpcCbFunc = funcPtr
}

func fromCRoute(cr CRoute) *route.Route {
	return route.NewRoute(C.GoString(cr.svType), C.GoString(cr.service), C.GoString(cr.method))
}

func fromCServer(csv CServer) (*cluster.Server, error) {
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

func fromCStringSliceToGoStringSlice(cSlice []*C.char) []string {
	res := make([]string, 0)
	for _, cStr := range cSlice {
		res = append(res, C.GoString(cStr))
	}
	return res
}

//export FreeServer
//remember that C.CString and C.CBytes allocs memory, we must free them
func FreeServer(sv *CServer) {
	C.free(unsafe.Pointer(sv.id))
	C.free(unsafe.Pointer(sv.svType))
	C.free(unsafe.Pointer(sv.metadata))
}

func goStringSliceFromCStringArray(arr **C.char, sz int) []string {
	var cCharSlice []*C.char
	slice := (*reflect.SliceHeader)(unsafe.Pointer(&cCharSlice))
	slice.Cap = sz
	slice.Len = sz
	slice.Data = uintptr(unsafe.Pointer(&arr))
	res := make([]string, 0)
	for _, cStr := range cCharSlice {
		res = append(res, C.GoString(cStr))
	}
	return res
}

// fromCArray receives an array from C and puts it into the Go slice pointed by s
func fromCArray(arr uintptr, sz int, s uintptr) {
	slice := (*reflect.SliceHeader)(unsafe.Pointer(s))
	slice.Cap = sz
	slice.Len = sz
	slice.Data = arr
}
