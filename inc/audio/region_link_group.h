/*
 * Copyright (C) 2020 Alexandros Theodotou <alex at zrythm dot org>
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
 * Group of linked regions.
 */

#ifndef __AUDIO_REGION_LINK_GROUP_H__
#define __AUDIO_REGION_LINK_GROUP_H__

#include "audio/region_identifier.h"
#include "utils/yaml.h"

typedef struct ZRegion ZRegion;

/**
 * @addtogroup audio
 *
 * @{
 */

#define REGION_LINK_GROUP_MAGIC 1222013
#define IS_REGION_LINK_GROUP(x) \
  (((RegionLinkGroup *) (x))->magic == \
     REGION_LINK_GROUP_MAGIC)

/**
 * A group of linked regions.
 */
typedef struct RegionLinkGroup
{
  /** Identifiers for regions in this link group. */
  RegionIdentifier * ids;
  int                num_ids;
  int                ids_size;

  int                magic;

  /** Group index. */
  int                group_idx;
} RegionLinkGroup;

static const cyaml_schema_field_t
  region_link_group_fields_schema[] =
{
  YAML_FIELD_DYN_ARRAY_VAR_COUNT (
    RegionLinkGroup, ids,
    region_identifier_schema_default),
  YAML_FIELD_INT (
    RegionLinkGroup, group_idx),

  CYAML_FIELD_END
};

static const cyaml_schema_value_t
  region_link_group_schema =
{
  CYAML_VALUE_MAPPING (CYAML_FLAG_POINTER,
    RegionLinkGroup, region_link_group_fields_schema),
};

static const cyaml_schema_value_t
  region_link_group_schema_default =
{
  CYAML_VALUE_MAPPING (CYAML_FLAG_DEFAULT,
    RegionLinkGroup, region_link_group_fields_schema),
};

void
region_link_group_init (
  RegionLinkGroup * self,
  int               idx);

void
region_link_group_add_region (
  RegionLinkGroup * self,
  ZRegion *         region);

void
region_link_group_remove_region (
  RegionLinkGroup * self,
  ZRegion *         region);

/**
 * @}
 */

#endif
