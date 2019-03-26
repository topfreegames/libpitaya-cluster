using System;
using System.Reflection;
using System.Reflection.Emit;
using System.Runtime.CompilerServices;
using pitaya.Models;

namespace Pitaya
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