using System;
using System.Runtime.InteropServices;

namespace NPitaya
{
    public partial class PitayaCluster
    {
        [DllImport("libpitaya_cluster", CallingConvention = CallingConvention.Cdecl, EntryPoint = "tfg_pitc_InitializeWithNats")]
        private static extern bool InitializeWithNatsInternal(IntPtr natsCfg, IntPtr sdCfg, IntPtr server, IntPtr cbPtr, IntPtr freePtr, NativeLogLevel logLevel, string logFile);

        [DllImport("libpitaya_cluster", CallingConvention = CallingConvention.Cdecl, EntryPoint = "tfg_pitc_InitializeWithGrpc")]
        private static extern bool InitializeWithGrpcInternal(IntPtr grpcCfg, IntPtr sdCfg, IntPtr server, IntPtr cbPtr, IntPtr freePtr, NativeLogLevel logLevel,  string logFile);

        [DllImport("libpitaya_cluster", CallingConvention = CallingConvention.Cdecl, EntryPoint = "tfg_pitc_Terminate")]
        private static extern void TerminateInternal();

        [DllImport("libpitaya_cluster", CallingConvention = CallingConvention.Cdecl, EntryPoint = "tfg_pitc_GetServerById")]
        private static extern bool GetServerByIdInternal(string serverId, ref Server retServer);

        [DllImport("libpitaya_cluster", CallingConvention = CallingConvention.Cdecl, EntryPoint = "tfg_pitc_FreeServer")]
        private static extern void FreeServerInternal(IntPtr serverPtr);

        [DllImport("libpitaya_cluster", CallingConvention = CallingConvention.Cdecl, EntryPoint = "tfg_pitc_RPC")]
        private static extern unsafe bool RPCInternal(string serverId, string route, IntPtr data, int dataSize, MemoryBuffer** buffer, ref Error retErr);

        [DllImport("libpitaya_cluster", CallingConvention = CallingConvention.Cdecl, EntryPoint = "tfg_pitc_FreeMemoryBuffer")]
        private static extern unsafe void FreeMemoryBufferInternal(MemoryBuffer *ptr);

        [DllImport("libpitaya_cluster", CallingConvention = CallingConvention.Cdecl, EntryPoint = "tfg_pitc_OnSignal")]
        private static extern void OnSignalInternal(OnSignalFunc ptr);

        [DllImport("libpitaya_cluster", CallingConvention = CallingConvention.Cdecl, EntryPoint = "tfg_pitc_FreePitayaError")]
        private static extern unsafe void FreePitayaErrorInternal(ref Error err);

        [DllImport("libpitaya_cluster", CallingConvention = CallingConvention.Cdecl, EntryPoint = "tfg_pitc_WaitForRpc")]
        private static extern IntPtr tfg_pitc_WaitForRpc();
    
        [DllImport("libpitaya_cluster", CallingConvention = CallingConvention.Cdecl, EntryPoint = "tfg_pitc_FinishRpcCall")]
        private static extern void tfg_pitc_FinishRpcCall(IntPtr responseMemoryBufferPtr, IntPtr tagPtr);
        
        [DllImport("libpitaya_cluster", CallingConvention = CallingConvention.Cdecl, EntryPoint = "tfg_pitc_SendPushToUser")]
        private static extern unsafe bool PushInternal(string serverId, string serverType, IntPtr pushData, MemoryBuffer** buffer, ref Error retErr);
    
        [DllImport("libpitaya_cluster", CallingConvention = CallingConvention.Cdecl, EntryPoint = "tfg_pitc_SendKickToUser")]
        private static extern unsafe bool KickInternal(string serverId, string serverType, IntPtr pushData, MemoryBuffer** buffer, ref Error retErr);       
    }
}