/*
 * Copyright (C) 2018-2020 Alexandros Theodotou <alex at zrythm dot org>
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
 * along with Zrythm.  If not, see <https://www.gnu.org/licenses/>.
 */

/**
 * \file
 *
 * Track widget to be shown in the tracklist.
 */

#ifndef __GUI_WIDGETS_TRACK_H__
#define __GUI_WIDGETS_TRACK_H__

#include <gtk/gtk.h>

#define TRACK_WIDGET_TYPE \
  (track_widget_get_type ())
G_DECLARE_FINAL_TYPE (
  TrackWidget, track_widget, Z, TRACK_WIDGET,
  GtkBox)

typedef struct Track Track;
typedef struct CustomButtonWidget CustomButtonWidget;
typedef struct _ArrangerWidget ArrangerWidget;
typedef struct _MeterWidget MeterWidget;
typedef struct AutomationModeWidget
  AutomationModeWidget;

/**
 * @addtogroup widgets
 *
 * @{
 */

/**
 * Resize target.
 */
typedef enum TrackWidgetResizeTarget
{
  TRACK_WIDGET_RESIZE_TARGET_TRACK,
  TRACK_WIDGET_RESIZE_TARGET_AT,
  TRACK_WIDGET_RESIZE_TARGET_LANE,
} TrackWidgetResizeTarget;

/**
 * The TrackWidget is split into 3 parts.
 *
 * - 1. Top part contains the "main" view.
 * - 2. Lane part contains each lane.
 * - 3. Automation tracklist part contains each
 *      automation track.
 */
typedef struct _TrackWidget
{
  GtkBox               parent_instance;

  /** Main box containing the drawing area and the
   * meters on the right. */
  GtkBox *             main_box;

  /** Track icon, currently not placed anywhere
   * but used by the ColorAreaWidget to get the
   * pixbuf. */
  char                 icon_name[60];

  GtkGestureDrag *          drag;
  GtkGestureMultiPress *    multipress;

  /** Right-click gesture. */
  GtkGestureMultiPress *    right_mouse_mp;

  /** If drag update was called at least once. */
  int                 dragged;

  /** Number of clicks, used when selecting/moving/
   * dragging channels. */
  int                 n_press;

  /**
   * Set between enter-leave signals.
   *
   * This is because hover can continue to send
   * signals when
   * hovering over other overlayed widgets (buttons,
   * etc.).
   */
  int                 bg_hovered;

  /**
   * Set when the drag should resize instead of dnd.
   *
   * This is used to determine if we should resize
   * on drag begin.
   */
  int                 resize;

  /** Set during the whole resizing action. */
  int                 resizing;

  /** Resize target type (track/at/lane). */
  TrackWidgetResizeTarget  resize_target_type;

  /** The object to resize. */
  void *                   resize_target;

  /** Associated Track. */
  Track *                   track;

  /** Control held down on drag begin. */
  int                 ctrl_held_at_start;

  /** Used for highlighting. */
  GtkBox *            highlight_top_box;
  GtkBox *            highlight_bot_box;

  /** The track selection processing was done in
   * the dnd callbacks, so no need to do it in
   * drag_end. */
  int                 selected_in_dnd;

  /** Cache layout for drawing the name. */
  PangoLayout *       layout;

  /** For drag actions. */
  double              start_x;
  double              start_y;
  double              last_offset_y;

  /** Used during hovering to remember the last
   * known cursor position. */
  double              last_x;
  double              last_y;

  /** Used when mouse button is held down to
   * mark buttons as clicked. */
  int                 button_pressed;

  /** Currently clicked button. */
  CustomButtonWidget * clicked_button;

  /** Currently clicked automation mode button. */
  AutomationModeWidget * clicked_am;

  GtkDrawingArea *    drawing_area;

  /**
   * Signal handler IDs for tracks that have them.
   *
   * This is more convenient instead of having them
   * in each widget.
   */
  //gulong              record_toggle_handler_id;
  //gulong              solo_toggled_handler_id;
  //gulong              mute_toggled_handler_id;

  /** Buttons to be drawin in order. */
  CustomButtonWidget * top_buttons[8];
  int                  num_top_buttons;
  CustomButtonWidget * bot_buttons[8];
  int                  num_bot_buttons;

  MeterWidget *        meter_l;
  MeterWidget *        meter_r;

  /** Last MIDI event trigger time, for MIDI
   * ports. */
  gint64            last_midi_out_trigger_time;

  /** Set to 1 to redraw. */
  int                redraw;

  /** Cairo caches. */
  cairo_t *          cached_cr;
  cairo_surface_t *  cached_surface;
} TrackWidget;

/**
 * Sets up the track widget.
 *
 * Sets color, draw callback, etc.
 */
TrackWidget *
track_widget_new (Track * track);

/**
 * Sets the Track name on the TrackWidget.
 */
void
track_widget_set_name (
  TrackWidget * self,
  const char * name);

/**
 * Blocks all signal handlers.
 */
//void
//track_widget_block_all_signal_handlers (
  //TrackWidget * self);

/**
 * Unblocks all signal handlers.
 */
//void
//track_widget_unblock_all_signal_handlers (
  //TrackWidget * self);

/**
 * Wrapper.
 */
void
track_widget_force_redraw (
  TrackWidget * self);

/**
 * Wrapper to refresh mute button only.
 */
//void
//track_widget_refresh_buttons (
  //TrackWidget * self);

/**
 * Callback when automation button is toggled.
 */
void
track_widget_on_show_automation_toggled (
  TrackWidget * self);

/**
 * Callback when automation button is toggled.
 */
void
track_widget_on_show_lanes_toggled (
  TrackWidget * self);

/**
 * Callback when record button is toggled.
 */
void
track_widget_on_record_toggled (
  TrackWidget * self);

/**
 * Returns if cursor is in top half of the track.
 *
 * Used by timeline to determine if it will select
 * objects or range.
 */
int
track_widget_is_cursor_in_top_half (
  TrackWidget * self,
  double        y);

/**
 * Updates the full track size and redraws the
 * track.
 */
void
track_widget_update_size (
  TrackWidget * self);

/**
 * Highlights/unhighlights the Tracks
 * appropriately.
 *
 * @param highlight 1 to highlight top or bottom,
 *   0 to unhighlight all.
 */
void
track_widget_do_highlight (
  TrackWidget * self,
  gint          x,
  gint          y,
  const int     highlight);

/**
 * Converts Y from the arranger coordinates to
 * the track coordinates.
 */
int
track_widget_get_local_y (
  TrackWidget *    self,
  ArrangerWidget * arranger,
  int              arranger_y);

/**
 * Causes a redraw of the meters only.
 */
void
track_widget_redraw_meters (
  TrackWidget * self);

/**
 * @}
 */

#endif
