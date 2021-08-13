#!/bin/bash

# Use CALLGRIND_START_INSTRUMENTATION /
# CALLGRIND_STOP_INSTRUMENTATION to specify where
# to start instrumenting
#
# Set toggle-collect to the function to collect
# data for

set -x
valgrind --tool=callgrind \
  --collect-jumps=yes \
  --instr-atstart=no --collect-atstart=no \
  --toggle-collect=arranger_object_draw \
  --separate-threads=yes \
  --dump-instr=yes --compress-strings=no \
  --zero-before=main_window_widget_setup \
  --dump-before=main_window_widget_setup \
  --dump-after=main_window_widget_tear_down \
  "$@"
set +x
