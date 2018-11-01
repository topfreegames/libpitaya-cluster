using System;
using Pitaya;

namespace PitayaCSharpExample
{
  class Example
  {
    static void Main(string[] args)
    {
      Logger.SetLevel(LogLevel.DEBUG);

      Console.WriteLine("c# prog running");

      string serverId = System.Guid.NewGuid().ToString();

      SDConfig sdConfig = new SDConfig("127.0.0.1:2379", 30, "pitaya/", 30, true, 60);


      Server sv = new Server(
         serverId,
         "csharp",
         "{\"ip\":\"127.0.0.1\"}",
         "localhost",
         false);

      NatsConfig nc = new NatsConfig("127.0.0.1:4222", 2000, 1000, 3, 100);

      bool initStatus = PitayaCluster.Init(sdConfig, nc, sv);
      if (!initStatus)
      {
        throw new Exception("failed to initialize pitaya lib :/");
      }

      Logger.Info("pitaya lib initialized successfully :)");

      //      GrpcRPCClientConfig grpcClientConfig = new GrpcRPCClientConfig(5000, 5000, "127.0.0.1:2379", "pitaya/");
      //      GrpcRPCServerConfig grpcServerConfig = new GrpcRPCServerConfig(5340);
      //      PitayaCluster.Init(
      //        sdConfig,
      //        grpcClientConfig,
      //        grpcServerConfig,
      //        new Server(
      //          serverId,
      //          "csharp",
      //          "{\"ip\":\"127.0.0.1\",\"grpc-host\":\"127.0.0.1\",\"grpc-port\":\"5340\"}",
      //          false)
      //      );
      //
      //      PitayaCluster.ConfigureJaeger(1.0, GoString.fromString("test-svc"));
      //
      //      TestRemote tr = new TestRemote();
      //      PitayaCluster.RegisterRemote(tr);
      //
      //      // prevent from closing
      //      Console.ReadKey();
      //
      //      Server sv = PitayaCluster.GetServer(serverId);
      //      Logger.Info("got server with id: {0}", sv.id);
      //
      //      Protos.RPCMsg msg = new Protos.RPCMsg();
      //      msg.Msg = "hellow from bla";
      //
      //      try
      //      {
      //        Protos.RPCRes res = PitayaCluster.RPC<Protos.RPCRes>(Pitaya.Route.fromString("connector.testremote.test"), msg);
      //        Logger.Info("received rpc res {0}", res);
      //      }
      //      catch (Exception e)
      //      {
      //        Logger.Error("deu ruim: {0}", e);
      //      }
      //
      //      Console.ReadKey();
      //      PitayaCluster.Shutdown();
      //    }
      //  }
    }
  }
}