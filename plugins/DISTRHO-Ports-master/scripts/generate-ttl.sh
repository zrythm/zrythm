#!/bin/bash

set -e

if [ -d bin ]; then
  cd bin
else
  echo "Please run this script from the distrho root folder"
  exit
fi

PWD=`pwd`

if [ -f $PWD/../libs/lv2_ttl_generator.exe ]; then
  GEN=$PWD/../libs/lv2_ttl_generator.exe
  EXT=dll
elif [ -d /System/Library ]; then
  GEN=$PWD/../libs/lv2_ttl_generator
  EXT=dylib
else
  GEN=$PWD/../libs/lv2_ttl_generator
  EXT=so
fi

FOLDERS=`find ./lv2/ -name \*.lv2`

for i in $FOLDERS; do
  cd $i
  FILE=`ls *.$EXT 2>/dev/null | sort | head -n 1`
  if [ "$FILE"x != ""x ]; then
    $GEN ./$FILE
  fi
  cd ../..
done

# Remove cabbage logs
rm -f $PWD/lv2/cabbage*.lv2/CabbageLog.txt
