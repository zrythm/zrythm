#!/bin/bash
set -eu
version=$(meson introspect --projectinfo meson.build -i  | grep '"version"' | sed -e 's/^.*: "//' -e 's/".*$//')
check() {
    text="$1"
    echo -n "$text [yN] "
    read yn
    case "$yn" in [Yy]) ;; *) echo "Exiting"; exit 3 ;; esac
}
echo
echo "Preparing to make source package for Rubber Band v$version..."
echo
grep '^ *version:' meson.build | head -1
check "Is the above version number (in meson.build) correct?"
echo
echo "The dynamic library version should have a point increment for each"
echo "release, a minor increment for backward-compatible ABI changes, and"
echo "a major increment for incompatible ABI changes."
echo
grep 'rubberband_dynamic_library_version' meson.build | head -1
check "Is the above library version (from meson.build) correct?"
echo
echo "The API major version should increment for incompatible API changes,"
echo "and the minor version should increment for backward-compatible API"
echo "changes."
echo
grep 'RUBBERBAND.*VERSION' rubberband/RubberBandStretcher.h
check "Are the above version and API versions (from the C++ header) correct?"
echo
echo "The C header should contain the same versions as the C++ header."
echo
grep 'RUBBERBAND.*VERSION' rubberband/rubberband-c.h
check "Are the above version and API versions (from the C header) correct?"
echo
echo "The LV2 plugin RDF should contain 2x the API minor version for minorVersion"
echo "and an LV2-specific microVersion field, for the mono and stereo plugins."
echo
grep 'Version' ladspa-lv2/rubberband.lv2/lv2-rubberband.ttl
check "Are the above minor and micro versions (from the LV2 plugin RDF) correct?"
echo
grep '^PROJECT_NUMBER' Doxyfile
check "Is the above version (from Doxyfile) correct?"
echo
echo "The CHANGELOG should start with a list of changes for this release."
head -3 CHANGELOG
check "The first three lines of the CHANGELOG are above. Are they correct?"

hgid=$(hg id | awk '{ print $1; }')
case "$hgid" in
    *+) echo "ERROR: Working copy has been modified, please review and commit"
        echo "as appropriate before continuing"
        exit 1
        ;;
    *);;
esac

echo
check "All version checks passed. Continue?"

echo
echo "Going ahead..."
mkdir -p packages
output="packages/rubberband-$version.tar.bz2"
hg archive --exclude otherbuilds/deploy "$output"

echo "Checking that the package compiles..."
tmpdir=$(mktemp -d)
cleanup() {
    rm -rf "$tmpdir"
}
trap cleanup 0
prevdir=$(pwd)
( cd "$tmpdir"
  tar xvf "$prevdir/$output"
  cd "rubberband-$version"
  meson build
  ninja -C build
)

echo
echo "Checked, package is in $output"
