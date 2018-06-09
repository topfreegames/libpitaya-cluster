using System;
using System.IO;
using System.Runtime.InteropServices;
using System.Reflection;
using System.Text;
using Google.Protobuf;

namespace Pitaya
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
