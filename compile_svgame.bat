@echo off
 
echo ***************************
echo   pragma - server game qc
echo ***************************

fteqcc64 -src ./progs_src/svgame
move svgame.dat ./build/main/progs/

pause