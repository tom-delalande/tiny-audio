#!/bin/bash

# The plugin directory name CANNOT contain any dashes!!!
mkdir -p build/lv2/tinyaudio.lv2/

CFLAGS="-shared -g -Wno-unused-parameter --std=c99 -Wall -Werror -fPIC"
LFLAGS=" -Ilibs/clap/include -Ilibs/lv2/include"

DEBUG=1
if [ "$DEBUG" = "1" ]; then
  CFLAGS="$CFLAGS -g -O0"
fi

gcc $CFLAGS -dynamiclib -o build/lv2/tinyaudio.lv2/plugins.dylib ./src/plugin/p_plugins.c

gcc $CFLAGS -Ilibs/lv2/include -o build/lv2/tinyaudio.lv2/passthrough.so ./src/platform/i_lv2.c
cp src/platform/lv2/**.ttl build/lv2/tinyaudio.lv2/.
