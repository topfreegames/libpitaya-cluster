using System.Threading.Tasks;
using NPitaya.Models;

namespace exampleapp.remotes
{
#pragma warning disable 1998
    class TestRemote : BaseRemote
    {
        public async Task<NPitaya.Protos.RPCRes> Remote(NPitaya.Protos.RPCMsg msg)
        {
            var response = new NPitaya.Protos.RPCRes
            {
                Msg = $"hello from csharp :) {System.Guid.NewGuid().ToString()}", Code = 200
            };
            return response;
        }
    }
}