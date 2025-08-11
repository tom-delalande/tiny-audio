#!/bin/sh

./scripts/clean.sh
DEBUG=1 ./scripts/build.sh
./scripts/move.sh
