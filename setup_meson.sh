#!/bin/sh

meson setup --wipe builddir -Denable-tests=true

# ninja -C builddir
