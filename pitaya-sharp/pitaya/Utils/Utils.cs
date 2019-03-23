using System;
using System.Collections.Generic;
using System.Reflection;

namespace Pitaya.Utils {

  public static class Utils {

    public static string DefaultRemoteNameFunc(string methodName){
      var name = methodName;
      if (name != string.Empty && char.IsUpper(name[0]))
      {
        name = char.ToLower(name[0]) + name.Substring(1);
      }
      return name;
    } 
    
    public static IEnumerable<Type> GetTypesWithAttribute(Assembly assembly, Type attribute) {
      foreach(Type type in assembly.GetTypes()) {
        if (type.GetCustomAttributes(attribute, true).Length > 0) {
          yield return type;
        }
      }
    }
  }

}
