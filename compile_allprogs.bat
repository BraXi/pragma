@echo off

echo ***************************
echo   pragma - client game qc
echo ***************************

fteqcc64 -src ./progs_src/cgame
move cgame.dat ./build/main/progs/

echo ***************************
echo   pragma - server game qc
echo ***************************

fteqcc64 -src ./progs_src/svgame
move svgame.dat ./build/main/progs/

pause