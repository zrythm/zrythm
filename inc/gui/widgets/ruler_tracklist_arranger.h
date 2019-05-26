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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef __GUI_WIDGETS_RULER_TRACKLIST_ARRANGER_H__
#define __GUI_WIDGETS_RULER_TRACKLIST_ARRANGER_H__

#include "audio/position.h"
#include "gui/backend/tool.h"
#include "gui/backend/ruler_tracklist_selections.h"
#include "gui/widgets/arranger.h"
#include "gui/widgets/main_window.h"
#include "gui/widgets/piano_roll.h"

#include <gtk/gtk.h>

#define RULER_TRACKLIST_ARRANGER_WIDGET_TYPE \
  (ruler_tracklist_arranger_widget_get_type ())
G_DECLARE_FINAL_TYPE (
  RulerTracklistArrangerWidget,
  ruler_tracklist_arranger_widget,
  Z, RULER_TRACKLIST_ARRANGER_WIDGET,
  ArrangerWidget)

#define MW_RULER_TRACKLIST_ARRANGER \
  MW_CENTER_DOCK->ruler_tracklist_arranger

typedef struct _ArrangerBgWidget ArrangerBgWidget;
typedef struct _MarkerWidget MarkerWidget;

typedef struct _RulerTracklistArrangerWidget
{
  ArrangerWidget       parent_instance;

  /** The widget first clicked on. */
  GtkWidget *          start_widget;

  /** The Chord first clicked on. */
  ZChord *             start_chord;

  /** Type of the widget first clicked on. */
  GType                start_widget_type;

  /* These are created from the
   * RulerTracklistArrangerSelections so
   * the indexes match. They are set after the
   * selections are made, on drag_begin, and used in
   * drag_update to get positions relative to the
   * initial ones. */

  /* FIXME due to size limitations, create a new
   * struct that will hold this and only save its
   * pointer here */

  /** Initial start positions. */
  Position              chord_start_poses[500];

  /** Initial start positions. */
  Position              marker_start_poses[500];

} RulerTracklistArrangerWidget;

/**
 * To be called from get_child_position in parent
 * widget.
 *
 * Used to allocate the overlay children.
 */
void
ruler_tracklist_arranger_widget_set_allocation (
  RulerTracklistArrangerWidget * self,
  GtkWidget *          widget,
  GdkRectangle *       allocation);

/**
 * Gets the Track (ChordTrack, MarkerTrack, etc.).
 */
Track *
ruler_tracklist_arranger_widget_get_track_at_y (
  RulerTracklistArrangerWidget * self,
  double y);

/**
 * Sets transient object and actual object
 * visibility based on the current action.
 */
void
ruler_tracklist_arranger_widget_update_visibility (
  RulerTracklistArrangerWidget * self);

/**
 * Returns the appropriate cursor based on the
 * current hover_x and y.
 */
ArrangerCursor
ruler_tracklist_arranger_widget_get_cursor (
  RulerTracklistArrangerWidget * self,
  UiOverlayAction action,
  Tool            tool);

MarkerWidget *
ruler_tracklist_arranger_widget_get_hit_marker (
  RulerTracklistArrangerWidget *  self,
  double            x,
  double            y);

ChordWidget *
ruler_tracklist_arranger_widget_get_hit_chord (
  RulerTracklistArrangerWidget *  self,
  double                    x,
  double                    y);

void
ruler_tracklist_arranger_widget_select_all (
  RulerTracklistArrangerWidget *  self,
  int                       select);

/**
 * Shows context menu.
 *
 * To be called from parent on right click.
 */
void
ruler_tracklist_arranger_widget_show_context_menu (
  RulerTracklistArrangerWidget * self,
  gdouble              x,
  gdouble              y);

void
ruler_tracklist_arranger_widget_on_drag_begin_marker_hit (
  RulerTracklistArrangerWidget * self,
  double                   start_x,
  MarkerWidget *           rw);

void
ruler_tracklist_arranger_widget_on_drag_begin_chord_hit (
  RulerTracklistArrangerWidget * self,
  double                   start_x,
  ChordWidget *            cw);

/**
 * Fills in the positions that the
 * RulerTracklistArranger
 * remembers at the start of each drag.
 */
void
ruler_tracklist_arranger_widget_set_init_poses (
  RulerTracklistArrangerWidget * self);

void
ruler_tracklist_arranger_widget_create_chord (
  RulerTracklistArrangerWidget * self,
  Track *                  track,
  Position *               pos);

void
ruler_tracklist_arranger_widget_create_marker (
  RulerTracklistArrangerWidget * self,
  Track *                  track,
  Position *               pos);

/**
 * First determines the selection type (objects/
 * range), then either finds and selects items or
 * selects a range.
 *
 * @param[in] delete If this is a select-delete
 *   operation.
 */
void
ruler_tracklist_arranger_widget_select (
  RulerTracklistArrangerWidget * self,
  double                   offset_x,
  double                   offset_y,
  int                  delete);

void
ruler_tracklist_arranger_widget_move_items_x (
  RulerTracklistArrangerWidget * self,
  long                     ticks_diff);

void
ruler_tracklist_arranger_widget_on_drag_end (
  RulerTracklistArrangerWidget * self);

/**
 * Sets width to ruler width and height to
 * tracklist height.
 */
void
ruler_tracklist_arranger_widget_set_size (
  RulerTracklistArrangerWidget * self);

void
ruler_tracklist_arranger_widget_setup (
  RulerTracklistArrangerWidget * self);

/**
 * Readd children.
 */
void
ruler_tracklist_arranger_widget_refresh_children (
  RulerTracklistArrangerWidget * self);

#endif

