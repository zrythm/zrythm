// SPDX-FileCopyrightText: © 2018-2019, 2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef __GUI_WIDGETS_CHORD_ARRANGER_H__
#define __GUI_WIDGETS_CHORD_ARRANGER_H__

#include "audio/position.h"
#include "gui/backend/tool.h"
#include "gui/widgets/arranger.h"
#include "gui/widgets/main_window.h"

#include <gtk/gtk.h>

#define MW_CHORD_ARRANGER MW_CHORD_EDITOR_SPACE->arranger

typedef struct ChordObject        ChordObject;
typedef struct _ChordObjectWidget ChordObjectWidget;
typedef struct SnapGrid           SnapGrid;
typedef struct AutomationPoint    AutomationPoint;
typedef struct ZRegion            ChordRegion;
typedef struct Channel            Channel;

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
 * Called on drag begin in parent when background is double
 * clicked (i.e., a note is created).
 */
ChordObject *
chord_arranger_widget_create_chord (
  ArrangerWidget * self,
  const Position * pos,
  int              chord_index,
  ZRegion *        region);

/**
 * Called on move items_y setup.
 *
 * calculates the max possible y movement
 */
int
chord_arranger_calc_deltamax_for_chord_movement (int y_delta);

/**
 * @}
 */

#endif
