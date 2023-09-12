# pragma
Pragma is an custom Quake2 derived engine with strong focus on QC scripting and modding capabilities, due to heavy redesign it is not compatible with vanilla Q2 and by design _it will not run Q2 game_.

The most drastical change is the complete removal of game dll (the Q2's C modding API), which was replaced with QuakeC virtual machine, compiled 'progs' will run across all supported operating systems and are being overall more secure, allowing users to run various mods and not worry about any malicious code.

The second (in progress), yet very important change is the implementation of client game module, which introduces client-side QC scripting that will allow mods to implement new effects, menus, prediction, etc - untill now this could only be done by shipping custom builds of client EXE and is totally not the right way for mods

BE WARNED. THIS IS HEAVILY UNDER CONSTRUCTION AND IN EXPERIMENTAL STAGE AND WILL CHANGE DRASTICALLY FROM TIME TO TIME

## Features:
Pragma so far had introduced following additions and changes to Q2 engine:
- QuakeC Virtual Machine (.qc) allowing server (and soon client) code to be completly platform independant and very simple
- fixed many typos in code and corrected error messages to be [more] understandable and explainatory
- has no software renderer and OpenGL 1.x is currently the one and only
- Game (.DLL) API has been removed completly
- small minor bugfixes, but a few galaxies away from being as stable as Yamagi Quake II (pragma is based on original Q2 source release, remember)
- removed obscure CDAudio and IPX support
- its super simple even for newbies - all you need to start is a text editor and FTEQCC compiler to get started
- new UI (HUD/MENU) system (in works)

In vanilla Q2 anytime you changed something in game.dll the code needed to be compiled and quake2 restarted (which took some time), in pragma you can recompile scripts on the fly and your changes will be present when you reload a map


## Planned changes:
- complete removal of color palette and 8bit textures
- increase BSP and network protocol limits
- linux & dedicated server support
- move menus to QC or separate menu files (D3 or RTCW style maybe?)
- port modern renderer (Vulkan or OGL4+)


## Directory overview:

| Dir            | what should be there?                         |
|----------------|-----------------------------------------------|
| `src`          | pragma's C source code and VC2019 projects    |
| `progs_src`    | QC script sources for game and cgame          |
| `build`        | this is where exe and dll's will be copied to |
| `build/basepr` | where pragma specific assets should be put    |
| `build/baseq2` | where you should copy Q2's `pak0.pak`         |


## Build instructions:
Currently the only supported platform is Windows and the project build *must* be set to `x86`, while this is quite unfortunate, this project is based on id's code release with outdated platform targets that were cut.
In future the project should move to GLFW or SDL, allowing pragma to be built for linux and other platforms.

You'll need Visual Studio 2019 or Visual Studio 2022 to compile the engine (engine.exe) and renderer (renderer_ogl1.dll) projects, and FTEQCC to compile QC scripts.

1. Download ZIP or clone this repo.

2. Open VC2019 solution `src/pragma.sln` and compile every project for `x86`

If compilation is succesfull, the `engine.exe` and `renderer_ogl1.dll` will be created in `/build` directory.

4. Run `build_scripts_server.bat` to compile server QC scripts

This script runs FTEQCC in `/progs_src` and copies the compiled QC progs to `build/basepr/progs`

5. Copy over Quake 2's retail or demo `pak0.pak` to `build/baseq2/`

6. Run pragma with `run_pragma.bat` (alternatively start engine.exe with cmdline `+game basepr`)

https://www.youtube.com/watch?v=rLWEsG0bD44


# Screenshots and videos of pragma in action

players with diferent models and anims

[![pragma_players](https://img.youtube.com/vi/rLWEsG0bD44/0.jpg)](https://www.youtube.com/watch?v=rLWEsG0bD44)

some random physics test

[![pragma_players](https://img.youtube.com/vi/mbXaEDZLZgE/0.jpg)](https://www.youtube.com/watch?v=mbXaEDZLZgE)

new UI system in action (not on git yet)

![shot031](https://github.com/BraXi/pragma/assets/6434152/09d30811-2681-418c-8036-bb8622c35fb9)


goofing around, QC profiler displayed on the right

![shot009](https://github.com/BraXi/pragma/assets/6434152/f586402a-8bd5-405e-a9ff-9c8cd30deb5c)
