using System;
using Google.Protobuf;
using Pitaya;

namespace PitayaCSharpExample
{
  class TestRemote : BaseRemote
  {
    public string bla(string aaa, IMessage msg) {
      return "bla";
    }

    public Protos.TestMessage validRemote(Protos.TestMessage msg){
      Console.WriteLine("remote executed with arg {0}", msg);
      return msg;
    }
  }
}
