#!/bin/bash

VCPKG_DIR_WSL=$(wslpath $VCPKG_DIR)
VCPKG_DIR_NORMALIZED=$(echo $VCPKG_DIR | sed 's/\\/\//g')
prefix=$VCPKG_DIR_NORMALIZED/installed/x64-windows

# patch pkg-config files: convert cygwin paths to windows paths
find $VCPKG_DIR_WSL/installed/x64-windows/lib/pkgconfig -name '*.pc' | xargs -I{} sed -i 's/\/cygdrive\/\([^\/]\)/\1:/g' {}
