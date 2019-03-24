using System;
using System.Threading.Tasks;
using Grpc.Core;
using Protos;

namespace Pitaya
{
    public class PitayaServerImpl : Protos.Pitaya.PitayaBase
    {
        public override Task<Response> Call(Request req, ServerCallContext context)
        {
            switch (req.Type)
            {
              case RPCType.User:
                return Task.FromResult(PitayaCluster.HandleRpc(req, RPCType.User));
              case RPCType.Sys:
                return Task.FromResult(PitayaCluster.HandleRpc(req, RPCType.Sys));
              default:
                throw new Exception($"invalid rpc type, argument:{req.Type}");
            
            }
        }
    }
}