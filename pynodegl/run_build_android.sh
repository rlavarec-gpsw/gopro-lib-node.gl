set -x
source ~/Documents/2020/Git/python-build/build_scripts/android/cross-env.sh
export PYTHON_DIR=~/Documents/2020/Git/python-build/build-android/$ANDROID_ABI
export PYTHON_INCLUDE_DIR=$PYTHON_DIR/include
export PYTHON_LIB_DIR=$PYTHON_DIR/lib
export PYTHON_VERSION=3.9.4
bash build_android.sh
