# pragma
Pragma is an custom Quake2 derived engine with strong focus on QC scripting and modding capabilities, due to heavy redesign it is not compatible with vanilla Q2 and by design _it will not run Q2 game_.

This is the engine repo, for QC code go to https://github.com/BraXi/pragma_progs

BE WARNED. THIS IS HEAVILY UNDER CONSTRUCTION AND IN EXPERIMENTAL STAGE AND WILL CHANGE DRASTICALLY FROM TIME TO TIME


Pros:
- its super simple even for newbies
- Q1 modders will instantly feel like at home
- very scriptable and moddable without touching C code
- runs well even on Intel N3450 (Acer Travelmate Spin B is the slowest hardware I had offhand to test)
- could serve as a platform for standalone SP/COOP/MP games
- *has rotating brushes, origin brushes, colored static lights and infinite hull sizes out of the box* >:) <-- Q1 modders will understand, you don't need to

Cons:
- it's not ready

So far, some of the changes include:

- adds QuakeC VM (.qc) server and client side scripts
- has no software renderer (but nothing stops you from using ref_soft.dll if you wish to import, adopt & compile one)
- game .dll API has been removed completly (there were plans to keep it alongside QC, but ended not)
- has its own QC runtime compiler - edit your script, reload map and see changes instantly! (experimental, for more advanced QC syntax you may want to compile progs with fteqcc instead)
- it wants to convice you to animate models without tricky .frame manipulation (planned CL interp and prediction)
- no 8bit textures, no palettes, no cdaudio (who has dvd drives these days?) and the only supported image/texture format is currently .TGA
- small minor bugfixes, but a few galaxies away from being as stable as Yamagi Quake II (pragma is based on original Q2 source release, remember)

Planned changes:
- fix bugs and... even more bugfixes
- increase BSP and network protocol limits
- linux & dedicated server support (currently gutted as it didn't compile anyway)
- move menus to QC or separate menu files (D3 or RTCW style maybe?)
- port modern renderers
- and more.


Think of this project as if Carmack didn't remove QC somewhere during 1997's Q2 development but expanded it greatly.

One day I'll write down a detailed list of changes ;)

![pragma](https://user-images.githubusercontent.com/6434152/235779239-bb43cdc2-3564-429c-a901-690bd49e5f3c.jpg)
