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

#include "gui/backend/tool.h"
#include "gui/widgets/arranger.h"
#include "gui/widgets/main_window.h"
#include "gui/widgets/piano_roll.h"
#include "audio/position.h"

#include <gtk/gtk.h>

#define CHORD_ARRANGER_WIDGET_TYPE \
  (chord_arranger_widget_get_type ())
G_DECLARE_FINAL_TYPE (
  ChordArrangerWidget,
  chord_arranger_widget,
  Z, CHORD_ARRANGER_WIDGET,
  ArrangerWidget)

#define CHORD_ARRANGER MW_PIANO_ROLL->arranger

typedef struct _ArrangerBgWidget ArrangerBgWidget;
typedef struct ChordObject ChordObject;
typedef struct _ChordObjectWidget ChordObjectWidget;
typedef struct SnapGrid SnapGrid;
typedef struct AutomationPoint AutomationPoint;
typedef struct Region ChordRegion;
typedef struct Channel Channel;

typedef struct _ChordArrangerWidget
{
  ArrangerWidget  parent_instance;

  /**
   * Start ChordObject acting on. This is the
   * ChordObject that was clicked, even though
   * there could be more selected.
   */
  ChordObject *   start_chord_object;
} ChordArrangerWidget;

ARRANGER_W_DECLARE_FUNCS (
  Chord, chord);
ARRANGER_W_DECLARE_CHILD_OBJ_FUNCS (
  Chord, chord, ChordObject, chord);

/**
 * Called on drag begin in parent when background is double
 * clicked (i.e., a note is created).
 */
void
chord_arranger_widget_create_chord (
  ChordArrangerWidget * self,
  const Position *      pos);

#endif
