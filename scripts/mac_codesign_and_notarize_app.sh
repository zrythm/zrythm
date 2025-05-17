#! /bin/bash
# SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense

set -e

app_bundle="$1"
app_bundle_zip="$1.zip"
codesign_identity="$2"
notarization_keychain_profile="$3"

codesign \
  --deep --force --verbose --options runtime --timestamp \
  --sign "$codesign_identity" \
  "$app_bundle"
codesign \
  --verify --deep --verbose \
  "$app_bundle"
ditto \
  -c -k --keepParent \
  "$app_bundle" \
  "$app_bundle_zip"

if [ -n "$notarization_keychain_profile" ]; then
  # Use keychain profile if provided
  xcrun notarytool submit \
    "$app_bundle_zip" \
    --keychain-profile "$notarization_keychain_profile" \
    --wait
else
  # Fall back to environment variables (used in CI)
  xcrun notarytool submit \
    "$app_bundle_zip" \
    --apple-id "$APPLE_NOTARIZATION_APPLE_ID" \
    --team-id "$APPLE_NOTARIZATION_TEAM_ID" \
    --password "$APPLE_NOTARIZATION_ZRYTHM_APP_SPECIFIC_PASSWORD" \
    --wait
fi

xcrun stapler staple "$app_bundle"