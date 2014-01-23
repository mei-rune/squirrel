#!/bin/bash
OS=`uname -s`
PLATFORM_WINDOWS="Windows_NT"
PLATFORM_LINUX="Linux"
PLATFORM_MACOSX="Darwin"
GYP=./deps/uv/build/gyp/gyp
CONFIGURATION="debug"

# Parse command line arguments
while getopts ":o:c:" option
do
    case $option in
        o)
          OS=$OPTARG
          ;;
        c)
          CONFIGURATION=$OPTARG
          ;;
        ?)
          echo "invalid option provided"
          exit
          ;;
  esac
done

echo "----------------------------------------"
echo "Cloning submodules"
echo "----------------------------------------"

# Getting Gyp build environment.
if [ ! -d "tools/gyp" ]; then
    echo "svn checkout http://gyp.googlecode.com/svn/trunk/ tools/gyp"
    svn checkout http://gyp.googlecode.com/svn/trunk/ tools/gyp
fi

if [ ! -d "deps/uv/build/gyp" ]; then
    mkdir deps/uv/build
    cp -Rf tools/gyp deps/uv/build/gyp
fi

if [ $OS = $PLATFORM_WINDOWS ]; then
    echo "----------------------------------------"
    echo "Configuring for ${OS} & Visual Studio"
    echo "----------------------------------------"
    $GYP --depth=. -Icommon.gypi -Dwithout_ssl=true -Dwithout_pthread=true -Dlibrary=static_library -Dtarget_arch=x64 --build=$CONFIGURATION squirrel.gyp
    msbuild /p:Configuration=$CONFIGURATION squirrel.sln
else
    echo "----------------------------------------"
    echo "Configuring and compiling for ${OS}"
    echo "----------------------------------------"
    $GYP --depth=. -Goutput_dir=./builds/unix -Icommon.gypi -Dlibrary=static_library --build=$CONFIGURATION -f make squirrel.gyp
fi
