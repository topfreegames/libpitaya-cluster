using System.Threading.Tasks;
using Pitaya.Models;

namespace exampleapp.remotes
{
#pragma warning disable 1998
    class TestRemote : BaseRemoteMethod
    {
        public async Task<Protos.RPCRes> Remote(Protos.RPCMsg msg) {
            var response = new Protos.RPCRes
            {
                Msg = $"hello from csharp :) {System.Guid.NewGuid().ToString()}", Code = 200
            };
            return response;
        }
    }
}