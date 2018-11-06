using System;
using Google.Protobuf;
using Pitaya;

class TestRemote : BaseRemote
{
    public Protos.RPCRes remote(Protos.RPCMsg msg)
    {
        var response = new Protos.RPCRes
        {
            Msg = $"hello from unity :) {System.Guid.NewGuid().ToString()}", Code = 200
        };
        Console.WriteLine("remote executed with arg {0}", msg);
        return response;
    }
}