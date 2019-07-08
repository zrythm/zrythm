/*
 * Copyright (C) 2019 Alexandros Theodotou <alex at zrythm dot org>
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
 * Marker related code.
 */

#ifndef __AUDIO_MARKER_H__
#define __AUDIO_MARKER_H__

#include <stdint.h>

#include "audio/position.h"
#include "gui/backend/arranger_object.h"
#include "gui/backend/arranger_object_info.h"
#include "utils/yaml.h"

/**
 * @addtogroup audio
 *
 * @{
 */

#define marker_is_main(c) \
  arranger_object_info_is_main ( \
    &c->obj_info)

#define marker_is_transient(r) \
  arranger_object_info_is_transient ( \
    &r->obj_info)

/** Gets the main counterpart of the Marker. */
#define marker_get_main_marker(r) \
  ((Marker *) r->obj_info.main)

/** Gets the transient counterpart of the Marker. */
#define marker_get_main_trans_marker(r) \
  ((Marker *) r->obj_info.main_trans)

typedef enum MarkerCloneFlag
{
  MARKER_CLONE_COPY_MAIN,
  MARKER_CLONE_COPY,
} MarkerCloneFlag;

/**
 * Marker type.
 */
typedef enum MarkerType
{
  /** Song start Marker that cannot be deleted. */
  MARKER_TYPE_START,
  /** Song end Marker that cannot be deleted. */
  MARKER_TYPE_END,
  /** Custom Marker. */
  MARKER_TYPE_CUSTOM,
} MarkerType;

typedef struct _MarkerWidget MarkerWidget;

/**
 * Marker for the MarkerTrack.
 */
typedef struct Marker
{
  /** Marker position. */
  Position          pos;

  /** Cache, used in runtime operations. */
  Position          cache_pos;

  /** Marker type. */
  MarkerType        type;

  /** Name of Marker to be displayed in the UI. */
  char *            name;

  /** Position of Track this ChordObject is in. */
  int            track_pos;

  /** Cache. */
  Track *        track;

  /** Widget used to represent the Marker in the
   * UI. */
  MarkerWidget *    widget;

  ArrangerObjectInfo   obj_info;
} Marker;

static const cyaml_schema_field_t
  marker_fields_schema[] =
{
  CYAML_FIELD_MAPPING (
    "pos", CYAML_FLAG_DEFAULT,
    Marker, pos, position_fields_schema),

	CYAML_FIELD_END
};

static const cyaml_schema_value_t
marker_schema = {
  CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER,
    Marker, marker_fields_schema),
};

void
marker_init_loaded (Marker * self);

/**
 * Creates a Marker.
 */
Marker *
marker_new (
  const char * name,
  int          is_main);

/**
 * Returns if the two Marker's are equal.
 */
int
marker_is_equal (
  Marker * a,
  Marker * b);

DECLARE_IS_ARRANGER_OBJ_SELECTED (
  Marker, marker);

DECLARE_ARRANGER_OBJ_MOVE (
  Marker, marker);

ARRANGER_OBJ_DECLARE_SHIFT_TICKS (Marker, marker);

/**
 * Sets the Track of the Marker.
 */
void
marker_set_track (
  Marker * marker,
  Track *  track);

/**
 * Sets the name to all the Marker's counterparts.
 */
void
marker_set_name (
  Marker * marker,
  const char * name);

ARRANGER_OBJ_DECLARE_GEN_WIDGET (
  Marker, marker);

/**
 * Finds the marker in the project corresponding to
 * the given one.
 */
Marker *
marker_find (
  Marker * clone);

DECLARE_ARRANGER_OBJ_SET_POS (
  Marker, marker);

Marker *
marker_clone (
  Marker * src,
  MarkerCloneFlag flag);

/**
 * Frees the Marker.
 */
void
marker_free (
  Marker * self);

/**
 * @}
 */

#endif
