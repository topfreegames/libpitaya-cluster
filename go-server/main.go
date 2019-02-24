package main

import (
	"context"
	"flag"
	"fmt"

	"strings"

	"github.com/spf13/viper"
	"github.com/topfreegames/libpitaya-cluster/go-server/protos"
	"github.com/topfreegames/libpitaya-cluster/go-server/services"
	"github.com/topfreegames/pitaya"
	"github.com/topfreegames/pitaya/acceptor"
	"github.com/topfreegames/pitaya/component"
	"github.com/topfreegames/pitaya/modules"
	"github.com/topfreegames/pitaya/serialize/json"
	"github.com/topfreegames/pitaya/tracing/jaeger"
)

func configureJaeger(svc string) {
	opts := jaeger.Options{
		Disabled:    false,
		Probability: 1.0,
		ServiceName: svc,
	}

	_, err := jaeger.Configure(opts)
	if err != nil {
		fmt.Println("failed to configure jaeger")
	}
}

func configureFrontend(port int) {
	configureJaeger("connector")
	tcp := acceptor.NewTCPAcceptor(fmt.Sprintf(":%d", port))

	pitaya.Register(&services.Connector{},
		component.WithName("connector"),
		component.WithNameFunc(strings.ToLower),
	)
	pitaya.RegisterRemote(&services.ConnectorRemote{},
		component.WithName("connectorremote"),
		component.WithNameFunc(strings.ToLower),
	)

	pitaya.AddAcceptor(tcp)
}

// TestRemote remote
type TestRemote struct {
	component.Base
}

// Test is the test remote method
func (t *TestRemote) Test(ctx context.Context, msg *protos.RPCMsg) (*protos.RPCRes, error) {
	return &protos.RPCRes{
		Msg: fmt.Sprintf("ok! received: %s", msg.Msg),
	}, nil
}

func main() {
	port := flag.Int("port", 3250, "the port to listen")
	svType := "connector"

	flag.Parse()

	confs := viper.New()
	confs.Set("pitaya.cluster.rpc.server.grpc.port", 3939)

	meta := map[string]string{
		"grpc-host": "127.0.0.1",
		"grpc-port": "3939",
	}

	pitaya.Configure(true, svType, pitaya.Cluster, meta, confs)

	bs := modules.NewETCDBindingStorage(pitaya.GetServer(), pitaya.GetConfig())
	pitaya.RegisterModule(bs, "bindingsStorage")

	defer pitaya.Shutdown()

	pitaya.SetSerializer(json.NewSerializer())

	configureFrontend(*port)

	pitaya.RegisterRemote(
		&TestRemote{},
		component.WithName("testremote"),
		component.WithNameFunc(strings.ToLower),
	)
	pitaya.Start()
}
