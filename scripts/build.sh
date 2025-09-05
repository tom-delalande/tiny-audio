#!/bin/sh

set -e

mkdir -p build

PROJECT_NAME="clap-plugin"

CFLAGS="-shared -g -Wno-unused-parameter --std=c99 -Wall -Werror"
LFLAGS=" -Ilibs/clap/include -Ilibs/lv2/include"

if [ "$DEBUG" = "1" ]; then
  CFLAGS="$CFLAGS -g -O0"
fi

clang $CFLAGS $LFLAGS -o build/$PROJECT_NAME.clap src/main.c
