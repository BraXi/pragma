@echo off

echo ***************************
echo   pragma - client game qc
echo ***************************

fteqcc64 -src ./progs_src/cgame
move cgame.dat ./build/main/progs/

pause