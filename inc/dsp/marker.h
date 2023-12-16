// SPDX-FileCopyrightText: © 2019-2021 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * \file
 *
 * Marker related code.
 */

#ifndef __AUDIO_MARKER_H__
#define __AUDIO_MARKER_H__

#include <stdint.h>

#include "dsp/position.h"
#include "gui/backend/arranger_object.h"
#include "utils/yaml.h"

/**
 * @addtogroup dsp
 *
 * @{
 */

#define MARKER_WIDGET_TRIANGLE_W 10

#define marker_is_selected(r) arranger_object_is_selected ((ArrangerObject *) r)

#define marker_is_deletable(m) \
  ((m)->type != MARKER_TYPE_START && (m)->type != MARKER_TYPE_END)

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

static const cyaml_strval_t marker_type_strings[] = {
  {"start",   MARKER_TYPE_START },
  { "end",    MARKER_TYPE_END   },
  { "custom", MARKER_TYPE_CUSTOM},
};

/**
 * Marker for the MarkerTrack.
 */
typedef struct Marker
{
  /** Base struct. */
  ArrangerObject base;

  /** Marker type. */
  MarkerType type;

  /** Name of Marker to be displayed in the UI. */
  char * name;

  /** Escaped name for drawing. */
  char * escaped_name;

  /** Position of the marker track this marker is
   * in. */
  unsigned int track_name_hash;

  /** Index in the track. */
  int index;

  /** Cache layout for drawing the name. */
  PangoLayout * layout;
} Marker;

/**
 * Creates a Marker.
 */
Marker *
marker_new (const char * name);

/**
 * Returns if the two Marker's are equal.
 */
int
marker_is_equal (Marker * a, Marker * b);

void
marker_set_index (Marker * self, int index);

/**
 * Sets the Track of the Marker.
 */
void
marker_set_track_name_hash (Marker * marker, unsigned int track_name_hash);

Marker *
marker_find_by_name (const char * name);

/**
 * @}
 */

#endif
