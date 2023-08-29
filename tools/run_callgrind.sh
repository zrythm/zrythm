#!/bin/bash
# SPDX-FileCopyrightText: © 2021 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense
#
# Use CALLGRIND_START_INSTRUMENTATION /
# CALLGRIND_STOP_INSTRUMENTATION to specify where
# to start instrumenting
#
# Set toggle-collect to the function to collect
# data for

set -x
valgrind --tool=callgrind \
  --collect-jumps=yes \
  --separate-threads=yes \
  --dump-instr=yes --compress-strings=no \
  "$@"
set +x
