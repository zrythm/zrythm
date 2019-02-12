/*
 * Copyright (C) 2018-2019 Alexandros Theodotou
 *
 * This file is part of Zrythm
 *
 * Zrythm is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Zrythm is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Zrythm.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef __AUDIO_EXPORT_H__
#define __AUDIO_EXPORT_H__

/**
 * Audio format.
 */
typedef enum AudioFormat
{
  AUDIO_FORMAT_FLAC,
  AUDIO_FORMAT_OGG,
  AUDIO_FORMAT_WAV,
  AUDIO_FORMAT_MP3,
} AudioFormat;

/**
 * Bit depth.
 */
typedef enum BitDepth
{
  BIT_DEPTH_16,
  BIT_DEPTH_24,
  BIT_DEPTH_32
} BitDepth;

/**
 * Export settings to be passed to the exporter
 * to use.
 */
typedef struct ExportSettings
{
  AudioFormat       format;
  char *            artist;
  char *            genre;
  BitDepth          depth;
  char *            file_uri;
} ExportSettings;

/**
 * Exports an audio file based on the given
 * settings.
 *
 * TODO move some things into functions.
 */
void
exporter_export (ExportSettings * info);

#endif
