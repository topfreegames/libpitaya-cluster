using System;
using System.Threading;
using exampleapp.Handlers;
using exampleapp.remotes;
using NPitaya;
using NPitaya.Models;

namespace PitayaCSharpExample
{
  class Example
  {
    static void Main(string[] args)
    {
      Logger.SetLevel(LogLevel.DEBUG);

      Console.WriteLine("c# prog running");

      string serverId = System.Guid.NewGuid().ToString();

      var sdConfig = new SDConfig(
        endpoints: "http://127.0.0.1:2379",
        etcdPrefix: "pitaya/",
        heartbeatTTLSec: 60,
        logHeartbeat: true,
        logServerSync: true,
        logServerDetails: true,
        syncServersIntervalSec: 30);

      var sv = new Server(
         id: serverId,
         type: "csharp",
         metadata: "",
         hostname: "localhost",
         frontend: false);

      NatsConfig natsConfig = new NatsConfig("127.0.0.1:4222", 2000, 1000, 3, 100);

      GrpcConfig grpcConfig = new GrpcConfig("127.0.0.1", 5444, 2);

      PitayaCluster cluster = null;

      PitayaCluster.AddSignalHandler(() =>
      {
        Logger.Info("Calling terminate on cluster");
        PitayaCluster.Terminate();
        Logger.Info("Cluster terminated, exiting app");
        Environment.Exit(1);
        //Environment.FailFast("oops");
      });

      try
      {
        PitayaCluster.Initialize(grpcConfig, sdConfig, sv, NativeLogLevel.Debug, "");
        //PitayaCluster.Initialize(natsConfig, sdConfig, sv, NativeLogLevel.Debug, "");
      }
      catch (PitayaException exc)
      {
        Logger.Error("Failed to create cluster: {0}", exc.Message);
        Environment.Exit(1);
      }

      Logger.Info("pitaya lib initialized successfully :)");

      TestRemote tr = new TestRemote();
      PitayaCluster.RegisterRemote(tr);
      TestHandler th = new TestHandler();
      PitayaCluster.RegisterHandler(th);

      System.Threading.Thread.Sleep(1000);

      while (true)
      {
        Thread.Sleep(10);
      }
     
      //try
      //{
      //  var res = PitayaCluster.Rpc<Protos.RPCRes>(Route.FromString("csharp.testremote.remote"), null);
      //  Console.WriteLine($"Code: {res.Code}");
      //  Console.WriteLine($"Msg: {res.Msg}");
      //}
      //catch (PitayaException exc)
      //{
      //  Console.WriteLine($"RPC failed: {exc.Message}");
      //}

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
      // PitayaCluster.Shutdown();
      //    }
      //  }
    }
  }
}
