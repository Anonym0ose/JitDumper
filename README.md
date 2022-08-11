 # Jit Dumper - HWBP Branch
 
 A CIL method body dumper that gets information from the JIT compiler.
 This branch uses hardware breakpoint hooks instead of the inline hooks used in the master branch.
 PolyHook2 needs to be installed from vcpkg to build this project.
 
 # Commands to install PolyHook2:
 
 ```
 git clone https://github.com/Microsoft/vcpkg.git
 cd vcpkg
 .\bootstrap-vcpkg.bat -disableMetrics
 (as admin) .\vcpkg integrate install
 vcpkg.exe install polyhook2 --triplet x86-windows
 vcpkg.exe install polyhook2 --triplet x64-windows
 ```
 
 # Compatibility (Windows Only)
- .NET Framework 4.8 (x86 and x64) (Tested)
- .NET Core 1 (x86 and x64) (Not tested)
- .NET Core 2 (x86 and x64) (Not tested)
- .NET Core 3 (x86 and x64) (Tested)
- .NET 5 (x86 and x64) (Tested)

# Remarks
- Do NOT debug the Jit Dumper as this may interfere with the hardware breakpoint hooks.
- An internet connection is required for downloading PDB symbols.
- You are expected to have the C/C++ tools from Visual Studio and the Windows SDK installed.
- For best results with .NET Framework, make sure version 4.8 is installed.
- Loading dependencies is not supported in .NET Core.
- You should build this program with the same architechture and .NET version as your target application.

# Credits
- [Washi1337](https://github.com/Washi1337/) for AsmResolver, and for their overall help with this project.
- [Microsoft](https://github.com/microsoft/) for Detours.
- [stevemk14ebr](https://github.com/stevemk14ebr) for PolyHook2.