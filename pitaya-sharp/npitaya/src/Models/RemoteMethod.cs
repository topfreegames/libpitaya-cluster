using System;
using System.Reflection;

namespace NPitaya.Models
{
    public class RemoteMethod
    {
        public readonly IBaseRemote Obj;
        internal MethodBase Method { get; }
        internal Type ReturnType { get; }
        internal Type ArgType { get; }
        public RemoteMethod(IBaseRemote obj, MethodBase method, Type returnType, Type argType){
            Obj = obj;
            Method = method;
            ReturnType = returnType;
            ArgType = argType;
        }
    }
}