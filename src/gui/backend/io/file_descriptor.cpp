// SPDX-FileCopyrightText: © 2019-2021, 2023-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "gui/backend/io/file_descriptor.h"
#include "utils/audio_file.h"
#include "utils/exceptions.h"
#include "utils/format.h"
#include "utils/io.h"
#include "utils/logger.h"
#include "utils/string.h"

using namespace zrythm;

FileDescriptor::FileDescriptor (const fs::path &_abs_path)
    : type_ (get_type_from_path (_abs_path))
{
  // z_debug ("creating new FileDescriptor for {}", path);
  abs_path_ = _abs_path;
  label_ =
    utils::Utf8String::from_path (utils::io::path_get_basename (_abs_path));
}

std::unique_ptr<FileDescriptor>
FileDescriptor::new_from_uri (const utils::Utf8String &uri)
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

utils::Utf8String
FileDescriptor::get_type_description (FileType type)
{
  switch (type)
    {
    case FileType::Midi:
      return u8"MIDI";
    case FileType::Mp3:
      return u8"MP3";
    case FileType::Flac:
      return u8"FLAC";
    case FileType::Ogg:
      return u8"OGG (Vorbis)";
    case FileType::Wav:
      return u8"Wave";
    case FileType::Directory:
      return u8"Directory";
    case FileType::ParentDirectory:
      return u8"Parent directory";
    case FileType::Other:
      return u8"Other";
    default:
      z_return_val_if_reached ({});
    }
}

FileType
FileDescriptor::get_type_from_path (const fs::path &file)
{
  const auto ext = utils::Utf8String::from_path (utils::io::file_get_ext (file));
  FileType type = FileType::Other;

  if (fs::is_directory (file))
    type = FileType::Directory;
  else if (ext.empty ())
    type = FileType::Other;
  else if (
    ext.is_equal_ignore_case (u8"MID") || ext.is_equal_ignore_case (u8"MIDI")
    || ext.is_equal_ignore_case (u8"SMF"))
    type = FileType::Midi;
  else if (ext.is_equal_ignore_case (u8"mp3"))
    type = FileType::Mp3;
  else if (ext.is_equal_ignore_case (u8"flac"))
    type = FileType::Flac;
  else if (ext.is_equal_ignore_case (u8"ogg"))
    type = FileType::Ogg;
  else if (ext.is_equal_ignore_case (u8"wav"))
    type = FileType::Wav;
  else
    type = FileType::Other;

  return type;
}

std::optional<utils::Utf8String>
FileDescriptor::get_type_ext (FileType type)
{
  switch (type)
    {
    case FileType::Midi:
      return u8"mid";
      break;
    case FileType::Mp3:
      return u8"mp3";
      break;
    case FileType::Flac:
      return u8"flac";
      break;
    case FileType::Ogg:
      return u8"ogg";
      break;
    case FileType::Wav:
      return u8"wav";
      break;
    case FileType::ParentDirectory:
      return std::nullopt;
      break;
    case FileType::Directory:
      return std::nullopt;
      break;
    case FileType::Other:
      return std::nullopt;
      break;
    default:
      z_return_val_if_reached (std::nullopt);
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
          zrythm::utils::audio::AudioFile af (abs_path_);
          auto                            metadata = af.read_metadata ();
          if ((metadata.length / 1000) > 60)
            {
              autoplay = false;
            }
        }
      catch (const ZrythmException &e)
        {
          e.handle (fmt::format ("Error reading metadata from {}", abs_path_));
          return false;
        }
    }

  return autoplay;
}

utils::Utf8String
FileDescriptor::get_info_text_for_label () const
{
  auto file_type_label = get_type_description (type_);
  if (is_audio ())
    {
      try
        {
          utils::audio::AudioFile af (abs_path_);
          auto                    metadata = af.read_metadata ();
          return utils::Utf8String::from_qstring (format_qstr (
            QObject::tr (
              "<b>{}</b>\n"
              "Sample rate: {}\n"
              "Length: {}s "
              "{} ms | BPM: {:.1f}\n"
              "Channel(s): {} | Bitrate: {:L}.{} kb/s\n"
              "Bit depth: {} bits"),
            label_.escape_html (), metadata.samplerate, metadata.length / 1000,
            metadata.length % 1000, (double) metadata.bpm, metadata.channels,
            metadata.bit_rate / 1000, (metadata.bit_rate % 1000) / 100,
            metadata.bit_depth));
        }
      catch (const ZrythmException &e)
        {
          z_warning (
            "Error reading metadata from {}: {}",
            utils::Utf8String::from_path (abs_path_), e.what ());
          return utils::Utf8String::from_qstring (format_qstr (
            QObject::tr ("Failed reading metadata for {}"),
            utils::Utf8String::from_path (abs_path_)));
        }
    }
  else
    {
      return utils::Utf8String::from_utf8_encoded_string (format_str (
        "<b>{}</b>\nType: {}", label_.escape_html (),
        file_type_label.escape_html ()));
    }
}

utils::Utf8String
FileDescriptor::get_icon_name () const
{
  switch (type_)
    {
    case FileType::Midi:
      return u8"audio-midi";
    case FileType::Mp3:
    case FileType::Flac:
    case FileType::Ogg:
    case FileType::Wav:
      return u8"gnome-icon-library-sound-wave-symbolic";
    case FileType::Directory:
    case FileType::ParentDirectory:
      return u8"folder";
    case FileType::Other:
      return u8"application-x-zerosize";
    default:
      return {};
    }
}
