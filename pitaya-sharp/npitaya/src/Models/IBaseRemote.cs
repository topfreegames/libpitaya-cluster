using System.Collections.Generic;
using NPitaya;

namespace NPitaya.Models
{
    public interface IBaseRemote
    {
        string GetName();
        Dictionary<string, RemoteMethod> getRemotesMap();
    }
}