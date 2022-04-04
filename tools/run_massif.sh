#! /bin/sh
# SPDX-FileCopyrightText: © 2021 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense
#
# This should be called from the zrythm root dir

set -e

export ZRYTHM_DSP_THREADS=1
valgrind --tool=massif --num-callers=160 \
    --verbose --log-file=massif_log \
    --suppressions=tools/vg.sup build/src/zrythm \
    --dummy --buf-size=8194

