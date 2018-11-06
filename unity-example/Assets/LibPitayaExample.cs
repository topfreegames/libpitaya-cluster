using UnityEngine;
using UnityEngine.UI;
using Pitaya;
using System;
using AOT;

public class LibPitayaExample : MonoBehaviour {

	public Button initButton;

	public Button sendRPCButton;

	public InputField inputRPC;

	[MonoPInvokeCallback(typeof(PitayaCluster.LogHandler))]
	private static void LogFunction(string msg)
	{
		Debug.Log(msg);
	}

    void InitButtonClicked()
    {
	    Debug.Log("Init button clicked!");

        Pitaya.Logger.SetLevel(LogLevel.DEBUG);
        Console.WriteLine("c# prog running");

	    string serverId = Guid.NewGuid().ToString();

	    var sdConfig = new SDConfig("127.0.0.1:2379", 30, "pitaya/", 30, true, 60);

	    var sv = new Server(
		    serverId,
		    "csharp",
		    "{\"ip\":\"127.0.0.1\"}",
		    "localhost",
		    false);

	    NatsConfig nc = new NatsConfig("127.0.0.1:4222", 2000, 1000, 3, 100);

	    PitayaCluster.onSignalEvent += () =>
	    {
		    PitayaCluster.Shutdown();
		    Environment.Exit(0);
	    };

	    bool initStatus = PitayaCluster.Init(sdConfig, nc, sv, LogFunction);
	    if (!initStatus)
	    {
		    throw new Exception("failed to initialize pitaya lib :/");
	    }

	    Pitaya.Logger.Info("pitaya lib initialized successfully :)");

	    var tr = new TestRemote();
	    PitayaCluster.RegisterRemote(tr);

    }

	void SendRPCButtonClicked()
	{
		var msg = new Protos.RPCMsg {Msg = inputRPC.text};
		try
        {
            var res = PitayaCluster.Rpc<Protos.RPCRes>(Route.FromString("connector.testremote.test"), msg);
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
		Button btnInit = initButton.GetComponent<Button>();
		btnInit.onClick.AddListener(InitButtonClicked);

		Button btnSendRpc = sendRPCButton.GetComponent<Button>();
		btnSendRpc.onClick.AddListener(SendRPCButtonClicked);
	}

	// Update is called once per frame
	void Update () {

	}
}
