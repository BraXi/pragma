cd ./tools/ericw-tools

set gamedir=main
set mapfile=sewerjam1_doomshakalaka_full

qbsp -qbism ../../build/%gamedir%/maps/%mapfile%
#vis -threads 4 ../../build/%gamedir%/maps/%mapfile%
light -threads 8 -dirt ../../build/%gamedir%/maps/%mapfile%

pause

