using System;
using System.Reflection;
using Google.Protobuf;
using System.Collections.Generic;
using Pitaya.Models;

namespace Pitaya
{
  public class BaseHandler
  {
    public string GetName() {
      return GetType().Name;
    }

    public Dictionary<string,  HandlerMethod> getHandlerMap() {
      Dictionary<string, HandlerMethod> dict = new Dictionary<string, HandlerMethod>();
      MethodBase[] methods = this.GetType().GetMethods(BindingFlags.Instance|BindingFlags.Public);
      foreach (MethodInfo m in methods) {
        if (m.IsPublic){
          if (typeof(IMessage).IsAssignableFrom(m.ReturnType) || typeof(void) == m.ReturnType){
            ParameterInfo[] parameters = m.GetParameters();
            if (parameters.Length == 2) // TODO need to use context
            {
              if (typeof(Session) == parameters[0].ParameterType &&  // TODO support bytes in and out, support context
                  (typeof(IMessage).IsAssignableFrom(parameters[1].ParameterType))){
                dict[m.Name] = new HandlerMethod(this, m, m.ReturnType, parameters[1].ParameterType);
              }             
            }
            if (parameters.Length == 1)
              if (typeof(Session) == parameters[0].ParameterType)
                dict[m.Name] = new HandlerMethod(this, m, m.ReturnType, null);
          }
        }
      }
      return dict;
    }

    private static bool isValidHandler() {
      return true; //TODO implement this
    }
  }
}