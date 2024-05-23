// SPDX-FileCopyrightText: © 2019-2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * \file
 *
 * Audio clip schema.
 */

#ifndef __SCHEMAS_AUDIO_CLIP_H__
#define __SCHEMAS_AUDIO_CLIP_H__

#include <stdbool.h>

#include "utils/audio.h"
#include "utils/types.h"
#include "utils/yaml.h"

typedef enum BitDepth_v1
{
  BIT_DEPTH_16_v1,
  BIT_DEPTH_24_v1,
  BIT_DEPTH_32_v1
} BitDepth_v1;

typedef struct AudioClip_v1
{
  int         schema_version;
  char *      name;
  bpm_t       bpm;
  int         samplerate;
  BitDepth_v1 bit_depth;
  bool        use_flac;
  int         pool_id;
  char *      file_hash;
} AudioClip_v1;

static const cyaml_strval_t bit_depth_strings_v1[] = {
  {"16",  BIT_DEPTH_16_v1},
  { "24", BIT_DEPTH_24_v1},
  { "32", BIT_DEPTH_32_v1},
};

static const cyaml_schema_field_t audio_clip_fields_schema_v1[] = {
  YAML_FIELD_INT (AudioClip_v1, schema_version),
  YAML_FIELD_STRING_PTR (AudioClip_v1, name),
  YAML_FIELD_STRING_PTR_OPTIONAL (AudioClip_v1, file_hash),
  YAML_FIELD_FLOAT (AudioClip_v1, bpm),
  YAML_FIELD_ENUM (AudioClip_v1, bit_depth, bit_depth_strings_v1),
  YAML_FIELD_INT (AudioClip_v1, use_flac),
  YAML_FIELD_INT (AudioClip_v1, samplerate),
  YAML_FIELD_INT (AudioClip_v1, pool_id),

  CYAML_FIELD_END
};

static const cyaml_schema_value_t audio_clip_schema_v1 = {
  YAML_VALUE_PTR_NULLABLE (AudioClip_v1, audio_clip_fields_schema_v1),
};

#endif
