#!/usr/bin/env sh
#
# Copyright (C) 2019-2021 Alexandros Theodotou <alex at zrythm dot org>
#
# This file is part of Zrythm
#
# Zrythm is free software: you can redistribute it and/or modify
# it under the terms of the GNU Affero General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# Zrythm is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Affero General Public License for more details.
#
# You should have received a copy of the GNU Affero General Public License
# along with Zrythm.  If not, see <https://www.gnu.org/licenses/>.

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
