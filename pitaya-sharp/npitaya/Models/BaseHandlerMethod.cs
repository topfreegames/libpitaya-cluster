using System.Reflection;
using Google.Protobuf;
using System.Collections.Generic;
using System.Threading.Tasks;

namespace NPitaya.Models
{
    public class BaseHandlerMethod : IBaseRemote
    {
        delegate dynamic SimpleDelegate(Session s);
        delegate dynamic CompositeDelegate(Session s, dynamic a);
        public string GetName()
        {
            return GetType().Name;
        }

        public Dictionary<string, RemoteMethod> getRemotesMap()
        {
            Dictionary<string, RemoteMethod> dict = new Dictionary<string, RemoteMethod>();
            MethodBase[] methods = this.GetType().GetMethods(BindingFlags.Instance | BindingFlags.Public);
            foreach (var methodBase in methods)
            {
                var m = (MethodInfo) methodBase;
                if (m.IsPublic)
                {
                    if (typeof(Task).IsAssignableFrom(m.ReturnType))
                    {
                        var genericArguments = m.ReturnType.GetGenericArguments();
                        if (genericArguments.Length == 0 || (genericArguments.Length == 1 &&
                                                             typeof(IMessage).IsAssignableFrom(genericArguments[0])))
                        {
                            var returnType = genericArguments.Length == 0 ? null : genericArguments[0];
                            
                            ParameterInfo[] parameters = m.GetParameters();
                            if (parameters.Length == 2) // TODO need to use context
                            {
                                if (typeof(Session) ==
                                    parameters[0].ParameterType && // TODO support bytes in and out, support context
                                    (typeof(IMessage).IsAssignableFrom(parameters[1].ParameterType)))
                                {
                                    dict[m.Name] = new RemoteMethod(this, m, returnType, parameters[1].ParameterType);
                                }
                            }

                            if (parameters.Length == 1)
                                if (typeof(Session) == parameters[0].ParameterType)
                                {
                                    dict[m.Name] = new RemoteMethod(this, m, returnType, null);
                                }

                        }
                    }
                }
            }

            return dict;
        }

        private static bool isValidHandler()
        {
            return true; //TODO implement this
        }
    }
}