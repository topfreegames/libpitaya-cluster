using System;
using System.Threading;
using Pitaya;
using Pitaya.Models;

namespace PitayaCSharpExample.Handlers
{
  class TestHandler : BaseHandler
  {
    public Protos.RPCRes entry(Session session, Protos.RPCMsg msg) {
      Protos.RPCRes response = new Protos.RPCRes();
      response.Msg = String.Format("hello from csharp handler!!! :) {0}", System.Guid.NewGuid().ToString());
      response.Code = 200;
      return response;
    }
    
    public void notifyBind(Session session, Protos.RPCMsg msg) {
      Protos.RPCRes response = new Protos.RPCRes();
      response.Msg = String.Format("hello from csharp handler!!! :) {0}", System.Guid.NewGuid().ToString());
      response.Code = 200;

      session.Bind("uidbla");

      Console.WriteLine("handler executed with arg {0}", msg);
      Console.WriteLine("handler executed with session ipversion {0}", session.GetString("ipversion"));
    }

    public void setSessionDataTest(Session session, Protos.RPCMsg msg)
    {
      session.Set("msg", "testingMsg");
      session.Set("int", 3);
      session.Set("double", 3.33);
      session.PushToFrontend();
    }

    public void testNotify(Session session)
    {
      Console.WriteLine("got empty notify");
    }
  }
}