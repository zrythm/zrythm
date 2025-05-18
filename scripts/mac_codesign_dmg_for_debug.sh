#! /bin/bash
# SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense

set -e

dmg_bundle="$1"
codesign_identity="$2"

codesign \
  --force --verbose \
  --sign "$codesign_identity" \
  "$dmg_bundle"
codesign --verify --verbose "$dmg_bundle"