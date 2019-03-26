using System.Collections.Generic;
using Pitaya;

namespace pitaya.Models
{
    public interface IBaseRemote
    {
        string GetName();
        Dictionary<string, RemoteMethod> getRemotesMap();
    }
}