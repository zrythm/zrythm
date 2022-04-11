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
 * Region link group schema.
 */

#ifndef __SCHEMAS_AUDIO_REGION_LINK_GROUP_H__
#define __SCHEMAS_AUDIO_REGION_LINK_GROUP_H__

#include <stdbool.h>

#include "utils/yaml.h"

#include "schemas/audio/region_identifier.h"

typedef struct RegionLinkGroup_v1
{
  int                   schema_version;
  RegionIdentifier_v1 * ids;
  int                   num_ids;
  size_t                ids_size;
  int                   magic;
  int                   group_idx;
} RegionLinkGroup_v1;

static const cyaml_schema_field_t
  region_link_group_fields_schema_v1[] = {
    YAML_FIELD_INT (
      RegionLinkGroup_v1,
      schema_version),
    YAML_FIELD_DYN_ARRAY_VAR_COUNT (
      RegionLinkGroup_v1,
      ids,
      region_identifier_schema_default_v1),
    YAML_FIELD_INT (RegionLinkGroup_v1, group_idx),

    CYAML_FIELD_END
  };

static const cyaml_schema_value_t
  region_link_group_schema_v1 = {
    YAML_VALUE_PTR (
      RegionLinkGroup_v1,
      region_link_group_fields_schema_v1),
  };

static const cyaml_schema_value_t
  region_link_group_schema_default = {
    YAML_VALUE_DEFAULT (
      RegionLinkGroup_v1,
      region_link_group_fields_schema_v1),
  };

#endif
