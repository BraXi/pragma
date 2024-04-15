# pragma

[![Build Status](https://github.com/BraXi/pragma/actions/workflows/msbuild.yml/badge.svg)](https://github.com/BraXi/pragma/actions/workflows/msbuild.yml) [![Build Status](https://github.com/BraXi/pragma/actions/workflows/msbuild-release-x86.yml/badge.svg)](https://github.com/BraXi/pragma/actions/workflows/msbuild-release-x86.yml)

Pragma is an highly modified IdTech2 engine derived from Quake II aimed at standalone projects.

> THIS IS NOT ANOTHER SOURCE PORT AND YOU CANNOT PLAY QUAKE 2 WITH IT. 


## Quick Overview
- Native Game DLL was replaced with platform independant QuakeC Virtual Machine
- It's completly standalone and does not require Quake 2 data
- OpenGL 2.1 renderer which offloads much of the work to GPU
- Full upport for `MD3` models including groups, tags and per-surf textures
- Per pixel lighting, GLSL shaders, post process effects and more
- Dedicated server binary (currently x86 windows + linux)
- Textures are no longer limited to `256 x 256px`
- Improved network protocol, you're no longer limited to 4096qu coordinates limit
- Model, sound and image limits have been significantly raised
- Supports extended Quake 2 BSP format (qbism) which practicaly removes most of the limitations of the level format
- Decoupled lightmap support for maps compiled with `ericw-tools` (allowing for much greater lightmap quality)
- Client game, server game and user interface modules in QC which are (re)loaded only when needed
- Obscure features and file formats were removed including IPX protocol, CDAudio and `WAL, MD2, SP2` file formats
- Player movement code (`pmove`) was moved from client exe to cg/svg qc modules which allows mods to write new physics code while retaining prediction
- Plenty of bug fixes
- Not. Even. A. Single. Dependency. - everything is buildable out of the box in VS2019 without the need to configure anything
- Supports multiple bitmap fonts with proper glyph sizes


## Other notable planned changes:
- usage of bspx lumps lightgrids, normals and decoupledlm
- proper linux support (pragma runs fine in Wine)
- native 64bit binaries

## Directory overview:

| Dir            | what should be there?                         |
|----------------|-----------------------------------------------|
| `src`          | pragma's C source code and VC2019 projects    |
| `progs_src`    | QC script sources for game, cgame and menus   |
| `build`        | this is where exes and dll will be copied to  |
| `build/main`   | where pragma specific assets should be put    |
| `stuff`        | contains dev textures and netradiant gamepack |


## Build instructions:
Currently the only supported platform is Windows and the project build *must* be set to `x86`, while this is quite unfortunate, this project is based on id's original code release with outdated platform targets that were cut.
In future the project should move to GLFW or SDL, allowing pragma to be built for linux and other platforms.

You'll need Visual Studio 2019 or Visual Studio 2022 to compile the engine (engine.exe) and renderer (renderer_gl2.dll) projects, and FTEQCC to compile QC scripts (included in this repo).

1. Download ZIP or clone this repo.

2. Open VC2019 solution `src/pragma.sln` and compile every project for `x86`

> If compilation is succesfull, the `engine.exe`, `pragma_server.exe` and `renderer_ogl1.dll` will be created in `/build` directory.

4. Run `compile_cgame.bat`, `compile_svgame.bat` and `compile_gui.bat` to compile all QC scripts.. too much clicking? then use 'compile_allprogs.bat' :)

> These scripts compile QC with FTEQCC and copy compiled binaries to gamedir

5. Run pragma with `run_pragma.bat` :)
