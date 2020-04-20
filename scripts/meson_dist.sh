#!/usr/bin/env sh

# remove CI scripts
rm -rf "${MESON_DIST_ROOT}"/.builds
rm "${MESON_DIST_ROOT}"/.travis.yml
rm "${MESON_DIST_ROOT}"/.github
