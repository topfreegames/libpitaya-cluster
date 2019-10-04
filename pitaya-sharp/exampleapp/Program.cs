using System;
using System.Collections.Generic;
using System.Threading;
using System.Threading.Tasks;
using exampleapp.Handlers;
using exampleapp.remotes;
using NPitaya;
using NPitaya.Metrics;
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
        serverTypeFilters: new List<string>(),
        heartbeatTTLSec: 60,
        logHeartbeat: true,
        logServerSync: true,
        logServerDetails: true,
        syncServersIntervalSec: 30,
        maxNumberOfRetries: 0);

      var sv = new Server(
          id: serverId,
          type: "csharp",
          metadata: "",
          hostname: "localhost",
          frontend: false);

      var natsConfig = new NatsConfig(
          endpoint: "127.0.0.1:4222",
          connectionTimeoutMs: 2000,
          requestTimeoutMs: 1000,
          serverShutdownDeadlineMs: 3,
          serverMaxNumberOfRpcs: 100,
          maxConnectionRetries: 3,
          maxPendingMessages: 1000);

      var grpcConfig = new GrpcConfig(
        host: "127.0.0.1",
        port: 5444,
        serverShutdownDeadlineMs: 2000,
        serverMaxNumberOfRpcs: 200,
        clientRpcTimeoutMs: 10000
      );

      Dictionary<string, string> constantTags = new Dictionary<string, string>
      {
          {"game", "game"},
          {"serverType", "svType"}
      };
      var statsdMR = new StatsdMetricsReporter("localhost", 5000, "game", constantTags);
      MetricsReporters.AddMetricReporter(statsdMR);
      var prometheusMR = new PrometheusMetricsReporter("default", "game", 9090);
      MetricsReporters.AddMetricReporter(prometheusMR);

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
        PitayaCluster.Initialize(
            grpcConfig,
            sdConfig,
            sv,
            NativeLogLevel.Debug,
            new PitayaCluster.ServiceDiscoveryListener((action, server) =>
            {
                switch (action)
                {
                    case PitayaCluster.ServiceDiscoveryAction.ServerAdded:
                        Console.WriteLine("Server was added");
                        Console.WriteLine("    id: " + server.id);
                        Console.WriteLine("  type: " + server.type);
                        break;
                    case PitayaCluster.ServiceDiscoveryAction.ServerRemoved:
                        Console.WriteLine("Server was removed");
                        Console.WriteLine("    id: " + server.id);
                        Console.WriteLine("  type: " + server.type);
                        break;
                    default:
                        throw new ArgumentOutOfRangeException(nameof(action), action, null);
                }
            }));
        //PitayaCluster.Initialize(natsConfig, sdConfig, sv, NativeLogLevel.Debug, "");
      }
      catch (PitayaException exc)
      {
        Logger.Error("Failed to create cluster: {0}", exc.Message);
        Environment.Exit(1);
      }

      Logger.Info("pitaya lib initialized successfully :)");

      var tr = new TestRemote();
      PitayaCluster.RegisterRemote(tr);
      var th = new TestHandler();
      PitayaCluster.RegisterHandler(th);

      Thread.Sleep(1000);

      TrySendRpc();
      Console.ReadKey();
      PitayaCluster.Terminate();
    }
    static async void TrySendRpc(){
        try
        {
            var res = await PitayaCluster.Rpc<NPitaya.Protos.RPCRes>(Route.FromString("csharp.testRemote.remote"),
                null);
            Console.WriteLine($"Code: {res.Code}");
            Console.WriteLine($"Msg: {res.Msg}");
        }
        catch (PitayaException e)
        {
            Logger.Error("Error sending RPC Call: {0}", e.Message);
        }
    }
  }
}
