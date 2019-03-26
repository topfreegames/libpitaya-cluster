using System.Reflection;
using Google.Protobuf;
using System.Collections.Generic;
using System.Threading.Tasks;

namespace NPitaya.Models
{
    public class BaseRemoteMethod: IBaseRemote
    {
        public string GetName() {
            return GetType().Name;
        }

        public Dictionary<string,  RemoteMethod> getRemotesMap() {
            Dictionary<string, RemoteMethod> dict = new Dictionary<string, RemoteMethod>();
            MethodBase[] methods = GetType().GetMethods(BindingFlags.Instance|BindingFlags.Public);
            foreach (MethodInfo m in methods) {
                if (m.IsPublic){
                    if (typeof(Task).IsAssignableFrom(m.ReturnType))
                    {
                        var genericArguments = m.ReturnType.GetGenericArguments();
                        if (genericArguments.Length == 0 || (genericArguments.Length == 1 &&
                                                             typeof(IMessage).IsAssignableFrom(genericArguments[0])))
                        {
                            var returnType = genericArguments.Length == 0 ? null : genericArguments[0];
                            ParameterInfo[] parameters = m.GetParameters();
                            if (parameters.Length == 1)
                            {
                                if (typeof(IMessage).IsAssignableFrom(parameters[0].ParameterType))
                                {
                                    dict[m.Name] = new RemoteMethod(this, m, returnType, parameters[0].ParameterType);
                                }
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