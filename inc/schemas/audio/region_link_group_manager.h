/*
 * Copyright (C) 2020-2021 Alexandros Theodotou <alex at zrythm dot org>
 *
 * This file is part of Zrythm
 *
 * Zrythm is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Zrythm is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with Zrythm.  If not, see <https://www.gnu.org/licenses/>.
 */

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
