using System;
using System.Reflection;

namespace Pitaya.Models
{
  public class HandlerMethod
  {
    internal BaseHandler Obj { get; }

    internal MethodBase Method { get; }

    internal Type ReturnType { get; }

    internal Type ArgType { get; }

    public HandlerMethod(BaseHandler obj, MethodBase method, Type returnType, Type argType){
      Obj = obj;
      Method = method;
      ReturnType = returnType;
      ArgType = argType;
    }
  }
}