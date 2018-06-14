package services

import (
	"context"

	"github.com/topfreegames/pitaya"
	"github.com/topfreegames/pitaya-cluster/go-server/protos"
	"github.com/topfreegames/pitaya/component"
)

// ConnectorRemote is a remote that will receive rpc's
type ConnectorRemote struct {
	component.Base
}

// Connector struct
type Connector struct {
	component.Base
}

// HandleMsg struct
type HandleMsg struct {
	Route string `json:"route"`
	Msg   string `json:"msg"`
}

// SendRPC sends a rpc from a server to another
func (c *Connector) SendRPC(ctx context.Context, msg *HandleMsg) (*protos.RPCRes, error) {
	rpcRes := &protos.RPCRes{}
	err := pitaya.RPC(ctx, msg.Route, rpcRes, &protos.RPCMsg{
		Msg: msg.Msg,
	})
	if err != nil {
		return nil, err
	}
	return rpcRes, nil
}

// RemoteFunc is a function that will be called remotely
func (c *ConnectorRemote) RemoteFunc(ctx context.Context, msg *protos.RPCMsg) (*protos.RPCRes, error) {
	return &protos.RPCRes{
		Msg: msg.GetMsg(),
	}, nil
}
