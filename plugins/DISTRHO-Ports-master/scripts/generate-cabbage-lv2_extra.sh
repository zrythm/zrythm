#!/bin/bash

set -e

if [ -d bin ]; then
  cd bin
else
  echo "Please run this script from the distrho root folder"
  exit
fi

if [ ! -f cabbage/CabbagePluginSynthLv2.lv2/CabbagePluginSynthLv2.so   ]; then exit 0; fi
if [ ! -f cabbage/CabbagePluginMidiLv2.lv2/CabbagePluginMidiLv2.so     ]; then exit 0; fi
if [ ! -f cabbage/CabbagePluginEffectLv2.lv2/CabbagePluginEffectLv2.so ]; then exit 0; fi

mkdir -p lv2

if [ -f ../libs/lv2_ttl_generator.exe ]; then
  GEN=../../../libs/lv2_ttl_generator.exe
  EXT=dll
else
  GEN=../../../libs/lv2_ttl_generator
  EXT=so
fi

FILES=`find ./cabbage-extra -name \*.csd`

for i in $FILES; do
  basename=`echo $i | awk 'sub("./cabbage-extra/","")' | awk 'sub("/","\n")' | tail -n 1 | awk 'sub(".csd","")'`
  basename=`echo "cabbage-$basename"`
  lv2dir=`echo "./lv2-extra/"$basename".lv2/"`

  mkdir -p -v $lv2dir

  if ( echo $i | grep "./cabbage-extra/Synths/" > /dev/null ); then
    cp -v `pwd`/cabbage/CabbagePluginSynthLv2.lv2/CabbagePluginSynthLv2.so $lv2dir/$basename.$EXT
  elif ( echo $i | grep "./cabbage-extra/MIDI/" > /dev/null ); then
    cp -v `pwd`/cabbage/CabbagePluginMidiLv2.lv2/CabbagePluginMidiLv2.so $lv2dir/$basename.$EXT
  else
    cp -v `pwd`/cabbage/CabbagePluginEffectLv2.lv2/CabbagePluginEffectLv2.so $lv2dir/$basename.$EXT
  fi

  cp -v $i $lv2dir/$basename.csd

done

# Special files
# cp -v cabbage/Synths/bassline.snaps lv2/cabbage-bassline.lv2/

cd ..
