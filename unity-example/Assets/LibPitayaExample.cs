using UnityEngine;
using UnityEngine.UI;
using Pitaya;
using System;

public class LibPitayaExample : MonoBehaviour {

	public Button initButton;

	public Button sendRPCButton;

	public InputField inputRPC;

    void InitButtonClicked()
    {
        Pitaya.Logger.SetLevel(LogLevel.DEBUG);
        Console.WriteLine("c# prog running");

        SDConfig sdConfig = new SDConfig("127.0.0.1:2379", 30, "pitaya/", 30, true, 60);
        NatsRPCClientConfig rpcClientConfig = new NatsRPCClientConfig("nats://localhost:4222", 10, 5000);
        // TODO does it makes sense to give freedom to set reconnectionRetries and messagesBufferSize?
        NatsRPCServerConfig rpcServerConfig = new NatsRPCServerConfig("nats://localhost:4222", 10, 75);

        PitayaCluster.Init(
          sdConfig,
          rpcClientConfig,
          rpcServerConfig,
          new Server(
            System.Guid.NewGuid().ToString(),
            "unity",
            "{\"ip\":\"127.0.0.1\"}",
            false)
        );

        TestRemote tr = new TestRemote();
        PitayaCluster.RegisterRemote(tr);

    }

	void SendRPCButtonClicked(){
        string text = inputRPC.text;

        Protos.RPCMsg msg = new Protos.RPCMsg();
        msg.Msg = text;
        try
        {
            Protos.RPCRes res = PitayaCluster.RPC<Protos.RPCRes>(Pitaya.Route.fromString("connector.testremote.test"), msg);
            Debug.Log(String.Format("received rpc res: {0}", res.Msg));
        } catch (Exception e){
            Debug.Log(e.Message);
        }
    }
	// Use this for initialization
	void Start () {
		Button btnInit = initButton.GetComponent<Button>();
		btnInit.onClick.AddListener(InitButtonClicked);

		Button btnSendRpc = sendRPCButton.GetComponent<Button>();
		btnSendRpc.onClick.AddListener(SendRPCButtonClicked);
	}
	
	// Update is called once per frame
	void Update () {
		
	}
}
