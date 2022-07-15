/*
 * Copyright (C) 2018-2019 Alexandros Theodotou <alex at zrythm dot org>
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

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
void
chord_arranger_widget_create_chord (
  ArrangerWidget * self,
  const Position * pos,
  int              chord_index,
  ZRegion *        region);

/**
 * @}
 */

#endif
