/*
 * gui/instrument_timeline_view.h - The view of an instrument left of its
 *   timeline counterpart
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
 * along with Zrythm.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef __GUI_WIDGETS_TRACKLIST_H__
#define __GUI_WIDGETS_TRACKLIST_H__

#include <gtk/gtk.h>

#define USE_WIDE_HANDLE 1

#define TRACKLIST_WIDGET_TYPE                  (tracklist_widget_get_type ())
#define TRACKLIST_WIDGET(obj)                  (G_TYPE_CHECK_INSTANCE_CAST ((obj), TRACKLIST_WIDGET_TYPE, TracklistWidget))
#define TRACKLIST_WIDGET_CLASS(klass)          (G_TYPE_CHECK_CLASS_CAST  ((klass), TRACKLIST_WIDGET, TracklistWidgetClass))
#define IS_TRACKLIST_WIDGET(obj)               (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TRACKLIST_WIDGET_TYPE))
#define IS_TRACKLIST_WIDGET_CLASS(klass)       (G_TYPE_CHECK_CLASS_TYPE  ((klass), TRACKLIST_WIDGET_TYPE))
#define TRACKLIST_WIDGET_GET_CLASS(obj)        (G_TYPE_INSTANCE_GET_CLASS  ((obj), TRACKLIST_WIDGET_TYPE, TracklistWidgetClass))

#define FOREACH_SELECTED_TRACKS for (int i = 0; i < self->num_selected_tracks; \
                                     i++)
#define FOREACH_TW for (int i = 0; i < self->num_track_widgets; i++)
#define MW_TRACKLIST MAIN_WINDOW->tracklist

typedef struct TrackWidget TrackWidget;
typedef struct DragDestBoxWidget DragDestBoxWidget;

typedef struct TracklistWidget
{
  GtkBox                        parent_instance;  ///< parent paned with master
  GtkGestureDrag *              drag;
  GtkGestureMultiPress *        multipress;
  GtkGestureMultiPress *        right_mouse_mp; ///< right mouse multipress
  TrackWidget *                 master_tw; ///< master track widget
  TrackWidget *                 track_widgets[100]; ///< track paneds, in the order they appear.
                                      ///< invisible tracks do not get stored here,
                                      ///< track widgets are created dynamically
  int                           num_track_widgets;
  Track *                       selected_tracks[100];
  int                           num_selected_tracks;
  DragDestBoxWidget *           ddbox; ///< drag destination box for dropping
                                    ///< plugins in
} TracklistWidget;

typedef struct TracklistWidgetClass
{
  GtkBoxClass    parent_class;
} TracklistWidgetClass;

/**
 * Creates a new tracklist widget and sets it to main window.
 */
TracklistWidget *
tracklist_widget_new ();

/**
 * Adds master track.
 */
void
tracklist_widget_add_master_track (TracklistWidget * self);

/**
 * Adds a track to the tracklist widget.
 *
 * Must NOT be used with master track (see tracklist_widget_add_master_track).
 */
void
tracklist_widget_add_track (TracklistWidget * self,
                            Track *           track,
                            int               pos); ///< position to insert at,
                                                  ///< starting from 0 after master
/**
 * Removes the given track from the tracklist.
 */
void
tracklist_widget_remove_track (Track * track);

void
tracklist_widget_toggle_select_track (TracklistWidget * self,
                               Track *           track,
                               int               append); ///< append to selections

/**
 * Selects or deselects all tracks.
 */
void
tracklist_widget_toggle_select_all_tracks (TracklistWidget *self,
                                           int              select);

/**
 * Makes sure all the tracks for channels marked as visible are visible.
 */
void
tracklist_widget_show (TracklistWidget *self);


Track *
tracklist_widget_get_top_track ();

Track *
tracklist_widget_get_prev_visible_track (Track * track);

Track *
tracklist_widget_get_next_visible_track (Track * track);

Track *
tracklist_widget_get_bot_track ();

#endif
