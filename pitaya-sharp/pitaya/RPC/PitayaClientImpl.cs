using Grpc.Core;
using Protos;

namespace Pitaya
{
    public class PitayaClientImpl: Protos.Pitaya.PitayaClient
    {
        public override Response Call(Request request, CallOptions options)
        {
            return base.Call(request, options);
        }

        public override KickAnswer KickUser(KickMsg request, CallOptions options)
        {
            return base.KickUser(request, options);
        }

        public override Response PushToUser(Push request, CallOptions options)
        {
            return base.PushToUser(request, options);
        }

        public override Response SessionBindRemote(BindMsg request, CallOptions options)
        {
            return base.SessionBindRemote(request, options);
        }
    }
}