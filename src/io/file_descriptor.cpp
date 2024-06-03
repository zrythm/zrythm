// clang-format off
// SPDX-FileCopyrightText: © 2019-2021, 2023-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense
// clang-format on

#include <cstdlib>

#include <inttypes.h>

#include "io/audio_file.h"
#include "io/file_descriptor.h"
#include "utils/error.h"
#include "utils/io.h"
#include "utils/string.h"

#include <glib/gi18n.h>

#include "gtk_wrapper.h"

FileDescriptor::FileDescriptor (const char * _abs_path)
{
  // g_debug ("creating new FileDescriptor for %s", path);
  this->abs_path = _abs_path;
  this->type = get_type_from_path (_abs_path);
  char * tmp = g_path_get_basename (_abs_path);
  this->label = tmp;
  g_free (tmp);
}

std::unique_ptr<FileDescriptor>
FileDescriptor::new_from_uri (const char * uri, GError ** error)
{
  GError * err = NULL;
  char *   path = g_filename_from_uri (uri, NULL, &err);
  if (!path)
    {
      PROPAGATE_PREFIXED_ERROR (
        error, err, "Error getting file path from URI <%s>", uri);
      return nullptr;
    }

  auto self = std::make_unique<FileDescriptor> (path);
  g_free (path);

  return self;
}

bool
FileDescriptor::is_type_supported (ZFileType type)
{
  if (is_type_audio (type))
    {
      if (
        type == ZFileType::FILE_TYPE_FLAC || type == ZFileType::FILE_TYPE_OGG
        || type == ZFileType::FILE_TYPE_WAV || type == ZFileType::FILE_TYPE_MP3)
        return true;
    }
  if (is_type_midi (type))
    return true;

  return false;
}

bool
FileDescriptor::is_type_audio (ZFileType type)
{
  return type == ZFileType::FILE_TYPE_MP3 || type == ZFileType::FILE_TYPE_FLAC
         || type == ZFileType::FILE_TYPE_OGG || type == ZFileType::FILE_TYPE_WAV;
}

bool
FileDescriptor::is_type_midi (ZFileType type)
{
  return type == ZFileType::FILE_TYPE_MIDI;
}

char *
FileDescriptor::get_type_description (ZFileType type)
{
  switch (type)
    {
    case ZFileType::FILE_TYPE_MIDI:
      return g_strdup ("MIDI");
      break;
    case ZFileType::FILE_TYPE_MP3:
      return g_strdup ("MP3");
      break;
    case ZFileType::FILE_TYPE_FLAC:
      return g_strdup ("FLAC");
      break;
    case ZFileType::FILE_TYPE_OGG:
      return g_strdup ("OGG (Vorbis)");
      break;
    case ZFileType::FILE_TYPE_WAV:
      return g_strdup ("Wave");
      break;
    case ZFileType::FILE_TYPE_DIR:
      return g_strdup ("Directory");
      break;
    case ZFileType::FILE_TYPE_PARENT_DIR:
      return g_strdup ("Parent directory");
      break;
    case ZFileType::FILE_TYPE_OTHER:
      return g_strdup ("Other");
      break;
    default:
      g_return_val_if_reached (NULL);
    }
}

ZFileType
FileDescriptor::get_type_from_path (const char * file)
{
  const char * ext = io_file_get_ext (file);
  ZFileType    type = ZFileType::FILE_TYPE_OTHER;

  if (g_file_test (file, G_FILE_TEST_IS_DIR))
    type = ZFileType::FILE_TYPE_DIR;
  else if (string_is_equal (ext, ""))
    type = ZFileType::FILE_TYPE_OTHER;
  else if (
    string_is_equal_ignore_case (ext, "MID")
    || string_is_equal_ignore_case (ext, "MIDI")
    || string_is_equal_ignore_case (ext, "SMF"))
    type = ZFileType::FILE_TYPE_MIDI;
  else if (string_is_equal_ignore_case (ext, "mp3"))
    type = ZFileType::FILE_TYPE_MP3;
  else if (string_is_equal_ignore_case (ext, "flac"))
    type = ZFileType::FILE_TYPE_FLAC;
  else if (string_is_equal_ignore_case (ext, "ogg"))
    type = ZFileType::FILE_TYPE_OGG;
  else if (string_is_equal_ignore_case (ext, "wav"))
    type = ZFileType::FILE_TYPE_WAV;
  else
    type = ZFileType::FILE_TYPE_OTHER;

  return type;
}

