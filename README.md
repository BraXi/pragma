# pragma
Pragma is an custom Quake2 derived engine with strong focus on QC scripting and modding capabilities, due to heavy redesign it is not compatible with vanilla Q2 and by design _it will not run Q2 game_.

BE WARNED. THIS IS HEAVILY UNDER CONSTRUCTION AND IN EXPERIMENTAL STAGE AND WILL CHANGE DRASTICALLY FROM TIME TO TIME


Pros:
- its super simple even for newbies
- you don't need Quake 2 game files to start - it's completly stand alone
- Q1 modders will instantly feel like at home that's been
- very scriptable and moddable without touching C code
- runs well even on Intel N3450 (Acer Travelmate Spin B is the slowest hardware I had offhand to test)
- could serve as a platform for standalone SP/COOP/MP games
- *has rotating brushes, origin brushes, colored static lights and infinite hull sizes out of the box* >:) <-- Q1 modders will understand, you don't need to

Cons:
- it's not ready

So far, some of the changes include:

- adds QuakeC VM (.qc) server and client side scripts
- fixed many typos and corrected error messages to be understandable and explainatory
- has no software renderer and OGL is currently the one and only, but nothing stops you from using ref_soft.dll if you wish to import, adopt & compile one
- Game (.DLL) API has been removed completly
- small minor bugfixes, but a few galaxies away from being as stable as Yamagi Quake II (pragma is based on original Q2 source release, remember)
- removed obscure CDAudio and IPX support

Planned changes:
- no 8bit textures, no palettes
- increase BSP and network protocol limits
- linux & dedicated server support
- move menus to QC or separate menu files (D3 or RTCW style maybe?)
- port modern renderers
- and more.

Think of this project as if Carmack didn't remove QC somewhere during 1997's Q2 development but expanded it greatly.

One day I'll write down a detailed list of changes ;)

![shot009](https://github.com/BraXi/pragma/assets/6434152/f586402a-8bd5-405e-a9ff-9c8cd30deb5c)
