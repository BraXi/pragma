cd ./tools/ericw-tools

set gamedir=main
set mapfile=sewerjam1_doomshakalaka_full

qbsp -qbism ../../build/%gamedir%/maps/%mapfile%
vis -threads 8 ../../build/%gamedir%/maps/%mapfile%
light -threads 8 -extra -bounce -dirt ../../build/%gamedir%/maps/%mapfile%

pause

