#!/bin/sh
set -e

git submodule update --init --recursive
bear -- ./scripts/build.sh
