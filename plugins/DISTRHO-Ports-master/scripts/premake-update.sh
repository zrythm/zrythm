#!/bin/bash

set -e

LINUX=0
MAC=0
MINGW=0

if [ "$1" = "" ]; then
  echo "usage: $0 linux|mac|mingw"
  exit
fi

if [ "$1" = "linux" ]; then
  LINUX=1
elif [ "$1" = "mac" ]; then
  MAC=1
elif [ "$1" = "mingw" ]; then
  MINGW=1
else
  echo "parameter must be linux, mac or mingw"
  exit
fi

if [ -d ../libs ]; then
  cd ..
fi

run_premake()
{
  premake --os $1 --target gnu --cc gcc

  if [ $MAC == 1 ]; then
    sed -i -e "s|BLDCMD = ar -rcs \$(OUTDIR)/\$(TARGET) \$(OBJECTS) \$(TARGET_ARCH)|BLDCMD = ar -rcs \$(OUTDIR)/\$(TARGET) \$(OBJECTS)|" `find . -name \*.make`
  else
    sed -i -e "s/\$(LDFLAGS)/\$(LDFLAGS) \$(LDFLAGS)/" `find . -name \*.make`
  fi
}

# ------------------------------------------------------------------------------------------------------------

FOLDERS="."

if [ -d libs ]; then
  FOLDERS="libs"
fi

if [ -d ports ]; then
  FOLDERS="$FOLDERS ports"
fi

FILES=`find $FOLDERS -name premake.lua`

for i in $FILES; do
  FOLDER=`echo $i | awk sub'("/premake.lua","")'`

  cd $FOLDER

  if [ $LINUX = 1 ]; then
    run_premake "linux"
  elif [ $MAC = 1 ]; then
    run_premake "macosx"
  elif [ $MINGW = 1 ]; then
    run_premake "windows"
  fi

  if [ -d ../libs ]; then
    cd ..
  elif [ -d ../../libs ]; then
    cd ../..
  elif [ -d ../../../libs ]; then
    cd ../../..
  else
    cd ../../../..
  fi
done

# ------------------------------------------------------------------------------------------------------------

if [ ! -d sdks/vstsdk2.4 ]; then
  exit 0
fi

if [ -L sdks/vstsdk2.4/pluginterfaces ]; then
  exit 0
fi

if [ -d /usr/include/pluginterfaces ]; then
  cp -r /usr/include/pluginterfaces sdks/vstsdk2.4/
fi

if [ -d /usr/include/public.sdk ]; then
  cp -r /usr/include/public.sdk sdks/vstsdk2.4/
  cd sdks/vstsdk2.4; patch -p0 < fix-c++11.patch; cd ../..
fi

if [ -d sdks/vstsdk2.4/pluginterfaces ]; then
  sed -i -e "s/#define JUCE_PLUGINHOST_VST 0/#define JUCE_PLUGINHOST_VST 1/" libs/juce/build-juce/AppConfig.h || true
else
  sed -i -e "s/#define JUCE_PLUGINHOST_VST 1/#define JUCE_PLUGINHOST_VST 0/" libs/juce/build-juce/AppConfig.h || true
fi

# ------------------------------------------------------------------------------------------------------------
