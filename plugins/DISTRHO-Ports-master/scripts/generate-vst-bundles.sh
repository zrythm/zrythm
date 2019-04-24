#!/bin/bash

set -e

if [ -d bin ]; then
  cd bin
else
  echo "Please run this script from the distrho root folder"
  exit
fi

PWD=`pwd`

if [ ! -d /Developer ]; then
  echo "This doesn't seem to be OSX, please stop!"
  exit 0
fi

cd vst

PLUGINS=`ls | grep dylib`

for i in $PLUGINS; do
  FILE=`echo $i | awk 'sub(".dylib","")'`
  cp -r ../../scripts/plugin.vst/ $FILE.vst
  cp $i $FILE.vst/Contents/MacOS/$FILE
  rm -f $FILE.vst/Contents/MacOS/deleteme
  sed -i -e "s/X-PROJECTNAME-X/$FILE/" $FILE.vst/Contents/Info.plist
  SetFile -a B $FILE.vst
done

cd ../../
