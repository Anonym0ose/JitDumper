 # Jit Dumper

 A CIL method body dumper that gets information from the JIT compiler.

 # Compatibility (Windows Only)
- .NET Framework 4.8 (x86 and x64) (Tested)
- .NET Core 1 (x86 and x64) (Not tested)
- .NET Core 2 (x86 and x64) (Not tested)
- .NET Core 3 (x86 and x64) (Tested)
- .NET 5 (x86 and x64) (Tested)

# Remarks
- An internet connection is required for downloading PDB symbols.
- You are expected to have the C/C++ tools from Visual Studio and the Windows SDK installed.
- For best results with .NET Framework, make sure version 4.8 is installed.
- Loading dependencies is not supported in .NET Core.
- You should build this program with the same architechture and .NET version as your target application.

# Credits
- [Washi1337](https://github.com/Washi1337/) for AsmResolver, and for their overall help with this project.
- [Microsoft](https://github.com/microsoft/) for Detours.
