using System;
using AsmResolver.DotNet.Code.Cil;
using AsmResolver.DotNet.Serialized;
using AsmResolver.DotNet.Signatures;
using AsmResolver.DotNet.Signatures.Types;
using AsmResolver.IO;
using AsmResolver.PE.DotNet.Cil;

namespace JitDumper
{
    public class DumpedBodyReader
    {
        private readonly BlobReadContext _readContext;
        public CilMethodBody CilMethodBody { get; }

        public DumpedBodyReader(CilMethodBody body)
        {
            if (body.Owner.Module is not SerializedModuleDefinition module)
                throw new NotSupportedException("The module containing the method body should be a serialized module.");

            (_readContext, CilMethodBody) = (new BlobReadContext(module.ReaderContext), body);
        }

        public void ReadVariables(nint localSigs, nint localsSize)
        {
            CilMethodBody.LocalVariables.Clear();

            var source = new UnmanagedDataSource(localSigs, (ulong)localsSize);
            var reader = new BinaryStreamReader(source, source.BaseAddress, 0, (uint)localsSize);

            while (reader.Offset != reader.EndOffset)
            {
                CilMethodBody.LocalVariables.Add(new(TypeSignature.FromReader(in _readContext, ref reader)));
            }
        }

        public void ReadInstructions(nint ilCode, nint ilCodeSize)
        {
            CilMethodBody.Instructions.Clear();
            var source = new UnmanagedDataSource(ilCode, (ulong)ilCodeSize);
            var reader = new BinaryStreamReader(source, source.BaseAddress, 0, (uint)ilCodeSize);

            var operandResolver = new CilOperandResolver(CilMethodBody.Owner.Module, CilMethodBody);
            var disassembler = new CilDisassembler(in reader, operandResolver);
            CilMethodBody.Instructions.AddRange(disassembler.ReadInstructions());
        }

        public void ReadExceptionHandlers(nint ehClauses, nint exceptionHandlersCount)
        {
            CilMethodBody.ExceptionHandlers.Clear();

            const uint ehSize = CilExceptionHandler.FatExceptionHandlerSize;
            var source = new UnmanagedDataSource(ehClauses, ehSize * (ulong)exceptionHandlersCount);
            var reader = new BinaryStreamReader(source, source.BaseAddress, 0, ehSize * (uint)exceptionHandlersCount);

            for (int i = 0; i < exceptionHandlersCount; i++)
            {
                CilMethodBody.ExceptionHandlers.Add(CilExceptionHandler.FromReader(CilMethodBody, ref reader, true));
            }
        }
    }
}