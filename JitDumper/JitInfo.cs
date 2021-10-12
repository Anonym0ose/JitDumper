namespace JitDumper
{
    public struct JitInfo
    {
        public nint LocalSigs;
        public nint CilCode;
        public nint EhClauses;
        public nint CilCodeSize;
        public nint ExceptionHandlersCount;
        public nint LocalsSize;
    }
}