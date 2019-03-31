using System;
using System.Runtime.InteropServices;
using System.Threading.Tasks;
using Google.Protobuf;
using NPitaya.Models;
using NPitaya.Serializer;
using Protos;
using static NPitaya.Utils.Utils;

namespace NPitaya
{
    public partial class PitayaCluster
    {
        // TODO can delete this cb
        [UnmanagedFunctionPointer(CallingConvention.StdCall)]
        private delegate Task<IntPtr> RPCCb(IntPtr req);

        static readonly RPCCb RpcCbFunc = RPCCbFuncImpl;
        private static readonly IntPtr RpcCbFuncPtr = Marshal.GetFunctionPointerForDelegate(RpcCbFunc);
        
        private static async Task HandleIncomingRpc(IntPtr cRpcPtr)
        {
            try
            {
                var cRpc = (CRpc) Marshal.PtrToStructure(cRpcPtr, typeof(CRpc));
                var resPtr = await RPCCbFuncImpl(cRpc.reqBufferPtr);
                tfg_pitc_FinishRpcCall(resPtr, cRpc.tag);
            }
            catch (Exception e)
            {
                throw e; // TODO: Could handle this better, putting into a response and retuning
            }
            finally
            {
                // TODO hack freeing memory allocated in c++, may crash
                Marshal.FreeHGlobal(cRpcPtr);
            }
        }

        private static async Task<IntPtr> RPCCbFuncImpl(IntPtr bufferPtr)
        {
            var buffer = (MemoryBuffer) Marshal.PtrToStructure(bufferPtr, typeof(MemoryBuffer));
            try
            {
                Request req = new Request();
                req.MergeFrom(new CodedInputStream(buffer.GetData()));

                Response response;
                switch (req.Type)
                {
                    case RPCType.User:
                        response = await HandleRpc(req, RPCType.User);
                        break;
                    case RPCType.Sys:
                        response = await HandleRpc(req, RPCType.Sys);
                        break;
                    default:
                        //TODO hacky, freeing memory allocated in managed may crash
                        Marshal.FreeHGlobal(buffer.data);
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
            catch (Exception e)
            {
                throw e;
            }
            finally
            {
                //TODO hacky, freeing memory allocated in managed may crash
                Marshal.FreeHGlobal(buffer.data);
                Marshal.FreeHGlobal(bufferPtr);
            }
        }

        internal static async Task<Response> HandleRpc(Protos.Request req, RPCType type)
        {
            byte[] data = req.Msg.Data.ToByteArray();
            Route route = Route.FromString(req.Msg.Route);

            string handlerName = $"{route.service}.{route.method}";

            PitayaSession s = null;
            var response = new Response();

            RemoteMethod handler;
            if (type == RPCType.Sys)
            {
                s = new Models.PitayaSession(req.Session, req.FrontendID);
                if (!HandlersDict.ContainsKey(handlerName))
                {
                    response = GetErrorResponse("PIT-404",
                        $"remote/handler not found! remote/handler name: {handlerName}");
                    return response;
                }

                handler = HandlersDict[handlerName];
            }
            else
            {
                if (!RemotesDict.ContainsKey(handlerName))
                {
                    response = GetErrorResponse("PIT-404",
                        $"remote/handler not found! remote/handler name: {handlerName}");
                    return response;
                }

                handler = RemotesDict[handlerName];
            }

            try
            {
                Task ans;
                if (handler.ArgType != null)
                {
                    var arg = serializer.Unmarshal(data, handler.ArgType);
                    if (type == RPCType.Sys)
                        ans = handler.Method.Invoke(handler.Obj, new[] {s, arg}) as Task;
                    else
                        ans = handler.Method.Invoke(handler.Obj, new[] {arg}) as Task;
                }
                else
                {
                    if (type == RPCType.Sys)
                        ans = handler.Method.Invoke(handler.Obj, new object[] {s}) as Task;
                    else
                        ans = handler.Method.Invoke(handler.Obj, new object[] { }) as Task;
                }

                await ans;
                byte[] ansBytes;

                if (handler.ReturnType != typeof(void))
                {
                    ansBytes = SerializerUtils.SerializeOrRaw(ans.GetType().
                        GetProperty("Result")
                        ?.GetValue(ans), serializer);
                }
                else
                {
                    ansBytes = new byte[]{};
                }
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