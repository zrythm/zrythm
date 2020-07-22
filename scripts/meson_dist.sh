#!/usr/bin/env sh

# remove CI scripts
rm -rf "${MESON_DIST_ROOT}"/.builds
rm -rf "${MESON_DIST_ROOT}"/.travis.yml
rm -rf "${MESON_DIST_ROOT}"/.github
rm -rf "${MESON_DIST_ROOT}"/git-packaging-hooks
