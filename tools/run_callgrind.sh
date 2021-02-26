#!/bin/bash

NO_SCAN_PLUGINS=1 \
  valgrind --tool=callgrind \
  --dump-instr=yes --collect-jumps=yes \
  build/src/zrythm
