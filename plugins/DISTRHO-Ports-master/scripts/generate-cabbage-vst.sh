#!/bin/bash

set -e

if [ -d bin ]; then
  cd bin
else
  echo "Please run this script from the distrho root folder"
  exit
fi

if [ ! -f cabbage/CabbagePluginSynth.so  ]; then exit 0; fi
if [ ! -f cabbage/CabbagePluginMidi.so   ]; then exit 0; fi
if [ ! -f cabbage/CabbagePluginEffect.so ]; then exit 0; fi

mkdir -p vst

if [ -f ../libs/lv2_ttl_generator.exe ]; then
  EXT=dll
else
  EXT=so
fi

FILES=`find ./cabbage -name \*.csd`

for i in $FILES; do
  basename=`echo $i | awk 'sub("./cabbage/","")' | awk 'sub("/","\n")' | tail -n 1 | awk 'sub(".csd","")'`
  basename=`echo "cabbage-$basename"`

  if ( echo $i | grep "./cabbage/Synths/" > /dev/null ); then
    cp -v `pwd`/cabbage/CabbagePluginSynth.so vst/$basename.$EXT
  elif ( echo $i | grep "./cabbage/MIDI/" > /dev/null ); then
    cp -v `pwd`/cabbage/CabbagePluginMidi.so vst/$basename.$EXT
  else
    cp -v `pwd`/cabbage/CabbagePluginEffect.so vst/$basename.$EXT
  fi

  cp -v $i vst/$basename.csd

done

# Special files
cp -v cabbage/Synths/bassline.snaps vst/

cd ..
