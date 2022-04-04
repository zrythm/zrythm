#!@BASH@
# SPDX-FileCopyrightText: © 2020-2021 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense

set -ex

languages="$1"

# doc/user source dir
srcdir="$2"

# directory to render to
renderdir="$3"

# build dir containing the html dirs for each
# language
builddir="$4"

# zip file containing the bundled website
out_file="$5"

private_dir="$6"

out_filename=`basename "$out_file"`

rm -rf "$out_file"

for lang in $languages ; do
  mkdir -p $renderdir/$lang
  cp -R $builddir/$lang/html/* $renderdir/$lang/
  cp $builddir/$lang/epub/Zrythm.epub $renderdir/$lang/ || true
  pushd $builddir/$lang/html
  zip -r $renderdir/$lang/Zrythm-html.zip ./* ; \
  popd
done

# copy favicon
cp $srcdir/_static/favicon.ico $renderdir/

# create result zip
out_filename_noext="${out_filename%.*}"
mkdir -p "$private_dir"/$out_filename_noext
cp -r $renderdir/* "$private_dir"/$out_filename_noext
pushd "$private_dir"
zip -r $out_filename $out_filename_noext
popd
cp $private_dir/$out_filename $out_file
zip -sf $out_file
