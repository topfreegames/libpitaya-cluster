using System;
using System.Runtime.InteropServices;
using Google.Protobuf;
using Pitaya;

namespace PitayaCSharpExample
{
  class Example 
  {
    static void Main(string[] args)
    {
      // this line is necessary for sending an array of pointer structs from go to C#
      // disabling this check is hacky and I don't know the implications of it
      // read more https://golang.org/cmd/cgo/#hdr-Passing_pointers
      // if this doesnt work, run with GODEBUG=cgocheck=0 dotnet run
      Logger.SetLevel(LogLevel.DEBUG);

      // TODO y the fuck this doesnt work
      Console.WriteLine("c# prog running");

      SDConfig sdConfig = new SDConfig("127.0.0.1:2379", 30, "pitaya/", 30, true, 60);
      NatsRPCClientConfig rpcClientConfig = new NatsRPCClientConfig("nats://localhost:4222", 10, 5000);
      // TODO does it makes sense to give freedom to set reconnectionRetries and messagesBufferSize?
      NatsRPCServerConfig rpcServerConfig = new NatsRPCServerConfig("nats://localhost:4222", 10, 75);

      string serverId = System.Guid.NewGuid().ToString();

      PitayaCluster.Init(
        sdConfig,
        rpcClientConfig,
        rpcServerConfig,
        new Server(
          serverId,
          "csharp",
          "{\"ip\":\"127.0.0.1\"}",
          false)
      );

      TestRemote tr = new TestRemote();    
      PitayaCluster.RegisterRemote(tr);

      // prevent from closing
      Console.ReadKey();

      Server sv = PitayaCluster.GetServer(serverId);
      Logger.Info("got server with id: {0}", sv.id);

      Protos.RPCMsg msg = new Protos.RPCMsg();
      msg.Msg = "hellow from bla";

      try{
        Protos.RPCRes res = PitayaCluster.RPC<Protos.RPCRes>(Pitaya.Route.fromString("connector.testremote.test"), msg);
        Logger.Info("received rpc res {0}",res);
      }catch(Exception e){
        Logger.Error("deu ruim: {0}",e);
      }

      Console.ReadKey();
      PitayaCluster.Shutdown();
    }
  }
}
