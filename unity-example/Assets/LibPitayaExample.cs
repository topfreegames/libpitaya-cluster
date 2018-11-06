using UnityEngine;
using UnityEngine.UI;
using Pitaya;
using System;
using AOT;
using Logger = Pitaya.Logger;

public class LibPitayaExample : MonoBehaviour {

	public Button initButton;

	public Button sendRPCButton;

	public InputField inputRPC;

	[MonoPInvokeCallback(typeof(PitayaCluster.LogHandler))]
	private static void LogFunction(string msg)
	{
		Debug.Log(msg);
	}

	private PitayaCluster _cluster;

    void InitButtonClicked()
    {
	    Debug.Log("Init button clicked!");

        Pitaya.Logger.SetLevel(LogLevel.DEBUG);
        Console.WriteLine("c# prog running");

	    string serverId = Guid.NewGuid().ToString();

	    var sdConfig = new SDConfig(
		    endpoints: "http://127.0.0.1:4001",
		    etcdPrefix: "pitaya/",
		    heartbeatTTLSec: 60,
		    logHeartbeat: false,
		    logServerSync: true,
		    logServerDetails: false,
		    syncServersIntervalSec: 60);

	    var sv = new Server(
		    serverId,
		    "csharp",
		    "{\"ip\":\"127.0.0.1\"}",
		    "localhost",
		    false);

	    NatsConfig nc = new NatsConfig("127.0.0.1:4222", 2000, 1000, 3, 100);

	    Debug.Log("Adding signal handler");
	    PitayaCluster.AddSignalHandler(() =>
	    {
		    if (_cluster != null)
		    {
			    _cluster.Dispose();
			    _cluster = null;
		    }
		    Application.Quit();
	    });
	    Debug.Log("Adding signal handler DONE");

	    try
	    {
		    Debug.Log("Creating instance of PitayaCluster");
			_cluster = new PitayaCluster(sdConfig, nc, sv, LogFunction);
	    }
	    catch (PitayaException e)
	    {
		    Debug.LogError($"Failed to create cluster {e.Message}");
		    Application.Quit();
	    }

	    Pitaya.Logger.Info("pitaya lib initialized successfully :)");

	    var tr = new TestRemote();
	    _cluster.RegisterRemote(tr);

    }

	void SendRPCButtonClicked()
	{
		var msg = new Protos.RPCMsg {Msg = inputRPC.text};
		try
        {
            var res = _cluster.Rpc<Protos.RPCRes>(Route.FromString("csharp.testremote.remote"), msg);
            Debug.Log($"received rpc res: {res.Msg}");
        }
		catch (Exception e)
		{
            Debug.Log(e.Message);
        }
    }

	// Use this for initialization
	void Start ()
	{
		_cluster = null;

		Button btnInit = initButton.GetComponent<Button>();
		btnInit.onClick.AddListener(InitButtonClicked);

		Button btnSendRpc = sendRPCButton.GetComponent<Button>();
		btnSendRpc.onClick.AddListener(SendRPCButtonClicked);
	}

	// Update is called once per frame
	void Update () {

	}

	private void OnDestroy()
	{
        _cluster?.Dispose();
	}
}
