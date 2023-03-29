// SPDX-FileCopyrightText: © 2019-2021, 2023 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef __GUI_WIDGETS_TRACKLIST_H__
#define __GUI_WIDGETS_TRACKLIST_H__

#include <gtk/gtk.h>

#define USE_WIDE_HANDLE 1

#define TRACKLIST_WIDGET_TYPE (tracklist_widget_get_type ())
G_DECLARE_FINAL_TYPE (
  TracklistWidget,
  tracklist_widget,
  Z,
  TRACKLIST_WIDGET,
  GtkBox)

/**
 * @addtogroup widgets
 *
 * @{
 */

#define MW_TRACKLIST MW_TIMELINE_PANEL->tracklist

typedef struct _TrackWidget       TrackWidget;
typedef struct _DragDestBoxWidget DragDestBoxWidget;
typedef struct _ChordTrackWidget  ChordTrackWidget;
typedef struct Track              InstrumentTrack;
typedef struct Tracklist          Tracklist;
typedef struct _AddTrackMenuButtonWidget
  AddTrackMenuButtonWidget;

/**
 * The TracklistWidget holds all the Track's
 * in the Project.
 *
 * It is a box with all the pinned tracks, plus a
 * scrolled window at the end containing unpinned
 * tracks and the DragDestBoxWidget at the end.
 */
typedef struct _TracklistWidget
{
  GtkBox parent_instance;

  /** The scrolled window for unpinned tracks. */
  GtkScrolledWindow * unpinned_scroll;

  gulong unpinned_scroll_vall_changed_handler_id;

  /** For DND autoscroll. */
  gulong unpinned_scroll_scroll_up_id;
  gulong unpinned_scroll_scroll_down_id;
  double scroll_speed;

  /** Box to hold pinned tracks. */
  GtkBox * pinned_box;

  /** Box inside unpinned scroll. */
  GtkBox * unpinned_box;

  /**
   * Widget for drag and dropping plugins in to
   * create new tracks.
   */
  DragDestBoxWidget * ddbox;

  /**
   * Internal tracklist.
   */
  Tracklist * tracklist;

  AddTrackMenuButtonWidget * channel_add;

  /** Size group to set the pinned track box and
   * the pinned timeline to the same height. */
  GtkSizeGroup * pinned_size_group;
  GtkSizeGroup * unpinned_size_group;

  /** Cache. */
  GdkRectangle last_allocation;

  bool setup;

  double hover_y;
} TracklistWidget;

/**
 * Sets up the TracklistWidget.
 */
void
tracklist_widget_setup (
  TracklistWidget * self,
  Tracklist *       tracklist);

/**
 * Prepare for finalization.
 */
void
tracklist_widget_tear_down (TracklistWidget * self);

/**
 * Makes sure all the tracks for channels marked as
 * visible are visible.
 */
void
tracklist_widget_update_track_visibility (
  TracklistWidget * self);

/**
 * Generates a menu for adding tracks to the tracklist.
 */
GMenu *
tracklist_widget_generate_add_track_menu (void);

/**
 * Gets hit TrackWidget and the given coordinates.
 */
TrackWidget *
tracklist_widget_get_hit_track (
  TracklistWidget * self,
  double            x,
  double            y);

/**
 * Handle ctrl+shift+scroll.
 */
void
tracklist_widget_handle_vertical_zoom_scroll (
  TracklistWidget *          self,
  GtkEventControllerScroll * scroll_controller,
  double                     dy);

/**
 * Updates the scroll adjustment.
 */
void
tracklist_widget_set_unpinned_scroll_start_y (
  TracklistWidget * self,
  int               y);

/**
 * Refreshes each track without recreating it.
 */
void
tracklist_widget_soft_refresh (TracklistWidget * self);

/**
 * Deletes all tracks and re-adds them.
 */
void
tracklist_widget_hard_refresh (TracklistWidget * self);

/**
 * @}
 */

#endif
