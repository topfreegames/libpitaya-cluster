using System;
using Pitaya;
using Pitaya.Models;

namespace PitayaCSharpExample
{
  [Remote]
  class TestRemote : BaseRemoteMethod
  {
    public Protos.RPCRes remote(Protos.RPCMsg msg) {
      Protos.RPCRes response = new Protos.RPCRes();
      response.Msg = String.Format("hello from csharp :) {0}", System.Guid.NewGuid().ToString());
      response.Code = 200;

      Console.WriteLine("remote executed with arg {0}", msg);
      return response;
    }
  }
}
