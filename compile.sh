#!/bin/bash

BINDIR="bin"
NAME="$BINDIR/calc"

NAME_HOTCODE="$BINDIR/hotcode.so"

SRCDIR="src"
SRCS=$(find src/ -type f -name "*.c*")

HOTCODE="src/hotcode.c"
HOTCODE_FLAG="-fPIC -shared"

COOLCODE=${SRCS//"$HOTCODE"/}

COMPILER="clang -fcolor-diagnostics"
COMPILE_FLAGS="-Wall -Wextra"
INCLUDES="-Iinclude -lm -lncurses"
DEBUG_FLAG="-g -DDEBUG"

$COMPILER $HOTCODE_FLAG $SLOWMODE $COMPILE_FLAGS $DEBUG_FLAG $HOTCODE -o $NAME_HOTCODE $INCLUDES

$COMPILER $SLOWMODE $COMPILE_FLAGS $DEBUG_FLAG $COOLCODE -o $NAME $INCLUDES
