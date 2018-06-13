using System;
using System.Runtime.InteropServices;
using Google.Protobuf;
using Pitaya;

namespace PitayaCSharpExample
{
  class Example 
  {
    public static void rpcCbWrapper(RPCReq req) {
      byte[] data = new byte[req.dataLen];
      Marshal.Copy(req.data, data, 0, req.dataLen);
      Console.WriteLine("called with route: " + req.route + " and bts: " + data[0]);
    }
    
    static void Main(string[] args)
    {
      string debugEnv = Environment.GetEnvironmentVariable("GODEBUG");
      if (String.IsNullOrEmpty(debugEnv)) {
        throw new Exception("pitaya-cluster lib require you to set env var GODEBUG=cgocheck=0");
      }

      // this line is necessary for sending an array of pointer structs from go to C#
      // disabling this check is hacky and I don't know the implications of it
      // read more https://golang.org/cmd/cgo/#hdr-Passing_pointers
      // if this doesnt work, run with GODEBUG=cgocheck=0 dotnet run
      Logger.SetLevel(LogLevel.DEBUG);

      // TODO y the fuck this doesnt work
      Environment.SetEnvironmentVariable("GODEBUG", "cgocheck=0");
      Console.WriteLine("c# prog running");

      SDConfig sdConfig = new SDConfig(new string[]{"127.0.0.1:2379"}, 30, "pitaya/", 30, true, 60);
      NatsRPCClientConfig rpcClientConfig = new NatsRPCClientConfig("nats://localhost:4222", 10, 5000);
      // TODO does it makes sense to give freedom to set reconnectionRetries and messagesBufferSize?
      NatsRPCServerConfig rpcServerConfig = new NatsRPCServerConfig("nats://localhost:4222", 10, 75);
      PitayaCluster.Init(sdConfig, rpcClientConfig, rpcServerConfig, new Server("someid", "game", "{\"ip\":\"127.0.0.1\"}", true));
      Console.ReadKey();

      Server[] servers = PitayaCluster.GetServers("game");
      Console.WriteLine("ola server {0}", servers[0].id);

      TestRemote tr = new TestRemote();    
      PitayaCluster.RegisterRemote(tr);

      Protos.TestMessage tm = new Protos.TestMessage();
      tm.Msg = "ola";
      tm.I = 1;
      Protos.TestMessage res = PitayaCluster.RPC<Protos.TestMessage>(new Server("someid", "game", "", true), new Route("game","testremote", "validremote"), tm);
      Console.WriteLine("received res from c# rpc: {0}", res);
      // prevent from closing
      Console.ReadKey();
    }
  }
}
