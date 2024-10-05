// SPDX-FileCopyrightText: © 2020-2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * \file
 *
 * Region link group schema.
 */

#ifndef __SCHEMAS_AUDIO_REGION_LINK_GROUP_H__
#define __SCHEMAS_AUDIO_REGION_LINK_GROUP_H__

#include "schemas/dsp/region_identifier.h"
#include "utils/yaml.h"

typedef struct RegionLinkGroup_v1
{
  int                   schema_version;
  RegionIdentifier_v1 * ids;
  int                   num_ids;
  size_t                ids_size;
  int                   magic;
  int                   group_idx;
} RegionLinkGroup_v1;

static const cyaml_schema_field_t region_link_group_fields_schema_v1[] = {
  YAML_FIELD_INT (RegionLinkGroup_v1, schema_version),
  YAML_FIELD_DYN_ARRAY_VAR_COUNT (
    RegionLinkGroup_v1,
    ids,
    region_identifier_schema_default_v1),
  YAML_FIELD_INT (RegionLinkGroup_v1, group_idx),

  CYAML_FIELD_END
};

static const cyaml_schema_value_t region_link_group_schema_v1 = {
  YAML_VALUE_PTR (RegionLinkGroup_v1, region_link_group_fields_schema_v1),
};

static const cyaml_schema_value_t region_link_group_schema_default_v1 = {
  YAML_VALUE_DEFAULT (RegionLinkGroup_v1, region_link_group_fields_schema_v1),
};

#endif
