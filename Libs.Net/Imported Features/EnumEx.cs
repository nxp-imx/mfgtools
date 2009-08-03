using System;
using System.Collections.Generic;
using System.Globalization;
using System.Reflection;

namespace Utils
{
    // http://rodenbaugh.net
    public static class Enum<T>
        where T : struct, IComparable, IFormattable
    {
        #region Constructors
        static Enum()
        {
            Debug.Assert(typeof(T).IsEnum, string.Format("The class ({0}) must be an enum", typeof(T).FullName));
        }
        #endregion

        #region Static Methods
        ///<summary>
        ///</summary>
        ///<returns></returns>
        public static List<T> ConvertToList()
        {
            List<T> values = new List<T>();
            Array array = Enum.GetValues(typeof(T));
            foreach (T item in array)
                values.Add(item);

            return values;
        }

        ///<summary>
        ///</summary>
        ///<param name="value"></param>
        ///<returns></returns>
        public static string GetName(int value)
        {
            return Enum.GetName(typeof(T), value);
        }

        ///<summary>
        ///</summary>
        ///<param name="value"></param>
        ///<returns></returns>
        public static string GetName(long value)
        {
            return Enum.GetName(typeof(T), value);
        }

        ///<summary>
        ///</summary>
        ///<returns></returns>
        public static string[] GetNames()
        {
            return Enum.GetNames(typeof(T));
        }

        ///<summary>
        ///</summary>
        ///<typeparam name="U"></typeparam>
        ///<returns></returns>
        public static List<U> GetValues<U>()
        {
            List<U> values = new List<U>();
            Array array = Enum.GetValues(typeof(T));
            foreach (U item in array)
                values.Add(item);

            return values;
        }

        ///<summary>
        ///</summary>
        ///<param name="input"></param>
        ///<param name="result"></param>
        ///<returns></returns>
        public static bool TryParse(string input, out T? result)
        {
            result = null;
            if (!Enum.IsDefined(typeof(T), input))
                return false;

            result = (T)Enum.Parse(typeof(T), input, true);
            return true;
        }

        ///<summary>
        ///</summary>
        ///<param name="input"></param>
        ///<param name="result"></param>
        ///<returns></returns>
        public static bool TryParse(string input, out T result)
        {
            T? temp;
            if (!TryParse(input, out temp))
            {
                //input not found in the Enum, fill the out parameter with the first item from the enum
                Array values = Enum.GetValues(typeof(T));

                result = (T)values.GetValue(values.GetLowerBound(0));
                return false;
            }

            result = temp.Value;
            return true;
        }

        private static string GetName(T value)
        {
            return Enum.GetName(typeof(T), value);
        }
        #endregion

        #region Nested type: DataBinding
        ///<summary>
        ///</summary>
        public static class DataBinding
        {
            #region Delegates
            ///<summary>
            ///</summary>
            ///<param name="value"></param>
            public delegate string FormatEnumName(T value);
            #endregion

            #region Static Methods
            ///<summary>
            ///</summary>
            ///<returns></returns>
            public static IList<IBindableEnum<T>> CreateList()
            {
                return CreateList(null);
            }

            ///<summary>
            ///</summary>
            ///<param name="formatName"></param>
            ///<returns></returns>
            public static IList<IBindableEnum<T>> CreateList(FormatEnumName formatName)
            {
                List<IBindableEnum<T>> retVal = new List<IBindableEnum<T>>();

                Array values = Enum.GetValues(typeof(T));

                foreach (T value in values)
                {
                    if (formatName != null)
                        retVal.Add(new InternalBindableEnum(value, formatName(value)));
                    else
                        retVal.Add(new InternalBindableEnum(value));
                }

                return retVal;
            }
            #endregion

            #region Nested type: InternalBindableEnum
            private class InternalBindableEnum : IBindableEnum<T>
            {
                #region Readonly & Static Fields
                private readonly string name;
                #endregion

                #region Fields
                private T value;
                #endregion

                #region Constructors
                public InternalBindableEnum(T value)
                {
                    this.value = value;
                    name = GetName(value);
                }

                public InternalBindableEnum(T value, string name)
                {
                    this.value = value;
                    this.name = name;
                }
                #endregion

                #region Instance Properties
                public string Name
                {
                    get { return name; }
                }

                public T Value
                {
                    get { return value; }
                }
                #endregion

                #region Instance Public Methods
                public int CompareTo(IBindableEnum<T> other)
                {
                    return value.CompareTo(other.Value);
                }

                public bool Equals(IBindableEnum<T> other)
                {
                    return value.Equals(other.Value);
                }
                #endregion
            }
            #endregion
        }
        #endregion

        #region Nested type: Flags
        ///<summary>
        ///</summary>
        public static class Flags
        {

            private static readonly TypeCode typeCode;

            #region Constructors
            static Flags()
            {
                typeCode = Type.GetTypeCode(typeof(T));
                Debug.Assert(typeof(T).IsDefined(typeof(FlagsAttribute), false), string.Format("The Enum ({0}) must have the System.FlagsAttribute applied to it.", typeof(T).FullName));
            }
            #endregion

            #region Static Methods
            ///<summary>
            ///</summary>
            ///<param name="flags"></param>
            ///<param name="flagToClear"></param>
            ///<returns></returns>
            public static T ClearFlag(T flags, T flagToClear)
            {
                if (typeCode == TypeCode.Int64)
                    return ClearFlag(Convert.ToInt64(flags, CultureInfo.CurrentCulture), Convert.ToInt64(flagToClear, CultureInfo.CurrentCulture));
                else // all other integral types except unsigned* which are not yet supported....
                    return ClearFlag(Convert.ToInt32(flags, CultureInfo.CurrentCulture), Convert.ToInt32(flagToClear, CultureInfo.CurrentCulture));
            }

            ///<summary>
            ///</summary>
            ///<param name="flags"></param>
            ///<param name="flagToClear"></param>
            ///<returns></returns>
            public static T ClearFlag(int flags, T flagToClear)
            {
                return ClearFlag(flags, Convert.ToInt32(flagToClear, CultureInfo.CurrentCulture));
            }

            ///<summary>
            ///</summary>
            ///<param name="flags"></param>
            ///<param name="flagToClear"></param>
            ///<returns></returns>
            public static T ClearFlag(long flags, T flagToClear)
            {
                return ClearFlag(flags, Convert.ToInt64(flagToClear, CultureInfo.CurrentCulture));
            }


            ///<summary>
            ///</summary>
            ///<param name="flags"></param>
            ///<param name="flagToFind"></param>
            ///<returns></returns>
            public static bool IsFlagSet(T flags, T flagToFind)
            {
                if (typeCode == TypeCode.Int64)
                    return IsFlagSet(Convert.ToInt64(flags, CultureInfo.CurrentCulture), Convert.ToInt64(flagToFind, CultureInfo.CurrentCulture));
                else // all other integral types except unsigned* which are not yet supported....
                    return IsFlagSet(Convert.ToInt32(flags, CultureInfo.CurrentCulture), Convert.ToInt32(flagToFind, CultureInfo.CurrentCulture));
            }

            ///<summary>
            ///</summary>
            ///<param name="flags"></param>
            ///<param name="flagToFind"></param>
            ///<returns></returns>
            public static bool IsFlagSet(int flags, T flagToFind)
            {
                return IsFlagSet(Convert.ToInt32(flags, CultureInfo.CurrentCulture), Convert.ToInt32(flagToFind, CultureInfo.CurrentCulture));
            }

            ///<summary>
            ///</summary>
            ///<param name="flags"></param>
            ///<param name="flagToFind"></param>
            ///<returns></returns>
            public static bool IsFlagSet(long flags, T flagToFind)
            {
                return IsFlagSet(Convert.ToInt64(flags, CultureInfo.CurrentCulture), Convert.ToInt32(flagToFind, CultureInfo.CurrentCulture));
            }

            ///<summary>
            ///</summary>
            ///<param name="flags"></param>
            ///<param name="flagToSet"></param>
            ///<returns></returns>
            public static T SetFlag(T flags, T flagToSet)
            {
                if (typeCode == TypeCode.Int64)
                    return SetFlag(Convert.ToInt64(flags, CultureInfo.CurrentCulture), Convert.ToInt64(flagToSet, CultureInfo.CurrentCulture));
                else // all other integral types except unsigned* which are not yet supported....
                    return SetFlag(Convert.ToInt32(flags, CultureInfo.CurrentCulture), Convert.ToInt32(flagToSet, CultureInfo.CurrentCulture));
            }

            ///<summary>
            ///</summary>
            ///<param name="flags"></param>
            ///<param name="flagToSet"></param>
            ///<returns></returns>
            public static T SetFlag(int flags, T flagToSet)
            {
                return SetFlag(flags, Convert.ToInt32(flagToSet, CultureInfo.CurrentCulture));
            }

            ///<summary>
            ///</summary>
            ///<param name="flags"></param>
            ///<param name="flagToSet"></param>
            ///<returns></returns>
            public static T SetFlag(long flags, T flagToSet)
            {
                return SetFlag(flags, Convert.ToInt64(flagToSet, CultureInfo.CurrentCulture));
            }

            ///<summary>
            ///</summary>
            ///<param name="flags"></param>
            ///<param name="flagToSwap"></param>
            ///<returns></returns>
            public static T SwapFlag(T flags, T flagToSwap)
            {
                if (typeCode == TypeCode.Int64)
                    return SwapFlag(Convert.ToInt64(flags, CultureInfo.CurrentCulture), Convert.ToInt64(flagToSwap, CultureInfo.CurrentCulture));
                else // all other integral types except unsigned* which are not yet supported....
                    return SwapFlag(Convert.ToInt32(flags, CultureInfo.CurrentCulture), Convert.ToInt32(flagToSwap, CultureInfo.CurrentCulture));
            }

            ///<summary>
            ///</summary>
            ///<param name="flags"></param>
            ///<param name="flagToSwap"></param>
            ///<returns></returns>
            public static T SwapFlag(int flags, T flagToSwap)
            {
                return SwapFlag(flags, Convert.ToInt32(flagToSwap, CultureInfo.CurrentCulture));
            }

            ///<summary>
            ///</summary>
            ///<param name="flags"></param>
            ///<param name="flagToSwap"></param>
            ///<returns></returns>
            public static T SwapFlag(long flags, T flagToSwap)
            {
                return SwapFlag(flags, Convert.ToInt64(flagToSwap, CultureInfo.CurrentCulture));
            }

            ///<summary>
            ///</summary>
            ///<param name="flags"></param>
            ///<param name="flagToClear"></param>
            ///<returns></returns>
            public static T ClearFlag(int flags, int flagToClear)
            {
                if (IsFlagSet(flags, flagToClear))
                    flags &= ~flagToClear;

                return (T)Enum.ToObject(typeof(T), flags);
            }

            ///<summary>
            ///</summary>
            ///<param name="flags"></param>
            ///<param name="flagToClear"></param>
            ///<returns></returns>
            public static T ClearFlag(long flags, long flagToClear)
            {
                if (IsFlagSet(flags, flagToClear))
                    flags &= ~flagToClear;

                return (T)Enum.ToObject(typeof(T), flags);
            }

            ///<summary>
            ///</summary>
            ///<param name="flags"></param>
            ///<param name="flagToFind"></param>
            ///<returns></returns>
            public static bool IsFlagSet(int flags, int flagToFind)
            {
                return ((flags & flagToFind) == flagToFind);
            }

            ///<summary>
            ///</summary>
            ///<param name="flags"></param>
            ///<param name="flagToFind"></param>
            ///<returns></returns>
            public static bool IsFlagSet(long flags, long flagToFind)
            {
                return ((flags & flagToFind) == flagToFind);
            }

            ///<summary>
            ///</summary>
            ///<param name="flags"></param>
            ///<param name="flagToSet"></param>
            ///<returns></returns>
            public static T SetFlag(int flags, int flagToSet)
            {
                if (!IsFlagSet(flags, flagToSet))
                    flags |= flagToSet;

                return (T)Enum.ToObject(typeof(T), flags);
            }

            ///<summary>
            ///</summary>
            ///<param name="flags"></param>
            ///<param name="flagToSet"></param>
            ///<returns></returns>
            public static T SetFlag(long flags, long flagToSet)
            {
                if (!IsFlagSet(flags, flagToSet))
                    flags |= flagToSet;

                return (T)Enum.ToObject(typeof(T), flags);
            }

            ///<summary>
            ///</summary>
            ///<param name="flags"></param>
            ///<param name="flagToSwap"></param>
            ///<returns></returns>
            public static T SwapFlag(int flags, int flagToSwap)
            {
                if (IsFlagSet(flags, flagToSwap))
                    return ClearFlag(flags, flagToSwap);
                else
                    return SetFlag(flags, flagToSwap);
            }

            ///<summary>
            ///</summary>
            ///<param name="flags"></param>
            ///<param name="flagToSwap"></param>
            ///<returns></returns>
            public static T SwapFlag(long flags, long flagToSwap)
            {
                if (IsFlagSet(flags, flagToSwap))
                    return ClearFlag(flags, flagToSwap);
                else
                    return SetFlag(flags, flagToSwap);
            }
            #endregion
        }
        #endregion

        #region Nested type: Reflection
        ///<summary>
        ///</summary>
        public static class Reflection
        {
            #region Static Methods
            ///<summary>
            ///</summary>
            ///<param name="value"></param>
            ///<typeparam name="U"></typeparam>
            ///<returns></returns>
            public static U GetAttributeOnValue<U>(T value) where U : Attribute
            {
                string name = GetName(value);
                Type type = typeof(T);
                MemberInfo[] members = type.GetMember(name, BindingFlags.Public | BindingFlags.Static);
                if (members != null && members.Length > 0)
                    return Attribute.GetCustomAttribute(members[0], typeof(U)) as U;

                return null;
            }
            #endregion
        }
        #endregion
    }

    ///<summary>
    ///</summary>
    ///<typeparam name="T"></typeparam>
    public interface IBindableEnum<T> : IComparable<IBindableEnum<T>>, IEquatable<IBindableEnum<T>>
        where T : struct, IComparable, IFormattable
    {
        #region Instance Properties
        ///<summary>
        ///</summary>
        string Name { get; }
        ///<summary>
        ///</summary>
        T Value { get; }
        #endregion
    }
}
