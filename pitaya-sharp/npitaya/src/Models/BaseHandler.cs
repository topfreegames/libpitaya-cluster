using System.Reflection;
using Google.Protobuf;
using System.Collections.Generic;
using System.Threading.Tasks;

namespace NPitaya.Models
{
    public class BaseHandler : IBaseRemote
    {
        delegate dynamic SimpleDelegate(PitayaSession s);

        delegate dynamic CompositeDelegate(PitayaSession s, dynamic a);

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
                    if (typeof(IMessage).IsAssignableFrom(m.ReturnType) || typeof(void) == m.ReturnType)
                    {
                        ParameterInfo[] parameters = m.GetParameters();
                        if (parameters.Length == 2) // TODO need to use context
                        {
                            if (typeof(PitayaSession) ==
                                parameters[0].ParameterType && // TODO support bytes in and out, support context
                                (typeof(IMessage).IsAssignableFrom(parameters[1].ParameterType)))
                            {
                                dict[m.Name] = new RemoteMethod(this, m, m.ReturnType, parameters[1].ParameterType);
                            }
                        }

                        if (parameters.Length == 1)
                            if (typeof(PitayaSession) == parameters[0].ParameterType)
                            {
                                dict[m.Name] = new RemoteMethod(this, m, m.ReturnType, null);
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