// SPDX-FileCopyrightText: © 2019-2021 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <inttypes.h>
#include <stdlib.h>

#include "audio/encoder.h"
#include "audio/supported_file.h"
#include "utils/io.h"
#include "utils/objects.h"
#include "utils/string.h"

#include <glib/gi18n.h>
#include <gtk/gtk.h>

/**
 * Creates a new SupportedFile from the given absolute
 * path.
 */
SupportedFile *
supported_file_new_from_path (const char * path)
{
  SupportedFile * self = object_new (SupportedFile);

  g_debug (
    "creating new SupportedFile for %s", path);
  self->abs_path = g_strdup (path);
  self->type = supported_file_get_type (path);
  self->label = g_path_get_basename (path);

  g_warn_if_fail (
    self->type >= FILE_TYPE_MIDI
    && self->type < NUM_FILE_TYPES);

  return self;
}

SupportedFile *
supported_file_new_from_uri (const char * uri)
{
  GError * err = NULL;
  char *   path =
    g_filename_from_uri (uri, NULL, &err);
  if (!path)
    {
      g_critical (
        "Error getting file path from URI <%s>: %s",
        uri, err->message);
      return NULL;
    }

  SupportedFile * self =
    supported_file_new_from_path (path);
  g_free (path);

  return self;
}

/**
 * Returns if the given type is supported.
 */
int
supported_file_type_is_supported (ZFileType type)
{
  if (supported_file_type_is_audio (type))
    {
      if (
        type == FILE_TYPE_FLAC
        || type == FILE_TYPE_OGG
        || type == FILE_TYPE_WAV
        || type == FILE_TYPE_MP3)
        return 1;
    }
  if (supported_file_type_is_midi (type))
    return 1;

  return 0;
}

/**
 * Returns if the SupportedFile is an audio file.
 */
int
supported_file_type_is_audio (ZFileType type)
{
  return type == FILE_TYPE_MP3 || type == FILE_TYPE_FLAC
         || type == FILE_TYPE_OGG
         || type == FILE_TYPE_WAV;
}

/**
 * Returns if the SupportedFile is a midi file.
 */
int
supported_file_type_is_midi (ZFileType type)
{
  return type == FILE_TYPE_MIDI;
}

/**
 * Clones the given SupportedFile.
 */
SupportedFile *
supported_file_clone (SupportedFile * src)
{
  SupportedFile * dest = object_new (SupportedFile);

  dest->type = src->type;
  dest->abs_path = g_strdup (src->abs_path);
  dest->hidden = src->hidden;
  dest->label = g_strdup (src->label);

  return dest;
}

/**
 * Returns a human readable description of the given
 * file type.
 *
 * Example: wav -> "Wave file".
 */
char *
supported_file_type_get_description (ZFileType type)
{
  switch (type)
    {
    case FILE_TYPE_MIDI:
      return g_strdup ("MIDI");
      break;
    case FILE_TYPE_MP3:
      return g_strdup ("MP3");
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
supported_file_get_type (const char * file)
{
  const char * ext = io_file_get_ext (file);
  ZFileType    type = FILE_TYPE_OTHER;

  if (g_file_test (file, G_FILE_TEST_IS_DIR))
    type = FILE_TYPE_DIR;
  else if (string_is_equal (ext, ""))
    type = FILE_TYPE_OTHER;
  else if (
    string_is_equal_ignore_case (ext, "MID")
    || string_is_equal_ignore_case (ext, "MIDI")
    || string_is_equal_ignore_case (ext, "SMF"))
    type = FILE_TYPE_MIDI;
  else if (string_is_equal_ignore_case (ext, "mp3"))
    type = FILE_TYPE_MP3;
  else if (string_is_equal_ignore_case (ext, "flac"))
    type = FILE_TYPE_FLAC;
  else if (string_is_equal_ignore_case (ext, "ogg"))
    type = FILE_TYPE_OGG;
  else if (string_is_equal_ignore_case (ext, "wav"))
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
supported_file_type_get_ext (ZFileType type)
{
  switch (type)
    {
    case FILE_TYPE_MIDI:
      return g_strdup ("mid");
      break;
    case FILE_TYPE_MP3:
      return g_strdup ("mp3");
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
 * Returns whether the given file should auto-play
 * (shorter than 1 min).
 */
bool
supported_file_should_autoplay (
  const SupportedFile * self)
{
  bool autoplay = true;

  /* no autoplay if not audio/MIDI */
  if (
    !supported_file_type_is_audio (self->type)
    && !supported_file_type_is_midi (self->type))
    return false;

  if (supported_file_type_is_audio (self->type))
    {
      AudioEncoder * enc =
        audio_encoder_new_from_file (self->abs_path);
      if (!enc)
        return false;

      if ((enc->nfo.length / 1000) > 60)
        {
          autoplay = false;
        }
      audio_encoder_free (enc);
    }

  return autoplay;
}

/**
 * Returns a pango markup to be used in GTK labels.
 */
char *
supported_file_get_info_text_for_label (
  const SupportedFile * self)
{
  char * file_type_label =
    supported_file_type_get_description (self->type);
  char * label = NULL;
  if (supported_file_type_is_audio (self->type))
    {
      AudioEncoder * enc =
        audio_encoder_new_from_file (self->abs_path);
      if (!enc)
        {
          g_free (file_type_label);
          return g_strdup (
            _ ("Failed opening file"));
        }

      label = g_markup_printf_escaped (
        _ (
          "<b>%s</b>\n"
          "Sample rate: %d\n"
          "Length: %" PRId64 "s "
          "%" PRId64 " ms | BPM: %.1f\n"
          "Channel(s): %u | Bitrate: %'d.%d kb/s\n"
          "Bit depth: %d bits"),
        self->label, enc->nfo.sample_rate,
        enc->nfo.length / 1000,
        enc->nfo.length % 1000,
        (double) enc->nfo.bpm, enc->nfo.channels,
        enc->nfo.bit_rate / 1000,
        (enc->nfo.bit_rate % 1000) / 100,
        enc->nfo.bit_depth);
      audio_encoder_free (enc);
    }
  else
    label = g_markup_printf_escaped (
      "<b>%s</b>\n"
      "Type: %s",
      self->label, file_type_label);

  g_free (file_type_label);

  return label;
}

/**
 * Gets the corresponding icon name for the given
 * SupportedFile's type.
 */
const char *
supported_file_get_icon_name (
  const SupportedFile * const self)
{
  switch (self->type)
    {
    case FILE_TYPE_MIDI:
      return "audio-midi";
    case FILE_TYPE_MP3:
      return "audio-x-mpeg";
    case FILE_TYPE_FLAC:
      return "audio-x-flac";
    case FILE_TYPE_OGG:
      return "application-ogg";
    case FILE_TYPE_WAV:
      return "audio-x-wav";
    case FILE_TYPE_DIR:
      return "folder";
    case FILE_TYPE_PARENT_DIR:
      return "folder";
    case FILE_TYPE_OTHER:
      return "application-x-zerosize";
    default:
      return "";
    }
}

/**
 * Frees the instance and all its members.
 */
void
supported_file_free (SupportedFile * self)
{
  if (self->abs_path)
    g_free (self->abs_path);
  if (self->label)
    g_free (self->label);

  free (self);
}
