#!/bin/bash

# The plugin directory name CANNOT contain any dashes!!!
mkdir -p build/lv2/tinyaudio.lv2/
gcc -std=c99 -Ilibs/lv2/include -fPIC -shared -g -O0 -o build/lv2/tinyaudio.lv2/plugin.so ./src/platform/i_lv2.c
cp src/platform/lv2/**.ttl build/lv2/tinyaudio.lv2/.
