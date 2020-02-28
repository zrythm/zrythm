/*
 * Copyright (C) 2019 Alexandros Theodotou <alex at zrythm dot org>
 *
 * This file is part of Zrythm
 *
 * Zrythm is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Zrythm is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with Zrythm.  If not, see <https://www.gnu.org/licenses/>.
 */

/**
 * \file
 *
 * Supported file info.
 */

#ifndef __AUDIO_SUPPORTED_FILE_H__
#define __AUDIO_SUPPORTED_FILE_H__

/**
 * File type.
 */
typedef enum FileType
{
  FILE_TYPE_MIDI,
  FILE_TYPE_MP3,
  FILE_TYPE_FLAC,
  FILE_TYPE_OGG,
  FILE_TYPE_WAV,
  FILE_TYPE_DIR,
  /** Special entry ".." for the parent dir. */
  FILE_TYPE_PARENT_DIR,
  FILE_TYPE_OTHER,
  NUM_FILE_TYPES,
} FileType;

/**
 * Metadata for a supported file.
 */
typedef struct SupportedFile
{
  /** Absolute path. */
  char *         abs_path;

  /** Type of file. */
  FileType       type;

  /** Human readable label. */
  char *          label;

  /** Hidden or not. */
  int             hidden;

  /** MIDI file, if midi. */
  //MidiFile *     midi_file;

  /** Audio file, if audio. */
  //AudioFile *    midi_file;
} SupportedFile;

/**
 * Creates a new SupportedFile from the given absolute
 * path.
 */
SupportedFile *
supported_file_new_from_path (
  const char * path);

/**
 * Returns a human readable description of the given
 * file type.
 *
 * Example: wav -> "Wave file".
 */
char *
supported_file_type_get_description (
  FileType type);


/**
 * Clones the given SupportedFile.
 */
SupportedFile *
supported_file_clone (
  SupportedFile * src);

/**
 * Returns if the given type is supported.
 */
int
supported_file_type_is_supported (
  FileType type);

/**
 * Returns if the SupportedFile is an audio file.
 */
int
supported_file_type_is_audio (
  FileType type);

/**
 * Returns if the SupportedFile is a midi file.
 */
int
supported_file_type_is_midi (
  FileType type);

/**
 * Returns the most common extension for the given
 * filetype.
 */
const char *
supported_file_type_get_ext (
  FileType type);

/**
 * Returns the file type of the given file path.
 */
FileType
supported_file_get_type (
  const char * file);

/**
 * Frees the instance and all its members.
 */
void
supported_file_free (
  SupportedFile * self);

#endif
