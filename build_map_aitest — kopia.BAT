cd ./tools/ericw-tools/bin

set gamedir=main
set mapfile=bench

qbsp -q2bsp -allowupgrade 0  ../../../build/%gamedir%/maps/%mapfile%
vis -threads 4 ../../../build/%gamedir%/maps/%mapfile%
light -threads 4 -bounce -extra -dirt ../../../build/%gamedir%/maps/%mapfile%

