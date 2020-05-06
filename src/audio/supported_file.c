/*
 * Copyright (C) 2019-2020 Alexandros Theodotou <alex at zrythm dot org>
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

#include <stdlib.h>

#include "audio/supported_file.h"
#include "utils/io.h"
#include "utils/string.h"

#include <gtk/gtk.h>

/**
 * Creates a new SupportedFile from the given absolute
 * path.
 */
SupportedFile *
supported_file_new_from_path (
  const char * path)
{
  SupportedFile * self =
    calloc (1, sizeof (SupportedFile));

  self->abs_path = g_strdup (path);
  self->type =
    supported_file_get_type (path);
  self->label =
    g_path_get_basename (path);

  g_warn_if_fail (
    self->type >= FILE_TYPE_MIDI &&
    self->type < NUM_FILE_TYPES);

  return self;
}

/**
 * Returns if the given type is supported.
 */
int
supported_file_type_is_supported (
  ZFileType type)
{
  if (supported_file_type_is_audio (type))
    {
      if (type == FILE_TYPE_FLAC ||
          type == FILE_TYPE_OGG ||
          type == FILE_TYPE_WAV)
        return 1;

#ifdef HAVE_FFMPEG
      if (type == FILE_TYPE_MP3)
        return 1;
#endif
    }
  if (supported_file_type_is_midi (type))
    return 1;

  return 0;
}

/**
 * Returns if the SupportedFile is an audio file.
 */
int
supported_file_type_is_audio (
  ZFileType type)
{
  return
    type == FILE_TYPE_MP3 ||
    type == FILE_TYPE_FLAC ||
    type == FILE_TYPE_OGG ||
    type == FILE_TYPE_WAV;
}

/**
 * Returns if the SupportedFile is a midi file.
 */
int
supported_file_type_is_midi (
  ZFileType type)
{
  return type == FILE_TYPE_MIDI;
}

/**
 * Clones the given SupportedFile.
 */
SupportedFile *
supported_file_clone (
  SupportedFile * src)
{
  SupportedFile * dest =
    calloc (1, sizeof (SupportedFile));

  dest->type = src->type;
  dest->abs_path = g_strdup (src->abs_path);

  return dest;
}

/**
 * Returns a human readable description of the given
 * file type.
 *
 * Example: wav -> "Wave file".
 */
char *
supported_file_type_get_description (
  ZFileType type)
{
  switch (type)
    {
    case FILE_TYPE_MIDI:
      return g_strdup ("MIDI");
      break;
    case FILE_TYPE_MP3:
      return g_strdup ("MP3");
      break;
      break;
    case FILE_TYPE_FLAC:
      return g_strdup ("FLAC");
      break;
    case FILE_TYPE_OGG:
      return g_strdup ("OGG (Vorbis)");
      break;
    case FILE_TYPE_WAV:
      return g_strdup ("Wave");
      break;
    case FILE_TYPE_DIR:
      return g_strdup ("Directory");
      break;
    case FILE_TYPE_PARENT_DIR:
      return g_strdup ("Parent directory");
      break;
    case FILE_TYPE_OTHER:
      return g_strdup ("Other");
      break;
    default:
      g_return_val_if_reached (NULL);
    }
}

/**
 * Returns the file type of the given file path.
 */
ZFileType
supported_file_get_type (
  const char * file)
{
  const char * ext = io_file_get_ext (file);
  ZFileType type = FILE_TYPE_OTHER;

  if (g_file_test (file, G_FILE_TEST_IS_DIR))
    type = FILE_TYPE_DIR;
  else if (string_is_equal (ext, "", 1))
    type = FILE_TYPE_OTHER;
  else if (
      string_is_equal (ext, "MID", 1) ||
      string_is_equal (ext, "MIDI", 1) ||
      string_is_equal (ext, "SMF", 1))
    type = FILE_TYPE_MIDI;
  else if (
      string_is_equal (ext, "mp3", 1))
    type = FILE_TYPE_MP3;
  else if (
      string_is_equal (ext, "flac", 1))
    type = FILE_TYPE_FLAC;
  else if (
      string_is_equal (ext, "ogg", 1))
    type = FILE_TYPE_OGG;
  else if (
      string_is_equal (ext, "wav", 1))
    type = FILE_TYPE_WAV;
  else
    type = FILE_TYPE_OTHER;

  return type;
}

/**
 * Returns the most common extension for the given
 * filetype.
 */
const char *
supported_file_type_get_ext (
  ZFileType type)
{
  switch (type)
    {
    case FILE_TYPE_MIDI:
      return g_strdup ("mid");
      break;
    case FILE_TYPE_MP3:
      return g_strdup ("mp3");
      break;
      break;
    case FILE_TYPE_FLAC:
      return g_strdup ("FLAC");
      break;
    case FILE_TYPE_OGG:
      return g_strdup ("ogg");
      break;
    case FILE_TYPE_WAV:
      return g_strdup ("wav");
      break;
    case FILE_TYPE_PARENT_DIR:
      return NULL;
      break;
    case FILE_TYPE_DIR:
      return NULL;
      break;
    case FILE_TYPE_OTHER:
      return NULL;
      break;
    default:
      g_return_val_if_reached (NULL);
    }
}

/**
 * Frees the instance and all its members.
 */
void
supported_file_free (
  SupportedFile * self)
{
  if (self->abs_path)
    g_free (self->abs_path);
  if (self->label)
    g_free (self->label);

  free (self);
}
