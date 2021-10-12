using System;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Reflection;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using AsmResolver.DotNet;
using AsmResolver.DotNet.Builder;
using AsmResolver.DotNet.Code.Cil;
using AsmResolver.PE.File.Headers;

namespace JitDumper
{
    internal static class Program
    {
        private static ModuleDefinition _moduleDefinition;
        private static DumpedBodyReader _dumpedBodyReader;
        private static int _dumpedMethods;

        public static void HelloWorld()
        {
            Console.WriteLine("Hello World!");
        }

        private static void CompilationCallback(ref JitInfo jitInfo)
        {
            if (jitInfo.CilCodeSize < _dumpedBodyReader.CilMethodBody.Instructions.Size)
                return;

            _dumpedBodyReader.ReadVariables(jitInfo.LocalSigs, jitInfo.LocalsSize);
            _dumpedBodyReader.ReadInstructions(jitInfo.CilCode, jitInfo.CilCodeSize);
            _dumpedBodyReader.ReadExceptionHandlers(jitInfo.EhClauses, jitInfo.ExceptionHandlersCount);
            _dumpedMethods++;
        }

        public static void Main(string[] args)
        {
            if (args.Length == 0 || !File.Exists(args[0]))
            {
                Console.WriteLine("Usage:    JitDumper.exe  <file-path>");
                Console.WriteLine("Example:  JitDumper.exe  C:\\Path\\To\\File.exe");
                Console.WriteLine("Press any key to exit...");
                Console.ReadKey();
                return;
            }

            var assembly = Assembly.LoadFrom(args[0]);
            var moduleHandle = assembly!.ManifestModule.ModuleHandle;
            RuntimeHelpers.RunModuleConstructor(moduleHandle);

            var moduleBaseAddress = Marshal.GetHINSTANCE(assembly.ManifestModule);
            _moduleDefinition = ModuleDefinition.FromModuleBaseAddress(moduleBaseAddress);
            var assemblyResolver = (AssemblyResolverBase)_moduleDefinition.MetadataResolver.AssemblyResolver;
            assemblyResolver.SearchDirectories.Add(Path.GetDirectoryName(assembly.Location));

            if (!PInvoke.AddHook(
                Marshal.GetFunctionPointerForDelegate<PInvoke.CompilationCallback>(CompilationCallback),
                typeof(Program).GetMethod("HelloWorld").MethodHandle.Value,
                AppDomain.CurrentDomain.Id,
                Environment.Version.Major))
            {
                Console.WriteLine("Press any key to exit...");
                Console.ReadKey();
                return;
            }

            var methods = _moduleDefinition.GetAllTypes().SelectMany(t => t.Methods).Where(m => m.CilMethodBody != null)
                .ToArray();

            var stopwatch = new Stopwatch();
            stopwatch.Start();

            foreach (var method in methods)
            {
                _dumpedBodyReader = new(method.CilMethodBody);
                PInvoke.CompileMethod(moduleHandle.GetMethodHandle(method));
            }

            stopwatch.Stop();
            Console.WriteLine($"Dumped {_dumpedMethods}/{methods.Length} in {stopwatch.Elapsed}");

            var imageBuilder = new ManagedPEImageBuilder();
            var factory = new DotNetDirectoryFactory
            {
                MethodBodySerializer = new CilMethodBodySerializer
                {
                    ComputeMaxStackOnBuildOverride = false
                }
            };

            imageBuilder.DotNetDirectoryFactory = factory;

            if (_moduleDefinition.MachineType == MachineType.I386 && _moduleDefinition.PEKind == OptionalHeaderMagic.Pe32Plus)
                _moduleDefinition.PEKind = OptionalHeaderMagic.Pe32;

            string output = args[0].Insert(args[0].Length - 4, "-Dumped");
            _moduleDefinition.Write(output, imageBuilder);

            Console.WriteLine($"File saved to: {output}");
            Console.WriteLine("Press any key to exit...");
            Console.ReadKey();
        }
    }
}