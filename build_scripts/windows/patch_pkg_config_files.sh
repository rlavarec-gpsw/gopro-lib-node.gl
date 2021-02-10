#!/bin/bash

VCPKG_DIR_WSL=$(wslpath $VCPKG_DIR)
VCPKG_DIR_NORMALIZED=$(echo $VCPKG_DIR | sed 's/\\/\//g')
prefix=$VCPKG_DIR_NORMALIZED/installed/x64-windows

# Export spirv-cross pkg-config file

read -r -d '' SPIRV_CROSS_PKG_CONFIG_CONTENTS << EOM
prefix=$prefix
exec_prefix=${prefix}
libdir=${prefix}/lib
includedir=${prefix}/include

Name: spirv-cross
Description: SPIRV-Cross
Version: 2020-02-26

Requires:
Libs: -L\${libdir} -lspirv-cross-cpp -lspirv-cross-core -lspirv-cross-glsl -lspirv-cross-hlsl -lspirv-cross-msl -lspirv-cross-reflect -lspirv-cross-util -lspirv-cross-c
Cflags: -I\${includedir}

EOM

echo "$SPIRV_CROSS_PKG_CONFIG_CONTENTS" > $VCPKG_DIR_WSL/installed/x64-windows/lib/pkgconfig/spirv-cross.pc

# patch pkg-config files: convert cygwin paths to windows paths
find $VCPKG_DIR_WSL/installed/x64-windows/lib/pkgconfig -name '*.pc' | xargs -I{} sed -i 's/\/cygdrive\/\([^\/]\)/\1:/g' {}
