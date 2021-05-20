set -x
mkdir -p build/temp.android-28-arm64v8a-3.9
mkdir -p build/lib.android-28-arm64v8a-3.9

$CC \
-Wno-unused-result -Wsign-compare -Wunreachable-code -fno-common -dynamic -DNDEBUG -g -fwrapv -O3 -Wall \
--sysroot=$SYSROOT \
-I$SYSROOT/usr/include \
-I$ANDROID_PREFIX/include \
-I$SYSROOT/usr/include \
-I$ANDROID_PREFIX/include \
-I$PYTHON_INCLUDE_DIR/python3.9d \
-I/Users/jmoguillansky/devel/.sx/build-android/arm64-v8a/nodegl/libnodegl \
-c pynodegl.c \
-o build/temp.android-28-arm64v8a-3.9/pynodegl.o

$CC \
-shared \
-L$SYSROOT/usr/lib \
-L$ANDROID_PREFIX/lib \
--sysroot=$SYSROOT \
-I$SYSROOT/usr/include \
-I$ANDROID_PREFIX/include \
-I$SYSROOT/usr/include \
-I$ANDROID_PREFIX/include \
-I$PWD/../nodegl-env/include \
build/temp.android-28-arm64v8a-3.9/pynodegl.o \
$PYTHON_LIB_DIR/libpython3.9d.a -lm \
-L/Users/jmoguillansky/devel/.sx/build-android/arm64-v8a//nodegl/libnodegl -lnodegl \
-o build/lib.android-28-arm64v8a-3.9/pynodegl.cpython-39.so
