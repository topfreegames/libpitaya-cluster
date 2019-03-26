using System;
using System.Collections.Generic;
using System.IO;
using System.Reflection;
using System.Runtime.InteropServices;
using Google.Protobuf;
using Pitaya.Models;

namespace Pitaya.Utils {

    public static class Utils {

        public static string DefaultRemoteNameFunc(string methodName){
            var name = methodName;
            if (name != string.Empty && char.IsUpper(name[0]))
            {
                name = char.ToLower(name[0]) + name.Substring(1);
            }
            return name;
        } 
    
        public static IEnumerable<Type> GetTypesWithAttribute(Assembly assembly, Type attribute) {
            foreach(Type type in assembly.GetTypes()) {
                if (type.GetCustomAttributes(attribute, true).Length > 0) {
                    yield return type;
                }
            }
        }
    
        internal static Protos.Response GetErrorResponse(string code, string msg)
        {
            var response = new Protos.Response {Error = new Protos.Error {Code = code, Msg = msg}};
            return response;
        }

        internal static byte[] ProtoMessageToByteArray(IMessage msg)
        {
            if (msg == null)
            {
                return new byte[] { };
            }

            var mem = new MemoryStream();
            var o = new CodedOutputStream(mem);
            msg.WriteTo(o);
            o.Flush();
            mem.Close();
            return mem.ToArray();
        }

        internal static IntPtr ByteArrayToIntPtr(byte[] data)
        {
            IntPtr ptr = Marshal.AllocHGlobal(data.Length);
            Marshal.Copy(data, 0, ptr, data.Length);
            return ptr;
        }
    
        internal static T GetProtoMessageFromMemoryBuffer<T>(MemoryBuffer rpcRes)
        {
            byte[] resData = rpcRes.GetData();
            var response = new Protos.Response();
            response.MergeFrom(new CodedInputStream(resData));
            var res = (IMessage) Activator.CreateInstance(typeof(T));
            res.MergeFrom(response.Data);
            Logger.Debug("getProtoMsgFromResponse: got this res {0}", res);
            return (T) res;
        }
    }

}