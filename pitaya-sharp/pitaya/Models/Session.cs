using System;
using System.Collections.Generic;
using Google.Protobuf;
using Newtonsoft.Json;
using Pitaya.Constants;
using Protos;

namespace Pitaya.Models
{
    public class Session
    {
        private Int64 _id;
        private string _uid;
        private string _frontendId;
        private Dictionary<string, object> _data;
        private string _rawData;
        public string RawData => _rawData;

        public Session(Protos.Session sessionProto)
        {
            _id = sessionProto.Id;
            _uid = sessionProto.Uid;
            _rawData = sessionProto.Data.ToStringUtf8();
            _data = JsonConvert.DeserializeObject<Dictionary<string, object>>(_rawData);
        }

        public Session(Protos.Session sessionProto, string frontendId):this(sessionProto)
        {
            _frontendId = frontendId;
        }

        public override string ToString()
        {
            return $"ID: {_id}, UID: {_uid}, Data: {_rawData}";
        }

        public void Set(string key, object value)
        {
            _data[key] = value;
            _rawData = JsonConvert.SerializeObject(_data);
        }

        public object GetObject(string key)
        {
            if (!_data.ContainsKey(key))
            {
                throw new Exception($"key not found in session, parameter: {key}");
            }

            return _data[key];
        }

        public string GetString(string key)
        {
            return GetObject(key) as string;
        }
        
        public int GetInt(string key)
        {
            var obj = GetObject(key);
            return obj is int ? (int) obj : 0;
        }

        public double GetDouble(string key)
        {
            var obj = GetObject(key);
            return obj is double ? (double) obj : 0;
        }

        public void PushToFrontend()
        {
            if (String.IsNullOrEmpty(_frontendId))
            {
                throw new Exception("cannot push to frontend, frontendId is invalid!");
            }
            SendRequestToFront(Routes.SessionPushRoute, true);
        }

        public void Bind(string uid)
        {
            if (_uid != "")
            {
                throw new Exception("session already bound!");
            }
            _uid = uid;
            // TODO only if server type is backend
            // TODO bind callbacks
            if (!string.IsNullOrEmpty(_frontendId)){
                BindInFrontend();
            }
        }

        private void BindInFrontend()
        {
            SendRequestToFront(Routes.SessionBindRoute, false);
        }

        private void SendRequestToFront(string route, bool includeData)
        {
            var sessionProto = new Protos.Session
            {
                Id = _id,
                Uid = _uid
            };
            if (includeData)
            {
                sessionProto.Data = ByteString.CopyFromUtf8(_rawData);
            }
            Console.WriteLine($"sending {sessionProto}");
            PitayaCluster.Rpc<Response>(_frontendId, Route.FromString(route), sessionProto);
        }
    }
}