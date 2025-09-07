#!/bin/sh
set -e

./scripts/clean.sh
./scripts/build.sh
./scripts/move.sh
