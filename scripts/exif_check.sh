#!/bin/bash
# SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: CC0-1.0

if exiftool -exif -if '$exif' doc/user/_static/img/*.png ; then
  echo 'Error: Exif found. Please remove exif data.'
  exit 1
else
  echo 'No Exif found'
fi
