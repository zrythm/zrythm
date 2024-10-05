// SPDX-FileCopyrightText: © 2019-2021, 2023-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "gui/cpp/gtk_widgets/gtk_wrapper.h"

#include <glib/gi18n.h>

#include "common/io/audio_file.h"
#include "common/io/file_descriptor.h"
#include "common/utils/exceptions.h"
#include "common/utils/format.h"
#include "common/utils/io.h"
#include "common/utils/logger.h"
#include "common/utils/string.h"

FileDescriptor::FileDescriptor (const std::string &_abs_path)
{
  // z_debug ("creating new FileDescriptor for {}", path);
  abs_path_ = _abs_path;
  type_ = get_type_from_path (_abs_path.c_str ());
  label_ = Glib::path_get_basename (_abs_path);
}

std::unique_ptr<FileDescriptor>
FileDescriptor::new_from_uri (const std::string &uri)
{
  GError * err = NULL;
  char *   path = g_filename_from_uri (uri.c_str (), nullptr, &err);
  if (!path)
    {
      throw ZrythmException (fmt::format (
        "Error getting file path from URI <{}>: {}", uri, err->message));
    }

  auto self = std::make_unique<FileDescriptor> (path);
  g_free (path);

  return self;
}

bool
FileDescriptor::is_type_supported (FileType type)
{
  if (is_type_audio (type))
    {
      if (
        type == FileType::Flac || type == FileType::Ogg || type == FileType::Wav
        || type == FileType::Mp3)
        return true;
    }
  if (is_type_midi (type))
    return true;

  return false;
}

bool
FileDescriptor::is_type_audio (FileType type)
{
  return type == FileType::Mp3 || type == FileType::Flac
         || type == FileType::Ogg || type == FileType::Wav;
}

bool
FileDescriptor::is_type_midi (FileType type)
{
  return type == FileType::Midi;
}

std::string
FileDescriptor::get_type_description (FileType type)
{
  switch (type)
    {
    case FileType::Midi:
      return "MIDI";
    case FileType::Mp3:
      return "MP3";
    case FileType::Flac:
      return "FLAC";
    case FileType::Ogg:
      return "OGG (Vorbis)";
    case FileType::Wav:
      return "Wave";
    case FileType::Directory:
      return "Directory";
    case FileType::ParentDirectory:
      return "Parent directory";
    case FileType::Other:
      return "Other";
    default:
      z_return_val_if_reached ("");
    }
}

FileType
FileDescriptor::get_type_from_path (const fs::path &file)
{
  const auto ext = io_file_get_ext (file.string ());
  FileType   type = FileType::Other;

  if (fs::is_directory (file))
    type = FileType::Directory;
  else if (ext.empty ())
    type = FileType::Other;
  else if (
    string_is_equal_ignore_case (ext, "MID")
    || string_is_equal_ignore_case (ext, "MIDI")
    || string_is_equal_ignore_case (ext, "SMF"))
    type = FileType::Midi;
  else if (string_is_equal_ignore_case (ext, "mp3"))
    type = FileType::Mp3;
  else if (string_is_equal_ignore_case (ext, "flac"))
    type = FileType::Flac;
  else if (string_is_equal_ignore_case (ext, "ogg"))
    type = FileType::Ogg;
  else if (string_is_equal_ignore_case (ext, "wav"))
    type = FileType::Wav;
  else
    type = FileType::Other;

  return type;
}

const char *
FileDescriptor::get_type_ext (FileType type)
{
  switch (type)
    {
    case FileType::Midi:
      return "mid";
      break;
    case FileType::Mp3:
      return "mp3";
      break;
    case FileType::Flac:
      return "flac";
      break;
    case FileType::Ogg:
      return "ogg";
      break;
    case FileType::Wav:
      return "wav";
      break;
    case FileType::ParentDirectory:
      return NULL;
      break;
    case FileType::Directory:
      return NULL;
      break;
    case FileType::Other:
      return NULL;
      break;
    default:
      z_return_val_if_reached (nullptr);
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
      try
        {
          AudioFile af (abs_path_);
          auto      metadata = af.read_metadata ();
          if ((metadata.length / 1000) > 60)
            {
              autoplay = false;
            }
        }
      catch (const ZrythmException &e)
        {
          e.handle ("Error reading metadata from {}", abs_path_);
          return false;
        }
    }

  return autoplay;
}

std::string
FileDescriptor::get_info_text_for_label () const
{
  auto file_type_label = get_type_description (type_);
  if (is_audio ())
    {
      try
        {
          AudioFile af (abs_path_);
          auto      metadata = af.read_metadata ();
          auto      label_str = g_markup_printf_escaped (
            _ ("<b>%s</b>\n"
                         "Sample rate: %d\n"
                         "Length: %" PRId64 "s "
                         "%" PRId64 " ms | BPM: %.1f\n"
                         "Channel(s): %u | Bitrate: %'d.%d kb/s\n"
                         "Bit depth: %d bits"),
            this->label_.c_str (), metadata.samplerate, metadata.length / 1000,
            metadata.length % 1000, (double) metadata.bpm, metadata.channels,
            metadata.bit_rate / 1000, (metadata.bit_rate % 1000) / 100,
            metadata.bit_depth);
          std::string ret = label_str;
          g_free (label_str);
          return ret;
        }
      catch (const ZrythmException &e)
        {
          z_warning ("Error reading metadata from {}: {}", abs_path_, e.what ());
          return format_str (_ ("Failed reading metadata for {}"), abs_path_);
        }
    }
  else
    {
      auto label_str = g_markup_printf_escaped (
        "<b>%s</b>\n"
        "Type: %s",
        label_.c_str (), file_type_label.c_str ());
      std::string ret = label_str;
      g_free (label_str);
      return ret;
    }
}

const char *
FileDescriptor::get_icon_name () const
{
  switch (type_)
    {
    case FileType::Midi:
      return "audio-midi";
    case FileType::Mp3:
    case FileType::Flac:
    case FileType::Ogg:
    case FileType::Wav:
      return "gnome-icon-library-sound-wave-symbolic";
    case FileType::Directory:
      return "folder";
    case FileType::ParentDirectory:
      return "folder";
    case FileType::Other:
      return "application-x-zerosize";
    default:
      return "";
    }
}