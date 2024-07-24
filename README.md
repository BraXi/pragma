### pragma - the improved IdTech2 engine derived from Quake II aimed at standalone projects.


[![Build Status](https://github.com/BraXi/pragma/actions/workflows/msbuild.yml/badge.svg)](https://github.com/BraXi/pragma/actions/workflows/msbuild.yml) [![Build Status](https://github.com/BraXi/pragma/actions/workflows/msbuild-release-x86.yml/badge.svg)](https://github.com/BraXi/pragma/actions/workflows/msbuild-release-x86.yml)

> #1 YOU CANNOT PLAY QUAKE 2 WITH IT. \
> #2 Things change and break _quite often_ \
> #3 Linux (and *insert your OS of choice*) will be fully suported when interested parties contribute and maintain code to make it work


## Quick Overview of changes
- Zero dependencies on external C libraries - No need for cmake or any of that crap, compiles with Visual Studio out of the box
- QuakeC Virtual Machines for game code, client game and GUI
- It's completly standalone and does not require Quake 2 data
- Speedy OpenGL 2.1 renderer with GLSL programmed pipeline
- Full support for `MD3` vertex morph models including groups, tags and per-surf textures
- (Partial as of writing) support for skeletal animated `SMD` models
- Improved network protocol, no longer limited to 4096qu coordinates in any direction, etc
- Dynamic per pixel point and spot lights
- Support for higher resolution lightmaps via `DECOUPLED_LM` bspx extension for BSPs compiled with `ericw-tools`
- Supports extended `QBISM` BSP format which removes most of the limitations of the Q2 BSP, maps can be gigantic and richer in quality (use `-qbism` switch for ericw-tools qbsp program)
- Post procesing effects (blur, contrast, gamma, grayscale...) and more
- Server (thus all entities) can run at `40, 20 or 10` ticks per second (Q2 is at 10)
- Dedicated server binary (only for windows and linux with 32bit libs)
- All textures are RGB(A) `TGA` format and are no longer limited to `256 x 256px` and no color palette
- Model, sound and image limits have been significantly raised and can be configured in `pragma_config.h`
- Player movement code (`pmove`) was moved from client exe to cg/svg qc modules, thus mods can freely modify physics code while retaining prediction and not needing to ship their own client binaries
- Obscure features and file formats were removed including IPX protocol, CDAudio, ConProc and `WAL, MD2, SP2` file formats
- Animations can be independant from server ticks and play at desired FPS, although old method of animating via `.frame` still works
- Plenty of bug fixes
- Supports multiple bitmap fonts with proper glyph sizes
- Trigger entities can use inline models for perfect touch detecton
- Astar pathfinding

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

