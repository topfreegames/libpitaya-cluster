using System;
using System.Runtime.Serialization;
using System.Text;
using System.Threading.Tasks;
using NPitaya.Models;
using NPitaya.Protos;

#pragma warning disable 1998
namespace exampleapp.Handlers
{
    class TestHandler : BaseHandler
    {
        public async Task<RPCRes> Entry(PitayaSession pitayaSession, RPCMsg msg)
        {
            var response = new NPitaya.Protos.RPCRes
            {
                Msg = $"hello from csharp handler!!! :) {Guid.NewGuid().ToString()}",
                Code = 200
            };
            return response;
        }

        public async Task NotifyBind(PitayaSession pitayaSession, RPCMsg msg)
        {
            var response = new NPitaya.Protos.RPCRes
            {
                Msg = $"hello from csharp handler!!! :) {Guid.NewGuid().ToString()}",
                Code = 200
            };

            pitayaSession.Bind("uidbla");

            Console.WriteLine("handler executed with arg {0}", msg);
            Console.WriteLine("handler executed with session ipversion {0}", pitayaSession.GetString("ipversion"));
        }

        public async Task SetSessionDataTest(PitayaSession pitayaSession, RPCMsg msg)
        {
            pitayaSession.Set("msg", "testingMsg");
            pitayaSession.Set("int", 3);
            pitayaSession.Set("double", 3.33);
            pitayaSession.PushToFrontend();
        }

        public async Task TestPush(PitayaSession pitayaSession)
        {
            Console.WriteLine("got empty notify");
            var msg = Encoding.UTF8.GetBytes("test felipe");
            var ok = pitayaSession.Push(new RPCRes{Code = 200, Msg = "testFelipe"}, "test.route");
            if (!ok)
            {
                Logger.Error("push to user failed!");
            }
        }
        public async Task TestKick(PitayaSession pitayaSession)
        {
            var ok = pitayaSession.Kick();
            if (!ok)
            {
                Logger.Error("kick user failed!");
            }
        }

        [DataContract]
        public class TestClass
        {
            [DataMember(Name = "msg")]
            public string Msg;
            [DataMember(Name = "code")]
            public int Code;
        }

        public async Task<TestClass> OnlyValidWithJson(PitayaSession s, TestClass t)
        {
            return t;
        }
    }
}