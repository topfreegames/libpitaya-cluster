package main

import (
	"context"
	"fmt"

	"unsafe"

	"github.com/gogo/protobuf/proto"
	"github.com/topfreegames/pitaya/protos"
	"github.com/topfreegames/pitaya/util"

	"github.com/topfreegames/pitaya/tracing"
)

/*
#include "cluster.h"
*/
import "C"

type pitayaServer struct{}

func (r *pitayaServer) Call(ctx context.Context, msg *protos.Request) (*protos.Response, error) {
	c, err := util.GetContextFromRequest(msg, server.ID)
	if err != nil {
		log.Errorf("failed to get context from request: %s", err.Error())
		return nil, err
	}
	log.Debugf("processing incoming message with route: %s", msg.GetMsg().GetRoute())
	ptr := bridgeRPCCb(msg)
	rawdata := *(*[]byte)(ptr)
	data := make([]byte, len(rawdata))
	copy(data, rawdata)
	resp := &protos.Response{}
	err = proto.Unmarshal(data, resp)
	C.free((unsafe.Pointer)(ptr))

	defer tracing.FinishSpan(c, err)
	return resp, nil
}

func (r *pitayaServer) PushToUser(ctx context.Context, push *protos.Push) (*protos.Response, error) {
	return nil, fmt.Errorf("Not implemented")
}

func (r *pitayaServer) SessionBindRemote(ctx context.Context, bind *protos.BindMsg) (*protos.Response, error) {
	return nil, fmt.Errorf("Not implemented")
}

func (r *pitayaServer) KickUser(ctx context.Context, in *protos.KickMsg) (*protos.KickAnswer, error) {
	return nil, fmt.Errorf("Not implemented")
}
