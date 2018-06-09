using System;
using System.IO;
using System.Runtime.InteropServices;
using System.Reflection;
using System.Text;
using Google.Protobuf;

namespace Pitaya
{
  class RemoteMethod
  {
    public BaseRemote obj;
    public MethodBase method;
    public Type returnType;
    public Type argType;

    public RemoteMethod(BaseRemote obj, MethodBase method, Type returnType, Type argType){
      this.obj = obj;
      this.method = method;
      this.returnType = returnType;
      this.argType = argType;
    }
  }
}
