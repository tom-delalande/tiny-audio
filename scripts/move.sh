#!/bin/sh

PROJECT_NAME="clap-plugin"

CLAP_BUNDLE="build/${PROJECT_NAME}.clap"

DEST="$HOME/Library/Audio/Plug-Ins/CLAP"
mkdir -p "$DEST"
cp -R "$CLAP_BUNDLE" "$DEST/"
