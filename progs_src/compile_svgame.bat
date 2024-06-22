@echo off
 
echo ***************************
echo   pragma - server game qc
echo ***************************

fteqcc64 -src ./svgame
move svgame.dat ../build/main/progs/

pause