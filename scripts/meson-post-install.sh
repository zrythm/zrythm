#!/usr/bin/env sh
#
# SPDX-FileCopyrightText: © 2019-2021 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense

prefix="$MESON_INSTALL_DESTDIR_PREFIX"
datadir="$prefix/share"
schemadir="$datadir/glib-2.0/schemas"
fontsdir="$datadir/fonts/zrythm"
desktop_db_dir="$datadir/applications"
mime_dir="$datadir/mime"
doc_dir="$datadir/doc/zrythm"

if [ ! "$DESTDIR" ]; then
  echo "Compiling gsettings schemas..."
  glib-compile-schemas $schemadir

  if command -v gtk-update-icon-cache; then
    echo "Updating icon cache..."
    touch "$datadir/icons/hicolor"
    gtk-update-icon-cache
  fi

  if command -v update-mime-database; then
    echo "Updating MIME database..."
    update-mime-database "$mime_dir"
  fi

  if command -v update-desktop-database; then
    echo "Updating desktop database..."
    if [ ! -e  "$desktop_db_dir" ]; then
      mkdir -p "$desktop_db_dir"
    fi
    update-desktop-database -q "$desktop_db_dir"
  fi

  if command -v update-gdk-pixbuf-loaders; then
    update-gdk-pixbuf-loaders
  fi
fi
