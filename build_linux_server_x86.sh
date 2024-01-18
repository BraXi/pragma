#!/bin/bash

# -------------------------------
# pragma linux dedicated server
# -------------------------------

COMPILER="gcc"
CFLAGS="-c -O2 -m32"

LINKER="gcc"
LFLAGS="-O2 -s -m32 -L/usr/lib32/"
LIBS="-pthread -lm"

OUTFILE="./pragma_server"

# Source files
SRC[0]=./src/client/cgame/cg_builtins
SRC[1]=./src/qcommon/navigation
SRC[2]=./src/qcommon/cmd
SRC[3]=./src/qcommon/cmodel
SRC[4]=./src/qcommon/common
SRC[5]=./src/qcommon/crc
SRC[6]=./src/qcommon/cvar
SRC[7]=./src/qcommon/files
SRC[8]=./src/qcommon/md4
SRC[9]=./src/qcommon/net_chan
SRC[10]=./src/qcommon/shared
SRC[11]=./src/script/scr_builtins_math
SRC[12]=./src/script/scr_debug
SRC[13]=./src/script/scr_exec
SRC[14]=./src/script/scr_main
SRC[15]=./src/script/scr_builtins_shared
SRC[16]=./src/script/scr_utils
SRC[17]=./src/server/sv_ai
SRC[18]=./src/server/sv_devtools
SRC[19]=./src/server/sv_load
SRC[20]=./src/server/sv_gentity
SRC[21]=./src/server/sv_physics
SRC[22]=./src/server/sv_script
SRC[23]=./src/server/sv_ccmds
SRC[24]=./src/server/sv_builtins
SRC[25]=./src/server/sv_write
SRC[26]=./src/server/sv_init
SRC[27]=./src/server/sv_main
SRC[28]=./src/server/sv_send
SRC[29]=./src/server/sv_user
SRC[30]=./src/server/sv_world
SRC[31]=./src/platform/linux_net
SRC[32]=./src/platform/linux_shared
SRC[33]=./src/platform/linux_main

#clear

echo ==========================================
echo Compiling...
echo Flags: $CFLAGS
echo ==========================================

rm ./linux_obj/*.o

CONTINUE=1
OBJS=""

# Compilation
for file in ${SRC[*]}
do
	if [ $CONTINUE == 1 ]
	then
    		OUT_NAME=`basename $file`
    		OBJS="$OBJS ./linux_obj/$OUT_NAME.o"
    		echo $COMPILER $CFLAGS ${file}.c -o ./linux_obj/${OUT_NAME}.o

    		CONTINUE=0

    		$COMPILER $CFLAGS ${file}.c -o ./linux_obj/${OUT_NAME}.o && CONTINUE=1
	fi
done


# Linking
if [ $CONTINUE == 1 ]
then
	clear
	echo ==========================================
	echo Linking...
	echo Flags: $LFLAGS
	echo ==========================================
	echo $LINKER ${OBJS} $LFLAGS $LIBS -o $OUTFILE

	CONTINUE=0
	$LINKER $LFLAGS ${OBJS} $LIBS -o $OUTFILE && CONTINUE=1
fi


# Launch
if [ $CONTINUE == 1 ]
then
	#clear
	rm ./linux_obj/*.o
	echo ==========================================
	echo Success!
	echo Executing $OUTFILE ...
	echo ==========================================
	$OUTFILE
fi