const char *
FileDescriptor::get_type_ext (ZFileType type)
{
  switch (type)
    {
    case ZFileType::FILE_TYPE_MIDI:
      return "mid";
      break;
    case ZFileType::FILE_TYPE_MP3:
      return "mp3";
      break;
    case ZFileType::FILE_TYPE_FLAC:
      return "flac";
      break;
    case ZFileType::FILE_TYPE_OGG:
      return "ogg";
      break;
    case ZFileType::FILE_TYPE_WAV:
      return "wav";
      break;
    case ZFileType::FILE_TYPE_PARENT_DIR:
      return NULL;
      break;
    case ZFileType::FILE_TYPE_DIR:
      return NULL;
      break;
    case ZFileType::FILE_TYPE_OTHER:
      return NULL;
      break;
    default:
      g_return_val_if_reached (NULL);
    }
}

bool
FileDescriptor::should_autoplay () const
{
  bool autoplay = true;

  /* no autoplay if not audio/MIDI */
  if (!is_audio () && !is_midi ())
    return false;

  if (is_audio ())
    {
      AudioFile * af = audio_file_new (this->abs_path.c_str ());
      GError *    err = NULL;
      bool        success = audio_file_read_metadata (af, &err);
      if (!success)
        {
          HANDLE_ERROR (err, "Error reading metadata from %s", af->filepath);
          return false;
        }

      if ((af->metadata.length / 1000) > 60)
        {
          autoplay = false;
        }

      audio_file_free (af);
    }

  return autoplay;
}

char *
FileDescriptor::get_info_text_for_label () const
{
  char * file_type_label = get_type_description (this->type);
  char * label_str = NULL;
  if (is_audio ())
    {
      AudioFile * af = audio_file_new (this->abs_path.c_str ());
      GError *    err = NULL;
      bool        success = audio_file_read_metadata (af, &err);
      if (!success)
        {
          HANDLE_ERROR (err, "Error reading metadata from %s", af->filepath);
          g_free (file_type_label);
          return g_strdup_printf (
            _ ("Failed reading metadata for %s"), af->filepath);
        }

      label_str = g_markup_printf_escaped (
        _ ("<b>%s</b>\n"
           "Sample rate: %d\n"
           "Length: %" PRId64 "s "
           "%" PRId64 " ms | BPM: %.1f\n"
           "Channel(s): %u | Bitrate: %'d.%d kb/s\n"
           "Bit depth: %d bits"),
        this->label.c_str (), af->metadata.samplerate,
        af->metadata.length / 1000, af->metadata.length % 1000,
        (double) af->metadata.bpm, af->metadata.channels,
        af->metadata.bit_rate / 1000, (af->metadata.bit_rate % 1000) / 100,
        af->metadata.bit_depth);

      audio_file_free (af);
    }
  else
    label_str = g_markup_printf_escaped (
      "<b>%s</b>\n"
      "Type: %s",
      this->label.c_str (), file_type_label);

  g_free (file_type_label);

  return label_str;
}

const char *
FileDescriptor::get_icon_name () const
{
  switch (this->type)
    {
    case ZFileType::FILE_TYPE_MIDI:
      return "audio-midi";
    case ZFileType::FILE_TYPE_MP3:
    case ZFileType::FILE_TYPE_FLAC:
    case ZFileType::FILE_TYPE_OGG:
    case ZFileType::FILE_TYPE_WAV:
      return "gnome-icon-library-sound-wave-symbolic";
    case ZFileType::FILE_TYPE_DIR:
      return "folder";
    case ZFileType::FILE_TYPE_PARENT_DIR:
      return "folder";
    case ZFileType::FILE_TYPE_OTHER:
      return "application-x-zerosize";
    default:
      return "";
    }
}