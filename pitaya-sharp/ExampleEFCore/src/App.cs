using System;
using System.Linq;
using System.Security.Cryptography.X509Certificates;
using System.Threading;
using ExampleORM.Models;
using ExampleORM.Servers.BusinessLogic.Handlers;
using Microsoft.EntityFrameworkCore;
using Npgsql;
using NPitaya;
using NPitaya.Models;

namespace ExampleORM
{
    class App
    {
        static void Main(string[] args)
        {
            Logger.SetLevel(LogLevel.INFO);

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

            GrpcConfig grpcConfig = new GrpcConfig("127.0.0.1", 5444, 2);

            PitayaCluster.AddSignalHandler(() =>
            {
                Logger.Info("Calling terminate on cluster");
                PitayaCluster.Terminate();
                Logger.Info("Cluster terminated, exiting app");
                Environment.Exit(1);
            });
            
            PitayaCluster.RegisterHandler(new UserHandler());

            try
            {
                PitayaCluster.Initialize(grpcConfig, sdConfig, sv, NativeLogLevel.Debug, "");
            }
            catch (PitayaException exc)
            {
                Logger.Error("Failed to create cluster: {0}", exc.Message);
                Environment.Exit(1);
            }
            
            while (true)
            {
                Thread.Sleep(10);
            }
            ////
        }
    }
}