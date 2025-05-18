#! /bin/bash
# SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense

set -e

app_bundle="$1"
app_bundle_zip="$1.zip"
codesign_identity="$2"
entitlements_file_path="$3"

codesign \
  --deep --force --verbose \
  --entitlements "$entitlements_file_path" \
  --sign "$codesign_identity" \
  "$app_bundle"
codesign \
  --verify --deep --verbose \
  "$app_bundle"