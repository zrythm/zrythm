// SPDX-FileCopyrightText: © 2018-2021 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * \file
 *
 * Scale object inside the chord Track in the
 * TimelineArranger.
 */

#ifndef __AUDIO_SCALE_OBJECT_H__
#define __AUDIO_SCALE_OBJECT_H__

#include <cstdint>

#include "dsp/position.h"
#include "dsp/scale.h"
#include "gui/backend/arranger_object.h"

typedef struct MusicalScale MusicalScale;

/**
 * @addtogroup dsp
 *
 * @{
 */

#define scale_object_is_selected(r) \
  arranger_object_is_selected ((ArrangerObject *) r)

#define SCALE_OBJECT_MAGIC 13187994
#define IS_SCALE_OBJECT(tr) (tr && tr->magic == SCALE_OBJECT_MAGIC)

/**
 * A ScaleObject to be shown in the
 * TimelineArrangerWidget.
 *
 * @extends ArrangerObject
 */
typedef struct ScaleObject
{
  /** Base struct. */
  ArrangerObject base;

  MusicalScale * scale;

  int index;

  int magic;

  /** Cache layout for drawing the name. */
  PangoLayout * layout;
} ScaleObject;

/**
 * Creates a ScaleObject.
 */
ScaleObject *
scale_object_new (MusicalScale * descr);

void
scale_object_set_index (ScaleObject * self, int index);

int
scale_object_is_equal (ScaleObject * a, ScaleObject * b);

/**
 * @}
 */

#endif
