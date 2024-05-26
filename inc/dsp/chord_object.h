// clang-format off
// SPDX-FileCopyrightText: © 2018-2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense
// clang-format on

/**
 * \file
 *
 * Chord object in the TimelineArranger.
 */

#ifndef __AUDIO_CHORD_OBJECT_H__
#define __AUDIO_CHORD_OBJECT_H__

#include <cstdint>

#include "dsp/chord_descriptor.h"
#include "dsp/position.h"
#include "dsp/region_identifier.h"
#include "gui/backend/arranger_object.h"
#include "utils/yaml.h"

/**
 * @addtogroup dsp
 *
 * @{
 */

#define CHORD_OBJECT_MAGIC 4181694
#define IS_CHORD_OBJECT(x) (((ChordObject *) x)->magic == CHORD_OBJECT_MAGIC)
#define IS_CHORD_OBJECT_AND_NONNULL(x) (x && IS_CHORD_OBJECT (x))

#define CHORD_OBJECT_WIDGET_TRIANGLE_W 10

#define chord_object_is_selected(r) \
  arranger_object_is_selected ((ArrangerObject *) r)

typedef struct ChordDescriptor ChordDescriptor;

/**
 * A ChordObject to be shown in the TimelineArrangerWidget.
 *
 * @extends ArrangerObject
 */
typedef struct ChordObject
{
  /** Base struct. */
  ArrangerObject base;

  /** The index inside the region. */
  int index;

  /** The index of the chord it belongs to (0 topmost). */
  int chord_index;

  int magic;

  /** Cache layout for drawing the name. */
  PangoLayout * layout;
} ChordObject;

/**
 * Creates a ChordObject.
 */
ChordObject *
chord_object_new (RegionIdentifier * region_id, int chord_index, int index);

int
chord_object_is_equal (ChordObject * a, ChordObject * b);

/**
 * Sets the region and index of the chord.
 */
void
chord_object_set_region_and_index (ChordObject * self, ZRegion * region, int idx);

/**
 * Returns the ChordDescriptor associated with this
 * ChordObject.
 */
ChordDescriptor *
chord_object_get_chord_descriptor (const ChordObject * self);

/**
 * Finds the ChordObject in the project
 * corresponding to the given one's position.
 */
ChordObject *
chord_object_find_by_pos (ChordObject * clone);

ZRegion *
chord_object_get_region (ChordObject * self);

/**
 * @}
 */

#endif
