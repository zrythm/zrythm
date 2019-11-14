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
 * along with Zrythm.  If not, see <https://www.gnu.org/licenses/>.
 */

/**
 * \file
 *
 * Track base widget to be inherited.
 */

#ifndef __GUI_WIDGETS_TRACK_H__
#define __GUI_WIDGETS_TRACK_H__

#include <gtk/gtk.h>

#define TRACK_WIDGET_TYPE \
  (track_widget_get_type ())
G_DECLARE_DERIVABLE_TYPE (
  TrackWidget,
  track_widget,
  Z, TRACK_WIDGET,
  GtkEventBox)

/**
 * @addtogroup widgets
 *
 * @{
 */

#define TRACK_WIDGET_GET_PRIVATE(self) \
  TrackWidgetPrivate * tw_prv = \
    track_widget_get_private (Z_TRACK_WIDGET (self));

#define SET_TRACK_ICON(icon_name) \
  tw_prv->icon = \
    gtk_icon_theme_load_icon ( \
      gtk_icon_theme_get_default (), \
      icon_name, \
      16, \
      GTK_ICON_LOOKUP_FORCE_SVG | \
        GTK_ICON_LOOKUP_FORCE_SIZE, \
      NULL)

typedef struct _ColorAreaWidget ColorAreaWidget;
typedef struct Track Track;
typedef struct _DzlMultiPaned DzlMultiPaned;
typedef struct _TrackTopGridWidget
  TrackTopGridWidget;
typedef struct _TrackLanelistWidget
  TrackLanelistWidget;

/**
 * The TrackWidget is split into 3 parts inside a
 * DzlMultiPaned.
 *
 * - 1. TrackTopGridWidget contains the "main" view.
 * - 2. TrackLanelistWidget contains the
 *   TrackLaneWidgets.
 * - 3. AutomationTracklistWidget contains the
 *   AutomationLaneWidgets.
 */
typedef struct
{
  /** The color on the left. */
  ColorAreaWidget *         color;

  /** Track icon, currently not placed anywhere
   * but used by the ColorAreaWidget to get the
   * pixbuf. */
  GdkPixbuf *                icon;

  /** This is the main grid of the track and is
   * the top part of the paned. */
  GtkGrid *                 main_grid;

  /**
   * This is the widget in the bottom half of the
   * paned.
   *
   * It will either be another track, or the
   * drag_dest_box, or nothing if this is the last
   * track in the pinned tracklist.
   */
  GtkWidget *               bot_widget;

  GtkGestureDrag *          drag;
  GtkGestureMultiPress *    multipress;

  /** Right-click gesture. */
  GtkGestureMultiPress *    right_mouse_mp;

  /**
   * The track multipane splitting the main
   * track content, the track lanes, and the bottom
   * content (automation tracklist).
   */
  GtkBox *                 paned;

  /** The top part of the TrackWidget. */
  TrackTopGridWidget *      top_grid;

  /** Box holding the TrackLanes. */
  //GtkBox *                  lanes_box;

  /** The track lanes. */
  TrackLanelistWidget *     lanelist;

  GtkEventBox *             event_box;

  /** If drag update was called at least once. */
  int                 dragged;

  /** Number of clicks, used when selecting/moving/
   * dragging channels. */
  int                 n_press;

  /**
   * Set between enter-leave signals.
   *
   * This is because hover can continue to send signals when
   * hovering over other overlayed widgets (buttons, etc.).
   */
  int                 bg_hovered;

  /** Set when the drag should resize instead of dnd. */
  int                 resize;

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

  /**
   * Signal handler IDs for tracks that have them.
   *
   * This is more convenient instead of having them
   * in each widget.
   */
  gulong              record_toggle_handler_id;
  gulong              solo_toggled_handler_id;
  gulong              mute_toggled_handler_id;
} TrackWidgetPrivate;

typedef struct _TrackWidgetClass
{
  GtkPanedClass parent_class;
} TrackWidgetClass;

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

void
track_widget_on_solo_toggled (
  GtkToggleButton * btn,
  void *            data);

/**
 * General handler for tracks that have mute
 * buttons.
 */
void
track_widget_on_mute_toggled (
  GtkToggleButton * btn,
  TrackWidget *     data);

/**
 * Blocks all signal handlers.
 */
void
track_widget_block_all_signal_handlers (
  TrackWidget * self);

/**
 * Unblocks all signal handlers.
 */
void
track_widget_unblock_all_signal_handlers (
  TrackWidget * self);

/**
 * Wrapper.
 */
void
track_widget_refresh (TrackWidget * self);

/**
 * Wrapper to refresh mute button only.
 */
void
track_widget_refresh_buttons (
  TrackWidget * self);

TrackWidgetPrivate *
track_widget_get_private (TrackWidget * self);

/**
 * Callback when automation button is toggled.
 */
void
track_widget_on_show_automation_toggled (
  GtkWidget * widget,
  TrackWidget * self);

/**
 * Callback when automation button is toggled.
 */
void
track_widget_on_show_lanes_toggled (
  GtkWidget * widget,
  TrackWidget * self);

/**
 * Callback when record button is toggled.
 */
void
track_widget_on_record_toggled (
  GtkWidget * widget,
  void *      data);

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
 * @}
 */

#endif
