using System.Runtime.InteropServices;
using AsmResolver.PE.DotNet.Metadata.Tables;

namespace JitDumper
{
    public static class PInvoke
    {
        [DllImport("jit_hook.dll", EntryPoint = "compile_method", CallingConvention = CallingConvention.Cdecl)]
        public static extern void CompileMethod(nint methodDesc);

        [DllImport("jit_hook.dll", EntryPoint = "add_hook", CallingConvention = CallingConvention.Cdecl)]
        public static extern bool AddHook(nint compilationCallback, nint uselessMethodDesc, int domainId,
            int runtimeVersionMajor);

        [DllImport("jit_hook.dll", EntryPoint = "get_unboxed_method", CallingConvention = CallingConvention.Cdecl)]
        public static extern nint GetUnboxedMethod(nint methodDesc);

        [DllImport("jit_hook.dll", EntryPoint = "resolve_token", CallingConvention = CallingConvention.Cdecl)]
        public static extern MetadataToken ResolveToken(MetadataToken token);

        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        public delegate void CompilationCallback(ref JitInfo info);
    }
}