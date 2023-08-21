// SPDX-FileCopyrightText: © 2019-2021, 2023 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * \file
 *
 * Supported file info.
 */

#ifndef __AUDIO_SUPPORTED_FILE_H__
#define __AUDIO_SUPPORTED_FILE_H__

#include <stdbool.h>

#include "utils/yaml.h"

typedef struct _WrappedObjectWithChangeSignal
  WrappedObjectWithChangeSignal;

/**
 * @addtogroup utils
 *
 * @{
 */

#define SUPPORTED_FILE_DND_PREFIX \
  Z_DND_STRING_PREFIX "SupportedFile::"

/**
 * File type.
 */
typedef enum ZFileType
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
} ZFileType;

static const cyaml_strval_t file_type_strings[] = {
  {"MIDI",        FILE_TYPE_MIDI      },
  { "mp3",        FILE_TYPE_MP3       },
  { "FLAC",       FILE_TYPE_FLAC      },
  { "ogg",        FILE_TYPE_OGG       },
  { "wav",        FILE_TYPE_WAV       },
  { "directory",  FILE_TYPE_DIR       },
  { "parent dir", FILE_TYPE_PARENT_DIR},
  { "other",      FILE_TYPE_OTHER     },
};

/**
 * Metadata for a supported file.
 */
typedef struct SupportedFile
{
  /** Absolute path. */
  char * abs_path;

  /** Type of file. */
  ZFileType type;

  /** Human readable label. */
  char * label;

  /** Hidden or not. */
  int hidden;

  /** MIDI file, if midi. */
  //MidiFile *     midi_file;

  /** Audio file, if audio. */
  //AudioFile *    midi_file;

  /** Used in Gtk. */
  WrappedObjectWithChangeSignal * gobj;
} SupportedFile;

static const cyaml_schema_field_t supported_file_fields_schema[] = {
  YAML_FIELD_STRING_PTR (SupportedFile, abs_path),
  YAML_FIELD_ENUM (SupportedFile, type, file_type_strings),
  YAML_FIELD_STRING_PTR_OPTIONAL (SupportedFile, label),
  YAML_FIELD_INT (SupportedFile, hidden),

  CYAML_FIELD_END
};

static const cyaml_schema_value_t supported_file_schema = {
  YAML_VALUE_PTR_NULLABLE (
    SupportedFile,
    supported_file_fields_schema),
};

/**
 * Creates a new SupportedFile from the given absolute
 * path.
 */
SupportedFile *
supported_file_new_from_path (const char * path);

SupportedFile *
supported_file_new_from_uri (const char * uri, GError ** error);

/**
 * Returns a human readable description of the given
 * file type.
 *
 * Example: wav -> "Wave file".
 */
char *
supported_file_type_get_description (ZFileType type);

/**
 * Clones the given SupportedFile.
 */
SupportedFile *
supported_file_clone (const SupportedFile * src);

/**
 * Returns if the given type is supported.
 */
int
supported_file_type_is_supported (ZFileType type);

/**
 * Returns if the SupportedFile is an audio file.
 */
int
supported_file_type_is_audio (ZFileType type);

/**
 * Returns if the SupportedFile is a midi file.
 */
int
supported_file_type_is_midi (ZFileType type);

/**
 * Returns the most common extension for the given
 * filetype.
 */
const char *
supported_file_type_get_ext (ZFileType type);

/**
 * Returns the file type of the given file path.
 */
NONNULL ZFileType
supported_file_get_type (const char * file);

/**
 * Returns whether the given file should auto-play
 * (shorter than 1 min).
 */
NONNULL bool
supported_file_should_autoplay (const SupportedFile * self);

/**
 * Gets the corresponding icon name for the given
 * SupportedFile's type.
 */
const char *
supported_file_get_icon_name (
  const SupportedFile * const self);

/**
 * Returns a pango markup to be used in GTK labels.
 */
NONNULL char *
supported_file_get_info_text_for_label (
  const SupportedFile * self);

/**
 * Frees the instance and all its members.
 */
void
supported_file_free (SupportedFile * self);

/**
 * @}
 */

#endif
