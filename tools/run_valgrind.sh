#!/bin/bash
# SPDX-FileCopyrightText: © 2019, 2021 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense
#
# This should be called from the zrythm root dir

# add --undef-value-errors=no to stop condotion depends on
# undefined value
ZRYTHM_SKIP_PLUGIN_SCAN=1 \
  ZRYTHM_DSP_THREADS=1 \
    valgrind --num-callers=30 --log-file=valog \
    --gen-suppressions=all \
    --suppressions=tools/vg.sup \
    build/src/zrythm
