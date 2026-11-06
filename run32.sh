#!/bin/bash
# Runs a 32-bit Linux binary using user-space i386 libs (no root needed)
R=$HOME/i386
exec $R/lib/ld-linux.so.2 --library-path "$R/lib/i386-linux-gnu:$R/usr/lib/i386-linux-gnu:$(dirname "$1")" "$@"
