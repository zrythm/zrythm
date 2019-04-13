#! /bin/bash

BUILD_DIR=build
WINDIR=$BUILD_DIR/win
mkdir $WINDIR

# ******************************
echo "Copying dlls..."
DLLS=" \
  libatk-1.0-0.dll \
  libbz2-1.dll \
  libcairo-2.dll \
  libcairo-gobject-2.dll \
  libdatrie-1.dll \
  libdl.dll \
  libepoxy-0.dll \
  libexpat-1.dll \
  libffi-6.dll \
  libFLAC-8.dll \
  libfontconfig-1.dll \
  libfreetype-6.dll \
  libfribidi-0.dll \
  libgcc_s_seh-1.dll \
  libgdk_pixbuf-2.0-0.dll \
  libgdk-3-0.dll \
  libgio-2.0-0.dll \
  libglib-2.0-0.dll \
  libgmodule-2.0-0.dll \
  libgobject-2.0-0.dll \
  libgraphite2.dll \
  libgtk-3-0.dll \
  libharfbuzz-0.dll \
  libiconv-2.dll \
  libintl-8.dll \
  libjasper-4.dll \
  libjpeg-8.dll \
  liblzma-5.dll \
  libogg-0.dll \
  libpango-1.0-0.dll \
  libpangocairo-1.0-0.dll \
  libpangoft2-1.0-0.dll \
  libpangowin32-1.0-0.dll \
  libpcre-1.dll \
  libpng16-16.dll \
  libportaudio-2.dll \
  libsamplerate-0.dll \
  libsndfile-1.dll \
  libstdc++-6.dll \
  libthai-0.dll \
  libtiff-5.dll \
  libvorbis-0.dll \
  libvorbisenc-2.dll \
  libvorbisfile-3.dll \
  libwinpthread-1.dll \
  lilv-0.dll \
  serd-0.dll \
  sord-0.dll \
  sratom-0.dll \
  zlib1.dll"

for file in $DLLS; do
  echo "copying $file"
  cp $1/bin/$file $WINDIR
done

# pixman doesn't work unless the mSYS one is used
echo "copying libpixman-1-0.dll"
cp data/dlls/libpixman-1-0.dll $WINDIR
# ******************************

# ******************************
echo "packaging settings.ini"
ETC_GTK_DIR="$WINDIR/etc/gtk-3.0"
mkdir -p $ETC_GTK_DIR
cp data/settings.ini $ETC_GTK_DIR/
# ******************************

# ******************************
echo "packaging glib schema"
SCHEMAS_DIR="$WINDIR/share/glib-2.0/schemas"
mkdir -p "$SCHEMAS_DIR"
cp "$BUILD_DIR/schemas/gschemas.compiled" \
  "$SCHEMAS_DIR/"
# ******************************

# ******************************
echo "packaging Adwaita icons"
ICONS_DIR="$WINDIR/share/icons"
mkdir -p "$ICONS_DIR"
cp -R "$1/share/icons/Adwaita" "$ICONS_DIR/"
# ******************************
