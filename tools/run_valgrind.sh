#!/bin/bash

# This should be call from the zrythm root dir

# add --undef-value-errors=no to stop condotion depends on
# undefined value
NO_SCAN_PLUGINS=1 \
  ZRYTHM_DSP_THREADS=1 \
    valgrind --num-callers=30 --log-file=valog \
    --suppressions=tools/vg.sup \
    build/src/zrythm
