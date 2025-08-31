#!/bin/bash

# Suppress Qt XKB compose table warnings
export QT_XKB_CONFIG_ROOT=""
export QT_LOGGING_RULES="qt.xkb.compose.debug=false"

./build/loom "$@"