using System.Reflection;
using Google.Protobuf;
using System.Collections.Generic;

namespace Pitaya
{
  public class BaseRemote
  {
    public string GetName() {
      return GetType().Name;
    }

    public Dictionary<string,  RemoteMethod> getRemotesMap() {
      Dictionary<string, RemoteMethod> dict = new Dictionary<string, RemoteMethod>();
      MethodBase[] methods = this.GetType().GetMethods(BindingFlags.Instance|BindingFlags.Public);
      foreach (MethodInfo m in methods) {
        if (m.IsPublic){
          if (typeof(IMessage).IsAssignableFrom(m.ReturnType)){
            ParameterInfo[] parameters = m.GetParameters();
            if (parameters.Length == 1) {
              if (typeof(IMessage).IsAssignableFrom(parameters[0].ParameterType)){
                dict[m.Name] = new RemoteMethod(this, m, m.ReturnType, parameters[0].ParameterType);
              }
            }
          }
        }
      }
      return dict;
    }

    private static bool isValidRemote() {
      return true;
    }
  }
}
