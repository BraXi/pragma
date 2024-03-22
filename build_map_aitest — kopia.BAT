cd ./tools/ericw-tools

set gamedir=main
set mapfile=bench

qbsp -qbism ../../build/%gamedir%/maps/%mapfile%
vis -threads 8 ../../build/%gamedir%/maps/%mapfile%
light -threads 8 -extra -bounce -dirt -world_units_per_luxel 8 ../../build/%gamedir%/maps/%mapfile%

pause

