cd ./tools/ericw-tools

set gamedir=main
set mapfile=test2

qbsp -qbism ../../build/%gamedir%/maps/%mapfile%
vis -threads 4 ../../build/%gamedir%/maps/%mapfile%
light -threads 4 -bounce -extra -dirt ../../build/%gamedir%/maps/%mapfile%

pause

