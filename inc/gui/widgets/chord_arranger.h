// SPDX-FileCopyrightText: © 2018-2019, 2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef __GUI_WIDGETS_CHORD_ARRANGER_H__
#define __GUI_WIDGETS_CHORD_ARRANGER_H__

#include "dsp/chord_region.h"
#include "dsp/position.h"

#include "gtk_wrapper.h"

class ChordObject;
TYPEDEF_STRUCT_UNDERSCORED (ArrangerWidget);

#define MW_CHORD_ARRANGER MW_CHORD_EDITOR_SPACE->arranger

/**
 * @addtogroup widgets
 *
 * @{
 */

/**
 * Returns the chord index at y.
 */
int
chord_arranger_widget_get_chord_at_y (double y);

/**
 * Called on drag begin in parent when background is double clicked (i.e., a
 * note is created).
 */
void
chord_arranger_widget_create_chord (
  ArrangerWidget * self,
  const Position   pos,
  int              chord_index,
  ChordRegion     &region);

/**
 * Called on move items_y setup.
 *
 * calculates the max possible y movement
 */
int
chord_arranger_calc_deltamax_for_chord_movement (int y_delta);

void
chord_arranger_on_drag_end (ArrangerWidget * self);

/**
 * @}
 */

#endif
