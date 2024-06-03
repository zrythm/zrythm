// clang-format off
// SPDX-FileCopyrightText: © 2019-2021, 2023 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense
// clang-format on

/**
 * \file
 *
 * Supported file info.
 */

#ifndef __AUDIO_SUPPORTED_FILE_H__
#define __AUDIO_SUPPORTED_FILE_H__

#include <memory>

#include <glib.h>

typedef struct _WrappedObjectWithChangeSignal WrappedObjectWithChangeSignal;

/**
 * @addtogroup utils
 *
 * @{
 */

#define SUPPORTED_FILE_DND_PREFIX Z_DND_STRING_PREFIX "FileDescriptor::"

/**
 * File type.
 */
enum class ZFileType
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
};

/**
 * Descriptor of a file.
 */
struct FileDescriptor
{
  FileDescriptor () = default;

  FileDescriptor (const char * abs_path);

  static std::unique_ptr<FileDescriptor>
  new_from_uri (const char * uri, GError ** error);

  /**
   * Returns a human readable description of the given file type.
   *
   * Example: wav -> "Wave file".
   */
  static char * get_type_description (ZFileType type);

  /**
   * Returns if the given type is supported.
   */
  static bool is_type_supported (ZFileType type);

  /**
   * Returns if the FileDescriptor is an audio file.
   */
  static bool is_type_audio (ZFileType type);

  /**
   * Returns if the FileDescriptor is a midi file.
   */
  static bool is_type_midi (ZFileType type);

  /**
   * Returns the most common extension for the given filetype.
   */
  static const char * get_type_ext (ZFileType type);

  /**
   * Returns the file type of the given file path.
   */
  static ZFileType get_type_from_path (const char * file);

  bool is_audio () const { return is_type_audio (type); }

  bool is_midi () const { return is_type_midi (type); }

  bool is_supported () const { return is_type_supported (type); }

  /**
   * Returns whether the given file should auto-play (shorter than 1 min).
   */
  bool should_autoplay () const;

  /**
   * Gets the corresponding icon name for the given FileDescriptor's type.
   */
  const char * get_icon_name () const;

  /**
   * Returns a pango markup to be used in GTK labels.
   */
  char * get_info_text_for_label () const;

  /** Absolute path. */
  std::string abs_path;

  /** Type of file. */
  ZFileType type = {};

  /** Human readable label. */
  std::string label;

  /** Hidden or not. */
  bool hidden = false;
};

/**
 * @}
 */

#endif
