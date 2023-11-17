# pragma
Pragma is an highly modified and upgraded IdTech2 engine with strong focus on QC scripting and a clean base for standalone projects.
It does not require Quake2 game data to launch.


> THIS IS NOT ANOTHER SOURCE PORT AND YOU CANNOT PLAY QUAKE 2 WITH IT. 
>
> IT IS HEAVILY UNDER CONSTRUCTION AND IN EXPERIMENTAL STAGE, THAT MEANS DRASTIC CHANGES TO CODE AND APIs HAPPEN

The biggest change is the complete removal of native game code (the Q2's game library), which was replaced with QuakeC VM, that is QC programs run across all supported operating systems and are being more secure, allowing users to run various mods and not worry about any malicious code.

The second, yet very important change is the implementation of client game module, which introduces client-side QC scripting that allows mods to implement new effects, menus, player physics and much more beyond what could be changed in Q2 mods.

Project has a simple rule - *no bloat*, all the changes and additions should not add unnecessary complexity, no plethora of fileformats and very limited dependencies to the code, the smaller the code is, the better.

## pragma vs. IdTech2 (Quake2) comparision, goals and current state
(* means the initial work has been done but its not usable in current form or incomplete, ** feature will be removed completly, *** feature will be implemented or needs to be reimplemented because current implementation is bad)
| Feature            | pragma | idTech2 (Q2) |
|----------------|--------|--------------|
| Server game | SVQuakeC progs | Native library |
| Client game | CLQuakeC progs | Hardcoded in engine |
| GUI (menus, hud)| In QC scripts*** | Hardcoded in engine |
| Pmove | In QC scripts | Hardcoded in engine |
| Renderers | OpenGL 2.1 with shaders*** | OpenGL 1.1, Software |
| Color palette | RGBA | colormap.pcx dependant (256 colors)|
| Image formats | TGA | TGA (sky only), PCX, WAL |
| Model formats | MD3, SP2 | MD2, SP2 |
| Map format | Q2 BSP V.38*** | Q2 BSP V.38 |
| IPX | No, nobody uses it | Yes |
| CDAudio | No, who still has cd drive? | Yes |
| Cinematics | No*** | Yes (.cin format) |
| savegames | No*** | Yes (prone to bugs, OS dependant) |
| Game "hot reload" | Recompile QC and restart map | No, needs engine restart |
| Asset limits (net) | up to 32768 models and sounds, 256 gui images | 256 models, 256 sounds, 256 images |
| Renderer limits | 64 dlights, 512 models, 8192 particles | 32 dlights, 128 models, 4096 particles |


## Features:
Pragma so far had introduced following additions and changes to Q2 engine:
- completly platform independant QuakeC Virtual Machine instead of native game library
- It's completly standalone, does not require Quake2 assets
- server-side, client-side and menu QC
- MD3 models
- updated OpenGL renderer with multitexture and (soon) glsl shaders
- complete removal of color palette and 8bit textures in favour of TGA format that supports full RGBA
- small minor bugfixes, but a few galaxies away from being as stable as Yamagi Quake II or any other mature source port (pragma is based on original Q2 source release, remember)
- removed obscure CDAudio and IPX code
- its super simple even for newbies - all you need is a text editor and FTEQCC compiler to create your first mod

In vanilla Q2 anytime you changed something in game.dll the code needed to be compiled and quake2 restarted (which took some time), in pragma you can recompile scripts on the fly and your changes will be present when you reload a map


## Other notable planned changes:
- support for BSPs with increased limits, lightgrids, vertexnormals and lightmapped turb (liquid) surfaces
- linux & dedicated server support
- modern renderer (Vulkan or OGL4+)
- rewrite of net parsing of certain packets like ``svc_muzzleflash`` and ``svc_temp_entity``, parsing being a lot more versatile and engine less likely crash when something unexpected happens or when server asks for something client doesn't have

## Directory overview:

| Dir            | what should be there?                         |
|----------------|-----------------------------------------------|
| `src`          | pragma's C source code and VC2019 projects    |
| `progs_src`    | QC script sources for game, cgame and menus    |
| `build`        | this is where exe and dll's will be copied to |
| `build/main`   | where pragma specific assets should be put    |
| `stuff`        | contains dev textures and netradiant gamepack |


## Build instructions:
Currently the only supported platform is Windows and the project build *must* be set to `x86`, while this is quite unfortunate, this project is based on id's original code release with outdated platform targets that were cut.
In future the project should move to GLFW or SDL, allowing pragma to be built for linux and other platforms.

You'll need Visual Studio 2019 or Visual Studio 2022 to compile the engine (engine.exe) and renderer (renderer_ogl1.dll) projects, and FTEQCC to compile QC scripts (included in this repo).

1. Download ZIP or clone this repo.

2. Open VC2019 solution `src/pragma.sln` and compile every project for `x86`

> If compilation is succesfull, the `engine.exe` and `renderer_ogl1.dll` will be created in `/build` directory.

4. Run `build_game.bat`, `build_cgame.bat` and `build_gui.bat` to compile all QC scripts.. too much clicking? then use 'build_all.bat' :)

> These scripts compile QC with FTEQCC and copy compiled binaries to gamedir

5. Run pragma with `run_pragma.bat` :)


# pragma in action

[![pragma_wpnlogic](https://img.youtube.com/vi/aveXkVqTDmQ/0.jpg)](https://www.youtube.com/watch?v=aveXkVqTDmQ)





