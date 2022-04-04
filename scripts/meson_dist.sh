#!/usr/bin/env sh
# SPDX-FileCopyrightText: © 2020 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense

# remove CI scripts
rm -rf "${MESON_DIST_ROOT}"/.builds
rm -rf "${MESON_DIST_ROOT}"/.builds-extra
rm -rf "${MESON_DIST_ROOT}"/.travis.yml
rm -rf "${MESON_DIST_ROOT}"/.github
rm -rf "${MESON_DIST_ROOT}"/git-packaging-hooks
