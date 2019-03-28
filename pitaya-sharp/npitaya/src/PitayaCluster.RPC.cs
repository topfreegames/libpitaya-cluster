using System;
using System.Runtime.InteropServices;
using Google.Protobuf;
using NPitaya.Models;
using NPitaya.Serializer;
using Protos;
using static NPitaya.Utils.Utils;

namespace NPitaya
{
    public partial class PitayaCluster
    {
        [UnmanagedFunctionPointer(CallingConvention.StdCall)]
        private delegate IntPtr RPCCb(IntPtr req);
        static readonly RPCCb RpcCbFunc = RPCCbFunc;
        private static readonly IntPtr RpcCbFuncPtr = Marshal.GetFunctionPointerForDelegate(RpcCbFunc);
 
        private static IntPtr RPCCbFunc(IntPtr bufferPtr)
        {
            var buffer = (MemoryBuffer) Marshal.PtrToStructure(bufferPtr, typeof(MemoryBuffer));
            Request req = new Request();
            req.MergeFrom(new CodedInputStream(buffer.GetData()));

            Response response;
            switch (req.Type)
            {
                case RPCType.User:
                    response = HandleRpc(req, RPCType.User);
                    break;
                case RPCType.Sys:
                    response = HandleRpc(req, RPCType.Sys);
                    break;
                default:
                    throw new Exception($"invalid rpc type, argument:{req.Type}");
            }
      
            var res = new MemoryBuffer();
            IntPtr pnt = Marshal.AllocHGlobal(Marshal.SizeOf(res));

            byte[] responseBytes = response.ToByteArray();
            res.data = ByteArrayToIntPtr(responseBytes);
            res.size = responseBytes.Length;
            Marshal.StructureToPtr(res, pnt, false);
      
            return pnt;
        }

        internal static Response HandleRpc(Protos.Request req, RPCType type)
        {

            byte[] data = req.Msg.Data.ToByteArray();
            Route route = Route.FromString(req.Msg.Route);

            string handlerName = $"{route.service}.{route.method}";

            Models.PitayaSession s = null;
            var response = new Protos.Response();

            RemoteMethod handler;
            if (type == RPCType.Sys)
            {
                s = new Models.PitayaSession(req.Session, req.FrontendID);
                if (!HandlersDict.ContainsKey(handlerName))
                {
                    response = GetErrorResponse("PIT-404", $"remote/handler not found! remote/handler name: {handlerName}");
                    return response;
                }       
                handler = HandlersDict[handlerName];
            } else
            {
                if (!RemotesDict.ContainsKey(handlerName))
                {
                    response = GetErrorResponse("PIT-404", $"remote/handler not found! remote/handler name: {handlerName}");
                    return response;
                }       
                handler = RemotesDict[handlerName];
            }
      
            try
            {
                object ans;
                if (handler.ArgType != null)
                {
                    var arg = Activator.CreateInstance(handler.ArgType);
                    serializer.Unmarshal(data, ref arg);
                    if (type == RPCType.Sys)
                        ans = handler.Method.Invoke(handler.Obj, new object[]{s, arg});
                    else
                        ans = handler.Method.Invoke(handler.Obj, new object[]{arg});
                }
                else
                {
                    if (type == RPCType.Sys)
                        ans = handler.Method.Invoke(handler.Obj, new object[]{s});
                    else
                        ans = handler.Method.Invoke(handler.Obj, new object[]{});
                }
                byte[] ansBytes = SerializerUtils.SerializeOrRaw(ans, serializer);
                response.Data = ByteString.CopyFrom(ansBytes);
                return response;
            }
            catch (Exception e)
            {
                var innerMostException = e;
                while (innerMostException.InnerException != null)
                    innerMostException = innerMostException.InnerException;
                if (e.InnerException != null)
                    Logger.Error("Exception thrown in handler, error:{0}",
                        innerMostException.Message); // TODO externalize method and only print stacktrace when debug
                response = GetErrorResponse("PIT-500", innerMostException.Message);
                return response;
            }
        }
    }
}