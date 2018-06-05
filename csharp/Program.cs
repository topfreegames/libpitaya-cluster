using System;
using System.Runtime.InteropServices;
using System.Text;

namespace Pitaya
{
  class Example 
  {
    static void Main(string[] args)
    {
      // this line is necessary for sending an array of pointer structs from go to C#
      // disabling this check is hacky and I don't know the implications of it
      // read more https://golang.org/cmd/cgo/#hdr-Passing_pointers
      // if this doesnt work, run with GODEBUG=cgocheck=0 dotnet run
      string debugEnv = Environment.GetEnvironmentVariable("GODEBUG");
      if (String.IsNullOrEmpty(debugEnv)) {
        throw new Exception("pitaya-cluster lib require you to set env var GODEBUG=cgocheck=0");
      }
      // TODO y the fuck this doesnt work
      Environment.SetEnvironmentVariable("GODEBUG", "cgocheck=0");
      Console.WriteLine("c# prog running");

      SDConfig sdConfig = new SDConfig(new string[]{"127.0.0.1:2379"}, 30, "pitaya/", 30, true, 60);
      int res = PitayaCluster.Init(sdConfig, new Server("someid", "game", "{\"ip\":\"127.0.0.1\"}", true));

      // prevent from closing
      Console.ReadKey();
    }
  }
}
