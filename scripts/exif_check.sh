#!/bin/bash

if exiftool -exif -if '$exif' doc/user/_static/img/*.png ; then
  echo 'Error: Exif found. Please remove exif data.'
  exit 1
else
  echo 'No Exif found'
fi
