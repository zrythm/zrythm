#!@BASH@

set -ex

languages="$1"

# directory to render to
renderdir="$2"

# build dir containing the html dirs for each language
builddir="$3"

# zip file containing the bundled website
out_file="$4"

private_dir="$5"

out_filename=`basename "$out_file"`

rm -rf "$out_file"

for lang in $languages ; do
  mkdir -p $renderdir/$lang
  cp -R $builddir/$lang/html/* $renderdir/$lang/
  cp $builddir/$lang/epub/Zrythm.epub $renderdir/$lang/ || true
  zip -r $renderdir/$lang/Zrythm-html.zip $builddir/$lang/html/* ; \
done

# create result zip
out_filename_noext="${out_filename%.*}"
mkdir -p "$private_dir"/$out_filename_noext
cp -r $renderdir/* "$private_dir"/$out_filename_noext
pushd "$private_dir"
zip -r $out_filename $out_filename_noext
popd
cp $private_dir/$out_filename $out_file
zip -sf $out_file
