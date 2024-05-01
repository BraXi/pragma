### pragma - the improved IdTech2 engine derived from Quake II aimed at standalone projects.


[![Build Status](https://github.com/BraXi/pragma/actions/workflows/msbuild.yml/badge.svg)](https://github.com/BraXi/pragma/actions/workflows/msbuild.yml) [![Build Status](https://github.com/BraXi/pragma/actions/workflows/msbuild-release-x86.yml/badge.svg)](https://github.com/BraXi/pragma/actions/workflows/msbuild-release-x86.yml)

> #1 YOU CANNOT PLAY QUAKE 2 WITH IT. \
> #2 Things change and break _quite often_ \
> #3 Linux (and *insert your OS of choice*) will be fully suported when interested parties contribute and maintain code to make it work


## Quick Overview of changes
- Native Game DLL replaced with platform independant QuakeC Virtual Machine
- It's completly standalone and does not require Quake 2 data
- OpenGL 2.1 renderer which offloads much of the work to GPU
- Full support for `MD3` models including groups, tags and per-surf textures
- Per pixel lighting, GLSL shaders, post process effects (blur, contrast, gamma, grayscale...) and more
- Dedicated server binary (currently x86 windows + linux)
- All textures are RGB(A) `TGA` format and are no longer limited to `256 x 256px` and no color palette
- Improved network protocol no longer limited to 4096qu coordinates in any direction
- Model, sound and image limits have been significantly raised
- Supports extended Quake 2 BSP format (qbism) which practicaly removes most of the limitations of the level format, levels can be bigger and richer in quality
- Decoupled lightmap support for maps compiled with `ericw-tools` allowing for much greater lightmap quality
- Client game, server game and user interface modules in QuakeC which are (re)loaded only when needed and completly secure
- User interface system with GUI scripting, mouse input and automatic scaling
- Obscure features and file formats were removed including IPX protocol, CDAudio, ConProc and `WAL, MD2, SP2` file formats
- Player movement code (`pmove`) was moved from client exe to cg/svg qc modules, thus mods can freely modify physics code while retaining prediction and not needing to ship their own client binaries
- Animations can be independant from server ticks and play at desired FPS, although old method of animating via `.frame` still works
- Server (thus all entities) can run at `40, 20 or 10` ticks per second (Q2 is at 10)
- Plenty of bug fixes
- Not. Even. A. Single. Dependency. - everything is buildable out of the box in VS2019 without the need to configure anything
- Supports multiple bitmap fonts with proper glyph sizes
- Trigger entities can use inline models for perfect touch detecton

## Directory overview:

| Dir            | what should be there?                         |
|----------------|-----------------------------------------------|
| `src`          | pragma's C source code and VC2019 projects    |
| `progs_src`    | QC script sources for game, cgame and menus   |
| `build`        | this is where exes and dll will be copied to  |
| `build/main`   | where pragma specific assets should be put    |
| `stuff`        | contains dev textures and netradiant gamepack |


# Build instructions:

## Windows
Open `pragma.sln` in either Visual Studio 2019 or Visual Studio 2022, set target to `x86` and build all projects including `engine`, `pragma_server` and `renderer_gl2`, if everything went fine three binaries  `engine.exe`, `pragma_server.exe` and `renderer_ogl2.dll` will be created in `/build` directory.

## Linux
Only the dedicated server can be built on this platform via `gcc` and I've only tested it under Ubuntu 22.4.

Open terminal and issue commands below, if everything went fine `pragma_server` binary will be created in project's root directory, note that the compile script is rarely updated and you may need to sometimes fix it yourself
```bash
sudo apt install gcc-multilib
cd pragma
chmod +x ./build_linux_server_x86.sh
./build_linux_server_x86.sh
```
## QuakeC Modules
I strongly suggest using `fteqcc` quakec compiler bundled with pragma, just run `compile_cgame.bat`, `compile_svgame.bat` and `compile_gui.bat` to compile all QC scripts. All QC programs will land in `build/main/progs`

