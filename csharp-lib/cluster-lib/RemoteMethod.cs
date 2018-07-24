using System;
using System.Reflection;

namespace Pitaya
{
  public class RemoteMethod
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
