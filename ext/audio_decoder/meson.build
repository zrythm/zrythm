# Copyright (C) 2019 Alexandros Theodotou <alex at zrythm dot org>
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

audio_decoder_srcs = [
  'ad_soundfile.c',
  'ad_ffmpeg.c',
  'ad_plugin.c',
  ]

audio_decoder_inc = [
  ext_inc,
  root_inc,
  ]
audio_decoder = static_library (
  'audio-decoder',
  sources: audio_decoder_srcs,
  dependencies: [
    config_h_dep,
    sndfile_dep,
    ],
  include_directories: audio_decoder_inc,
)
