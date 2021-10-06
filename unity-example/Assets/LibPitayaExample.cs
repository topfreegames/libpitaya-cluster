using UnityEngine;
using UnityEngine.UI;
using System;
using System.Collections.Generic;
using AOT;
using NPitaya;
using NPitaya.Models;

public class LibPitayaExample : MonoBehaviour {

	public Button initButton;

	public Button sendRPCButton;

	public InputField inputRPC;

	[MonoPInvokeCallback(typeof(Action<string>))]
	private static void LogFunction(string msg)
	{
		//Debug.Log(msg);
	}

    private void Init()
    {
	    Debug.Log("Init button clicked!");

        NPitaya.Models.Logger.SetLevel(LogLevel.DEBUG);
        Console.WriteLine("c# prog running");

	    string serverId = Guid.NewGuid().ToString();

	    var sdConfig = new SDConfig(
		    endpoints: "http://10.0.21.167:2379",
		    etcdPrefix: "pitaya/",
		    serverTypeFilters: new List<string>(), 
		    heartbeatTTLSec: 60,
		    logHeartbeat: false,
		    logServerSync: true,
		    logServerDetails: false,
		    syncServersIntervalSec: 60,
		    maxNumberOfRetries: 10,
		    retryDelayMilliseconds: 100);

        var grpcConfig = new GrpcConfig(
            host: "127.0.0.1",
            port: 3000,
            serverShutdownDeadlineMs: 3000,
            serverMaxNumberOfRpcs: 1000,
            clientRpcTimeoutMs: 4000
        );

	    var sv = new Server(
		    serverId,
		    "csharp",
		    "{\"ip\":\"127.0.0.1\"}",
		    "localhost",
		    false);

	    var nc = new NatsConfig(
		    endpoint: "127.0.0.1:4222",
		    connectionTimeoutMs: 2000,
		    requestTimeoutMs: 2000,
		    serverShutdownDeadlineMs: 4000,
		    serverMaxNumberOfRpcs: 1000,
		    maxConnectionRetries: 10,
		    maxPendingMessages: 100,
          	reconnectBufSize: 4 * 1024 * 1024);

	    Debug.Log("Adding signal handler");
	    Debug.Log("Adding signal handler DONE");

	    try
	    {
		    Debug.Log("Initializing PitayaCluster");
		    // PitayaCluster.Initialize(sdConfig, nc, sv);

		    var listener = new PitayaCluster.ServiceDiscoveryListener((action, server) =>
		    {
			    switch (action)
			    {
				    case PitayaCluster.ServiceDiscoveryAction.ServerAdded:
					    Debug.Log("Server added:");
					    Debug.Log("    id:  " + server.id);
					    Debug.Log("    type:" + server.type);
					    break;
				    case PitayaCluster.ServiceDiscoveryAction.ServerRemoved:
					    Debug.Log("Server removed:");
					    Debug.Log("    id:  " + server.id);
					    Debug.Log("    type:" + server.type);
					    break;
				    default:
					    throw new ArgumentOutOfRangeException(nameof(action), action, null);
			    }
		    });
		    #if UNITY_EDITOR
		    PitayaCluster.Initialize(nc, sdConfig, sv, NativeLogLevel.Debug, listener, "MY_LOG_FILE.txt");
		    #else
		    PitayaCluster.Initialize(grpcConfig, sdConfig, sv, NativeLogLevel.Debug, listener);
            #endif
	    }
	    catch (PitayaException e)
	    {
		    Debug.LogError($"Failed to create cluster {e.Message}");
		    Application.Quit();
		    return;
	    }

	    NPitaya.Models.Logger.Info("pitaya lib initialized successfully :)");

	    var tr = new TestRemote();
	    PitayaCluster.RegisterRemote(tr);

    }

	private void Awake()
	{
	    PitayaCluster.AddSignalHandler(() =>
	    {
		    Debug.Log("Got signal handler, quitting pitaya cluster");
		    Application.Quit();
	    });
	}

	private void SendRpcButtonClicked()
	{
		var msg = new Protos.RPCMsg {Msg = inputRPC.text};
		try
        {
            // var res = PitayaCluster.Rpc<Protos.RPCRes>(Route.FromString("csharp.testremote.remote"), msg);
            var res = PitayaCluster.Rpc<Protos.RPCRes>(Route.FromString("room.room.test"), msg).Result;
            Debug.Log($"received rpc res: {res.Msg}");
        }
		catch (Exception e)
		{
            Debug.Log(e.Message);
        }
    }

	// Use this for initialization
	private void Start ()
	{
		Button btnInit = initButton.GetComponent<Button>();
		btnInit.onClick.AddListener(Init);

		Button btnSendRpc = sendRPCButton.GetComponent<Button>();
		btnSendRpc.onClick.AddListener(SendRpcButtonClicked);

		Init();
	}

	// Update is called once per frame
	void Update () {

	}

	void OnApplicationQuit()
	{
		Debug.Log("Destroying pitaya cluster");
		PitayaCluster.Terminate();
	}

//	private void OnDestroy()
//	{
//	}
}
