#!/bin/sh

git submodule update --init --recursive
bear -- ./scripts/build.sh
