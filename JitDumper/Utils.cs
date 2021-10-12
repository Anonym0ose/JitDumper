using System;
using AsmResolver.DotNet;

namespace JitDumper
{
    public static class Utils
    {
        public static nint GetMethodHandle(this ModuleHandle moduleHandle, IMetadataMember method)
        {
            return moduleHandle.GetMethodHandle(method.MetadataToken.ToInt32());
        }

        private static nint GetMethodHandle(this ModuleHandle moduleHandle, int token)
        {
            nint handle = moduleHandle.ResolveMethodHandle(token).Value;
            return PInvoke.GetUnboxedMethod(handle);
        }
    }
}