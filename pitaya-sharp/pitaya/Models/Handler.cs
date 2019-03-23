using static Pitaya.Utils.Utils;

namespace Pitaya.Models
{
    [System.AttributeUsage(System.AttributeTargets.Class | System.AttributeTargets.Struct)]
    public class Handler: System.Attribute
    {
        private string _name;
        private PitayaCluster.RemoteNameFunc _nameFunc;

        public Handler(string name, PitayaCluster.RemoteNameFunc nameFunc)
        {
            _name = name;
            _nameFunc = nameFunc;
        }
        
        public Handler(string name)
        {
            _name = name;
            _nameFunc = DefaultRemoteNameFunc;
        }

        public Handler()
        {
            _name = DefaultRemoteNameFunc(GetType().Name);
            _nameFunc = DefaultRemoteNameFunc;
        }
    }
}