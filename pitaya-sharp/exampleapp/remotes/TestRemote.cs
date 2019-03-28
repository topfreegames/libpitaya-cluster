using System.Threading.Tasks;
using NPitaya.Models;

namespace exampleapp.remotes
{
#pragma warning disable 1998
    class TestRemote : BaseRemote
    {
        public Protos.RPCRes Remote(Protos.RPCMsg msg) {
            var response = new Protos.RPCRes
            {
                Msg = $"hello from csharp :) {System.Guid.NewGuid().ToString()}", Code = 200
            };
            return response;
        }
    }
}