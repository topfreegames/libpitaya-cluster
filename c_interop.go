package main

import (
	"encoding/json"
	"reflect"
	"unsafe"

	"github.com/topfreegames/pitaya/cluster"
)

/*
#include <stdlib.h>

typedef int bool;

struct Server {
	char* id;
	char* svType;
	char* metadata;
	bool frontend; };
struct SDConfig {
	char** endpoints;
	int endpointsLen;
	int etcdDialTimeoutSec;
	char* etcdPrefix;
	int heartbeatTTLSec;
	bool logHeartbeat;
	int syncServersIntervalSec;};
*/
import "C"

// Result is a sugar for int
type Result int32

// CServer represents c server type
type CServer C.struct_Server

// CSDConfig is the C config for service discovery
type CSDConfig C.struct_SDConfig

const (
	// Ok success result
	Ok Result = iota
	// Fail failure result
	Fail
)

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
