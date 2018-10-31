/*
 * inc/gui/widgets/arranger.h - MIDI arranger widget
 *
 * Copyright (C) 2018 Alexandros Theodotou
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef __GUI_WIDGETS_ARRANGER_H__
#define __GUI_WIDGETS_ARRANGER_H__

#include "gui/widgets/main_window.h"
#include "gui/widgets/midi_editor.h"
#include "audio/position.h"

#include <gtk/gtk.h>

#define ARRANGER_WIDGET_TYPE                  (arranger_widget_get_type ())
#define ARRANGER_WIDGET(obj)                  (G_TYPE_CHECK_INSTANCE_CAST ((obj), ARRANGER_WIDGET_TYPE, ArrangerWidget))
#define ARRANGER_WIDGET_CLASS(klass)          (G_TYPE_CHECK_CLASS_CAST  ((klass), ARRANGER_WIDGET, ArrangerWidgetClass))
#define IS_ARRANGER_WIDGET(obj)               (G_TYPE_CHECK_INSTANCE_TYPE ((obj), ARRANGER_WIDGET_TYPE))
#define IS_ARRANGER_WIDGET_CLASS(klass)       (G_TYPE_CHECK_CLASS_TYPE  ((klass), ARRANGER_WIDGET_TYPE))
#define ARRANGER_WIDGET_GET_CLASS(obj)        (G_TYPE_INSTANCE_GET_CLASS  ((obj), ARRANGER_WIDGET_TYPE, ArrangerWidgetClass))

#define MW_TIMELINE MAIN_WINDOW->timeline
#define MIDI_ARRANGER MIDI_EDITOR->midi_arranger

typedef struct ArrangerBgWidget ArrangerBgWidget;
typedef struct MidiNote MidiNote;
typedef struct SnapGrid SnapGrid;
typedef struct AutomationPoint AutomationPoint;

typedef enum ArrangerWidgetType
{
  ARRANGER_TYPE_TIMELINE,
  ARRANGER_TYPE_MIDI
} ArrangerWidgetType;

typedef enum ArrangerWidgetAction
{
  ARRANGER_ACTION_NONE,
  ARRANGER_ACTION_RESIZING_L,
  ARRANGER_ACTION_RESIZING_R,
  ARRANGER_ACTION_MOVING,
  ARRANGER_ACTION_CHANGING_CURVE,
  ARRANGER_ACTION_SELECTING
} ArrangerWidgetAction;

/**
 * Used internally.
 */
typedef enum ArrangerChildType
{
  ARRANGER_CHILD_TYPE_MIDI_NOTE,
  ARRANGER_CHILD_TYPE_REGION,
  ARRANGER_CHILD_TYPE_AP
} ArrangerChildType;

typedef struct ArrangerWidget
{
  GtkOverlay               parent_instance;
  ArrangerWidgetType       type;
  GtkDrawingArea           * bg;
  GtkGestureDrag           * drag;
  GtkGestureMultiPress     * multipress;
  double                   last_y;
  double                   last_offset_x;  ///< for dragging regions
  ArrangerWidgetAction     action;
  Region                   * region;  ///< region doing action upon (timeline)
  AutomationPoint *        ap;  ///< automation point doing action upon (timeline)
  MidiNote                 * midi_note;  ///< MIDI note doing action upon (midi)
  double                   start_x; ///< for dragging
  double                   start_y; ///< for dragging
  double                   start_pos_px; ///< for moving regions
  Position                 start_pos; ///< for moving regions
  Position                 end_pos; ///< for moving regions
  int                      n_press; ///< for multipress
  SnapGrid                 * snap_grid; ///< associated snap grid
} ArrangerWidget;

typedef struct ArrangerWidgetClass
{
  GtkOverlayClass       parent_class;
} ArrangerWidgetClass;

/**
 * Creates a timeline widget using the given timeline data.
 */
ArrangerWidget *
arranger_widget_new (ArrangerWidgetType type, SnapGrid * snap_grid);

/**
 * Sets up the MIDI editor for the given region.
 */
void
arranger_widget_set_channel (ArrangerWidget * arranger, Channel * channel);

/**
 * Gets x position in pixels
 */
int
arranger_get_x_pos_in_px (Position * pos);

#endif


