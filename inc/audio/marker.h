/*
 * Copyright (C) 2019 Alexandros Theodotou <alex at zrythm dot org>
 *
 * This file is part of Zrythm
 *
 * Zrythm is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Zrythm is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
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
#include "utils/yaml.h"

/**
 * @addtogroup audio
 *
 * @{
 */

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

  /** Selected or not. */
  int               selected;

  /** Visible or not. */
  int               visible;

  /** Widget used to represent the Marker in the
   * UI. */
  MarkerWidget *    widget;
} Marker;

static const cyaml_schema_field_t
  marker_fields_schema[] =
{
  CYAML_FIELD_MAPPING (
    "pos", CYAML_FLAG_DEFAULT,
    Marker, pos, position_fields_schema),
	CYAML_FIELD_INT (
    "selected", CYAML_FLAG_DEFAULT,
    Marker, selected),

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
  Position * pos);

static inline int
marker_is_equal (
  Marker * a,
  Marker * b)
{
  return !position_compare(&a->pos, &b->pos) &&
    !g_strcmp0 (a->name, b->name) &&
    a->visible == b->visible;
}

/**
 * Finds the marker in the project corresponding to
 * the given one.
 */
Marker *
marker_find (
  Marker * clone);

/**
 * Sets the Marker Position.
 */
void
marker_set_pos (
  Marker *    self,
  Position * pos);

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
