// SPDX-FileCopyrightText: © 2019-2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * \file
 *
 * Audio pool schema.
 */

#ifndef __SCHEMAS_AUDIO_POOL_H__
#define __SCHEMAS_AUDIO_POOL_H__

#include "gui/backend/backend/cyaml_schemas/dsp/clip.h"
#include "utils/yaml.h"

typedef struct AudioPool_v1
{
  int             schema_version;
  AudioClip_v1 ** clips;
  int             num_clips;
} AudioPool_v1;

static const cyaml_schema_field_t audio_pool_fields_schema_v1[] = {
  YAML_FIELD_INT (AudioPool_v1, schema_version),
  YAML_FIELD_DYN_ARRAY_VAR_COUNT (AudioPool_v1, clips, audio_clip_schema_v1),

  CYAML_FIELD_END
};

static const cyaml_schema_value_t audio_pool_schema_v1 = {
  YAML_VALUE_PTR (AudioPool_v1, audio_pool_fields_schema_v1),
};

#endif
