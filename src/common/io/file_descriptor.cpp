// SPDX-FileCopyrightText: © 2019-2021, 2023-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "common/io/audio_file.h"
#include "common/io/file_descriptor.h"
#include "common/utils/exceptions.h"
#include "common/utils/format.h"
#include "common/utils/io.h"
#include "common/utils/logger.h"
#include "common/utils/string.h"

using namespace zrythm;

FileDescriptor::FileDescriptor (const std::string &_abs_path)
{
  // z_debug ("creating new FileDescriptor for {}", path);
  abs_path_ = _abs_path;
  type_ = get_type_from_path (_abs_path.c_str ());
  label_ = utils::io::path_get_basename (_abs_path);
}

std::unique_ptr<FileDescriptor>
FileDescriptor::new_from_uri (const std::string &uri)
{
  return std::make_unique<FileDescriptor> (utils::io::uri_to_file (uri));
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
  const auto ext = utils::io::file_get_ext (file);
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
          return format_str (
            QObject::tr (
              "<b>{}</b>\n"
              "Sample rate: {}\n"
              "Length: {}s "
              "{} ms | BPM: {:.1f}\n"
              "Channel(s): {} | Bitrate: {:L}.{} kb/s\n"
              "Bit depth: {} bits")
              .toStdString (),
            string_escape_html (label_), metadata.samplerate,
            metadata.length / 1000, metadata.length % 1000,
            (double) metadata.bpm, metadata.channels, metadata.bit_rate / 1000,
            (metadata.bit_rate % 1000) / 100, metadata.bit_depth);
        }
      catch (const ZrythmException &e)
        {
          z_warning ("Error reading metadata from {}: {}", abs_path_, e.what ());
          return format_str (
            QObject::tr ("Failed reading metadata for {}").toStdString (),
            abs_path_);
        }
    }
  else
    {
      return format_str (
        "<b>{}</b>\nType: {}", string_escape_html (label_),
        string_escape_html (file_type_label));
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