set -x
BUILDDIR=build/temp.android-$ANDROID_VERSION-$ANDROID_ABI-$PYTHON_VERSION
PREFIX=build/lib.android-$ANDROID_VERSION-$ANDROID_ABI-$PYTHON_VERSION

mkdir -p $BUILDDIR
mkdir -p $PREFIX

$CC \
-Wno-unused-result -Wsign-compare -Wunreachable-code -fno-common -dynamic -DNDEBUG -g -fwrapv -O3 -Wall -fPIC \
--sysroot=$SYSROOT \
-I$SYSROOT/usr/include \
-I$ANDROID_PREFIX/include \
-I$PYTHON_INCLUDE_DIR/python3.9d \
-I/Users/$USER/devel/.sx/androidframeworks/$ANDROID_ABI/include \
-c pynodegl.c \
-o $BUILDDIR/pynodegl.o && \
\
$CC \
-shared \
-L$SYSROOT/usr/lib \
-L$ANDROID_PREFIX/lib \
--sysroot=$SYSROOT \
-I$SYSROOT/usr/include \
-I$ANDROID_PREFIX/include \
-I$PWD/../nodegl-env/include \
$BUILDDIR/pynodegl.o \
$PYTHON_LIB_DIR/libpython3.9d.a -lm \
-L/Users/$USER/devel/.sx/androidframeworks/$ANDROID_ABI/lib -lnodegl -lEGL -landroid -lsxplayer -lavcodec -lavformat -lavfilter \
-L${ANDROID_NDK}/platforms/android-${ANDROID_VERSION}/arch-${ABI_1}/usr/${LIB_DIR_0} \
-o $PREFIX/pynodegl.cpython-39.so
