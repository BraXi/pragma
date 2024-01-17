# pragma

[![Build Status](https://github.com/BraXi/pragma/actions/workflows/msbuild.yml/badge.svg)](https://github.com/BraXi/pragma/actions/workflows/msbuild.yml) [![Build Status](https://github.com/BraXi/pragma/actions/workflows/msbuild-release-x86.yml/badge.svg)](https://github.com/BraXi/pragma/actions/workflows/msbuild-release-x86.yml)

Pragma is an highly modified and upgraded IdTech2 engine with strong focus on QC scripting and is entirely aimed at standalone projects.


> THIS IS NOT ANOTHER SOURCE PORT AND YOU CANNOT PLAY QUAKE 2 WITH IT. 
>
> IT IS HEAVILY UNDER CONSTRUCTION AND IN EXPERIMENTAL STAGE, THAT MEANS DRASTIC CHANGES TO CODE AND APIs HAPPEN

The biggest change is the complete removal of native game code (the Q2's game library), which was replaced with QuakeC VM, that is QC programs run across all supported operating systems and are being more secure, allowing users to run various mods and not worry about any malicious code.

Along with server progs, it does also introduce QC scripting for client-game and GUIs.

Project has a simple rule - *no bloat*, all the changes and additions should not add unnecessary complexity, no plethora of fileformats and very limited dependencies to the code, the smaller the code is, the better.

I'm trying to avoid including any unnecessary dependencies to the point where you can compile it out of the box with Visual Studio 2019 and 2022 editions.


## pragma vs. IdTech2 (Quake2) comparision, goals and current state
(* means the initial work has been done but its not usable in current form or incomplete, ** feature will be removed completly, *** feature will be implemented or needs to be reimplemented because current implementation is bad)
| Feature            | pragma | idTech2 (Q2) |
|----------------|--------|--------------|
| Server game | SV QuakeC | Native library |
| Client game | CL QuakeC | Hardcoded in engine |
| GUI (menus, hud)| GUI QuakeC | Hardcoded in engine |
| Pmove | In QC scripts | Hardcoded in engine |
| Renderers | OpenGL 1.3 | OpenGL 1.1, Software |
| Color palette | RGBA | colormap dependant (256 colors)|
| Image formats | TGA | TGA (sky only), PCX, WAL |
| Model formats | MD3, SP2 | MD2, SP2 |
| Map format | Q2 BSP V.38*** | Q2 BSP V.38 |
| IPX | No, nobody uses it | Yes |
| CDAudio | No, who still has cd drive? | Yes |
| Cinematics | No*** | Yes (.cin format) |
| savegames | No*** | Yes (prone to bugs, OS dependant) |
| Asset limits (net) | up to 32768 models and sounds, 256 gui images | 256 models, 256 sounds, 256 images |
| Renderer limits | raised by a lot oob ;) | 32 dlights, 128 models, 4096 particles |


## Features:
Pragma so far had introduced following additions and changes to Q2 engine:
- It's completly standalone, does not require Quake2 assets
- completly platform independant QuakeC Virtual Machine instead of native game library
- dedicated server binary (currently windows only)
- Improved network protocol -- higher asset limits, full precision float coordinates, more control over entities compared to Q2
- server-side game, client-side game and GUI QC programs
- MD3 models with raised limits
- updated OpenGL renderer -- complete removal of color palette, 8bit textures and ancient hardware releated code
- lots of bug fixes and improvments to logic where necessary
- removed obscure "features" and ancient code, including CDAudio, IPX, qhost, conproc and more...
- its super simple even for newbies - all you need is a text editor and FTEQCC compiler to create your first mod, any change to QC programs will be instantly loaded by reloading map


## Other notable planned changes:
- support for BSPs with increased limits, lightgrids, vertexnormals and lightmapped turb (liquid) surfaces
- linux support
- modern renderer (Vulkan or OGL4+)
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

You'll need Visual Studio 2019 or Visual Studio 2022 to compile the engine (engine.exe) and renderer (renderer_ogl1.dll) projects, and FTEQCC to compile QC scripts (included in this repo).

1. Download ZIP or clone this repo.

2. Open VC2019 solution `src/pragma.sln` and compile every project for `x86`

> If compilation is succesfull, the `engine.exe`, `pragma_server.exe` and `renderer_ogl1.dll` will be created in `/build` directory.

4. Run `compile_cgame.bat`, `compile_svgame.bat` and `compile_gui.bat` to compile all QC scripts.. too much clicking? then use 'compile_allprogs.bat' :)

> These scripts compile QC with FTEQCC and copy compiled binaries to gamedir

5. Run pragma with `run_pragma.bat` :)


# pragma in action






