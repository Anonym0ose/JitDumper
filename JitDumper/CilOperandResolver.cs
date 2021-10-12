using AsmResolver.DotNet;
using AsmResolver.DotNet.Code.Cil;
using AsmResolver.PE.DotNet.Metadata.Tables;

namespace JitDumper
{
    /// <inheritdoc />
    public class CilOperandResolver : PhysicalCilOperandResolver
    {
        /// <inheritdoc />
        public CilOperandResolver(ModuleDefinition contextModule, CilMethodBody methodBody) : base(contextModule,
            methodBody)
        {

        }

        /// <inheritdoc />
        public override object ResolveMember(MetadataToken token)
        {
            var resolvedToken = PInvoke.ResolveToken(token);
            if (resolvedToken == 0)
                return base.ResolveMember(token);

            return base.ResolveMember(resolvedToken);
        }
    }
}
