using System;
using System.Threading.Tasks;
using Google.Protobuf;
using NPitaya.Models;
using Protos;
using Session = NPitaya.Models.Session;

#pragma warning disable 1998
namespace exampleapp.Handlers
{
    class TestHandler : BaseHandlerMethod
    {
        public RPCRes Entry(Session session, Protos.RPCMsg msg) {
            var response = new Protos.RPCRes
            {
                Msg = $"hello from csharp handler!!! :) {System.Guid.NewGuid().ToString()}",
                Code = 200
            };
            return response;
        }
    
        public void NotifyBind(Session session, Protos.RPCMsg msg) {
            var response = new Protos.RPCRes
            {
                Msg = $"hello from csharp handler!!! :) {System.Guid.NewGuid().ToString()}",
                Code = 200
            };

            session.Bind("uidbla");

            Console.WriteLine("handler executed with arg {0}", msg);
            Console.WriteLine("handler executed with session ipversion {0}", session.GetString("ipversion"));
        }

        public void SetSessionDataTest(Session session, Protos.RPCMsg msg)
        {
            session.Set("msg", "testingMsg");
            session.Set("int", 3);
            session.Set("double", 3.33);
            session.PushToFrontend();
        }

        public void TestPush(Session session)
        {
            Console.WriteLine("got empty notify");
            Push push = new Push
            {
                Uid = session.Uid,
                Route = "test.route",
                Data = ByteString.CopyFromUtf8("teste felipe")
            };
            var ok = session.Push(push);
            if (!ok)
            {
                Logger.Error("push to user failed!");
            }
        }
        public void TestKick(Session session)
        {
            var ok = session.Kick();
            if (!ok)
            {
                Logger.Error("kick user failed!");
            }
        }
    }
}