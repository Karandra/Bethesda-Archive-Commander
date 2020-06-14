# Bethesda Archive Commander
Bethesda Archive Commander (**BAC** or **BArC**) is archive viewer and extractor tool for archives used by Bethesda games.

This tool was written as a university project so don't expect much from it. It works and that's all I can tell you. It's written in C++ (C++14 at least), uses wxWidgets v3.1+ and requires [KxFramework](https://github.com/KerberX/KxFramework) v1.0 (there is a branch for v1.0). I also had a version ported to Qt5 but I can't find it anywhere.

# Supported formats
- Fallout 3
- Falout: New Vegas
- Oblivion
- Skyrim LE
- Possibly something else that uses BSA format that these games are using.

# Building
- Download and build wxWidgets v3.1+.
- Get KxFramework [v1.0](https://github.com/KerberX/KxFramework/tree/v1.0) to build somehow.
- Build this project in Visual Studio.
- ...
- PROFIT?

Or update it to use current version of KxFramework and use VCPkg to get it as a dependency.
