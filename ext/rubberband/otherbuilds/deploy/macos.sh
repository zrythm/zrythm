#!/bin/bash
set -eu
if [ ! -f ../rba/deploy/macos/notarize.sh ]; then
    echo "Need notarize script in ../rba/deploy/macos"
fi

version=$(grep '^ *version:' meson.build | head -1 | sed "s/^.*'\([0-9][0-9.]*\)'.*$/\1/")
echo
echo "Packaging command-line utility for Mac for Rubber Band v$version..."
echo
if [ -f /usr/local/lib/libsndfile.dylib ]; then
    echo "(WARNING: libsndfile dynamic library found in /usr/local/lib!"
    echo "Be sure that you aren't about to combine this external dependency"
    echo "with the hardened runtime)"
fi

echo -n "Proceed [Yn] ? "
read yn
case "$yn" in "") ;; [Yy]) ;; *) exit 3;; esac
echo "Proceeding"

rm -rf build_arm64 build_x86_64 tmp_pack
PKG_CONFIG_PATH=/usr/local/lib/pkgconfig/ meson build_arm64 --cross-file ./cross/macos-arm64.txt
PKG_CONFIG_PATH=/usr/local/lib/pkgconfig/ meson build_x86_64 --cross-file ./cross/macos-x86_64.txt
ninja -C build_arm64
ninja -C build_x86_64
mkdir tmp_pack
lipo build_arm64/rubberband build_x86_64/rubberband -create -output tmp_pack/rubberband
lipo build_arm64/rubberband-r3 build_x86_64/rubberband-r3 -create -output tmp_pack/rubberband-r3

echo
echo "Check the following version number: it should read $version"
tmp_pack/rubberband -V
echo
echo "So should this one:"
tmp_pack/rubberband-r3 -V
echo

key="Developer ID Application: Particular Programs Ltd (73F996B92S)"
mkdir -p packages
( cd tmp_pack
  codesign -s "$key" -fv --options runtime rubberband
  codesign -s "$key" -fv --options runtime rubberband-r3
  zipfile="rubberband-$version-gpl-executable-macos.zip"
  rm -f "$zipfile"
  zip "$zipfile" rubberband rubberband-r3
#  ditto -c -k rubberband rubberband-r3 "$zipfile" #!!! "can't archive multiple sources"
  ../../rba/deploy/macos/notarize.sh "$zipfile" com.breakfastquay.rubberband
)
package_dir="rubberband-$version-gpl-executable-macos"
rm -rf "$package_dir"
mkdir "$package_dir"
cp tmp_pack/rubberband "$package_dir"
cp tmp_pack/rubberband-r3 "$package_dir"
cp CHANGELOG README.md COPYING "$package_dir"
tar cvjf "$package_dir.tar.bz2" "$package_dir"
mv "$package_dir.tar.bz2" packages/
rm -rf "$package_dir"
echo
echo "Done, package is in packages/$package_dir.tar.bz2"

