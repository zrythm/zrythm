#! /bin/sh
# SPDX-FileCopyrightText: © 2020 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense
#
# This is used during the tests to create a temporary
# lv2 plugin environment before calling lv2lint

set -e

lv2info_bin=$1
plugin_uri=$2

$lv2info_bin "$plugin_uri" | grep "Bundle" | head -n 1 | awk '{print $2}'
