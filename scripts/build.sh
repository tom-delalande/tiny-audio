#!/bin/sh

set -e

mkdir -p build

PROJECT_NAME="clap-plugin"

if [ "$DEBUG" = "1" ]; then
  CFLAGS="$CFLAGS -g -O0"
fi

CFLAGS="-Wall -Werror"
LFLAGS="-shared -g -Wno-unused-parameter --std=c99 -Ilibs/clap/include"

clang $CFLAGS $LFLAGS -o build/$PROJECT_NAME.clap src/main.c
