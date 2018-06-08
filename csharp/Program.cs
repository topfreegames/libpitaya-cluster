using System;
using System.Runtime.InteropServices;
using System.Text;

namespace Pitaya
{
  class Example 
  {
    public static void rpcCbWrapper(RPCReq req) {
      byte[] data = new byte[req.dataLen];
      Marshal.Copy(req.data, data, 0, req.dataLen);
      Console.WriteLine("called with route: " + req.route + " reply: " + req.replyTopic + " and bts: " + data[0]);
    }
    
    static void Main(string[] args)
    {
      // this line is necessary for sending an array of pointer structs from go to C#
      // disabling this check is hacky and I don't know the implications of it
      // read more https://golang.org/cmd/cgo/#hdr-Passing_pointers
      // if this doesnt work, run with GODEBUG=cgocheck=0 dotnet run
      string debugEnv = Environment.GetEnvironmentVariable("GODEBUG");
      if (String.IsNullOrEmpty(debugEnv)) {
        throw new Exception("pitaya-cluster lib require you to set env var GODEBUG=cgocheck=0");
      }
      // TODO y the fuck this doesnt work
      Environment.SetEnvironmentVariable("GODEBUG", "cgocheck=0");
      Console.WriteLine("c# prog running");

      SDConfig sdConfig = new SDConfig(new string[]{"127.0.0.1:2379"}, 30, "pitaya/", 30, true, 60);
      NatsRPCClientConfig rpcClientConfig = new NatsRPCClientConfig("nats://localhost:4222", 10, 5000);
      // TODO does it makes sense to give freedom to set reconnectionRetries and messagesBufferSize?
      NatsRPCServerConfig rpcServerConfig = new NatsRPCServerConfig("nats://localhost:4222", 10, 75);
      // TODO configure better
      // TODO if res fail, panic
      PitayaCluster.Init(sdConfig, rpcClientConfig, rpcServerConfig, new Server("someid", "game", "{\"ip\":\"127.0.0.1\"}", true));
      Console.ReadKey();

      Protos.TestMessage tm = new Protos.TestMessage();
      tm.Msg = "ola";
      tm.I = 1;
      Console.WriteLine("received tm " + tm);
      PitayaCluster.RPC(new Server("someid", "game", "", true), new Route("abc","abc", "abc"), tm);
      // prevent from closing
      Console.ReadKey();
      
      Console.ReadKey();
    }
  }
}
