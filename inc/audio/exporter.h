/*
 * audio/export.h - Audio file exporter
 *
 * Copyright (C) 2018 Alexandros Theodotou
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

typedef enum AudioFormat
{
  AUDIO_FORMAT_FLAC,
  AUDIO_FORMAT_OGG
} AudioFormat;

typedef enum BitDepth
{
  BIT_DEPTH_16,
  BIT_DEPTH_24,
  BIT_DEPTH_32
} BitDepth;

typedef struct ExportInfo
{
  AudioFormat       format;
  BitDepth          depth;
  char *            file_uri;
} ExportInfo;

void
exporter_export (ExportInfo * info);

#endif

