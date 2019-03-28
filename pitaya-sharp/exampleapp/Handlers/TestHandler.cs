using System;
using System.Threading.Tasks;
using Google.Protobuf;
using NPitaya.Models;
using Protos;

#pragma warning disable 1998
namespace exampleapp.Handlers
{
    class TestHandler : BaseHandler
    {
        public RPCRes Entry(PitayaSession pitayaSession, Protos.RPCMsg msg) {
            var response = new Protos.RPCRes
            {
                Msg = $"hello from csharp handler!!! :) {System.Guid.NewGuid().ToString()}",
                Code = 200
            };
            return response;
        }
    
        public void NotifyBind(PitayaSession pitayaSession, Protos.RPCMsg msg) {
            var response = new Protos.RPCRes
            {
                Msg = $"hello from csharp handler!!! :) {System.Guid.NewGuid().ToString()}",
                Code = 200
            };

            pitayaSession.Bind("uidbla");

            Console.WriteLine("handler executed with arg {0}", msg);
            Console.WriteLine("handler executed with session ipversion {0}", pitayaSession.GetString("ipversion"));
        }

        public void SetSessionDataTest(PitayaSession pitayaSession, Protos.RPCMsg msg)
        {
            pitayaSession.Set("msg", "testingMsg");
            pitayaSession.Set("int", 3);
            pitayaSession.Set("double", 3.33);
            pitayaSession.PushToFrontend();
        }

        public void TestPush(PitayaSession pitayaSession)
        {
            Console.WriteLine("got empty notify");
            Push push = new Push
            {
                Uid = pitayaSession.Uid,
                Route = "test.route",
                Data = ByteString.CopyFromUtf8("teste felipe")
            };
            var ok = pitayaSession.Push(push);
            if (!ok)
            {
                Logger.Error("push to user failed!");
            }
        }
        public void TestKick(PitayaSession pitayaSession)
        {
            var ok = pitayaSession.Kick();
            if (!ok)
            {
                Logger.Error("kick user failed!");
            }
        }
    }
}