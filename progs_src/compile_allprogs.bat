@echo off

echo ***************************
echo   pragma - ui qc
echo ***************************

fteqcc64 -src ./ui
move svgame.dat ../build/main/progs/



echo ***************************
echo   pragma - client game qc
echo ***************************

fteqcc64 -src ./cgame
move cgame.dat ../build/main/progs/

echo ***************************
echo   pragma - server game qc
echo ***************************

fteqcc64 -src ./svgame
move svgame.dat ../build/main/progs/

pause