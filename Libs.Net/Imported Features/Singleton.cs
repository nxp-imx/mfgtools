using System;
//using System.Collections.Generic;
using System.Reflection;
//using System.Text;

namespace Utils
{
    public static class Singleton<T>
      where T : class
    {
        static Singleton()
        {
        }

        public static readonly T Instance =
          typeof(T).InvokeMember(typeof(T).Name,
                                 BindingFlags.CreateInstance |
                                 BindingFlags.Instance |
                                 BindingFlags.NonPublic,
                                 null, null, null) as T;
    }
}
