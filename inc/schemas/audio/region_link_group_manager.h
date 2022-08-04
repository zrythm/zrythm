// SPDX-FileCopyrightText: © 2020-2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * \file
 *
 * Region link group manager schema.
 */

#ifndef __SCHEMAS_AUDIO_REGION_LINK_GROUP_MANAGER_H__
#define __SCHEMAS_AUDIO_REGION_LINK_GROUP_MANAGER_H__

#include "utils/yaml.h"

#include "schemas/audio/region_link_group.h"

typedef struct RegionLinkGroupManager_v1
{
  int                  schema_version;
  RegionLinkGroup_v1 * groups;
  int                  num_groups;
  size_t               groups_size;
} RegionLinkGroupManager_v1;

static const cyaml_schema_field_t
  region_link_group_manager_fields_schema_v1[] = {
    YAML_FIELD_INT (RegionLinkGroupManager_v1, schema_version),
    YAML_FIELD_DYN_ARRAY_VAR_COUNT (
      RegionLinkGroupManager_v1,
      groups,
      region_link_group_schema_default_v1),

    CYAML_FIELD_END
  };

static const cyaml_schema_value_t
  region_link_group_manager_schema_v1 = {
    YAML_VALUE_PTR (
      RegionLinkGroupManager_v1,
      region_link_group_manager_fields_schema_v1),
  };

#endif
