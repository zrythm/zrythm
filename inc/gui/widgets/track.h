/*
 * gui/widgets/track.h - Track view
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

#ifndef __GUI_WIDGETS_TRACK_H__
#define __GUI_WIDGETS_TRACK_H__

#include <gtk/gtk.h>

#define TRACK_WIDGET_TYPE \
  (track_widget_get_type ())
G_DECLARE_DERIVABLE_TYPE (TrackWidget,
                      track_widget,
                      TRACK,
                      WIDGET,
                      GtkGrid)
#define IS_TRACK_WIDGET(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
   TRACK_WIDGET_TYPE))

#define TRACK_WIDGET_GET_PRIVATE(self) \
  TrackWidgetPrivate * tw_prv = \
    track_widget_get_private (TRACK_WIDGET (self));

typedef struct _ColorAreaWidget ColorAreaWidget;
typedef struct Track Track;

typedef struct
{
  /**
   * The color on the left.
   */
  ColorAreaWidget *             color;

  /**
   * The track top / bot paned splitting the main track
   * content and the bottom content (automation tracklist).
   */
  GtkPaned *                    paned;

  /**
   * The top part of the track.
   */
  GtkBox *                      top_grid;

  GtkLabel *                    name; ///< track name
  GtkImage *                    icon; ///< the icon

  /**
   * These are boxes to be filled by inheriting widgets.
   */
  GtkBox *                      upper_controls;
  GtkBox *                      right_activity_box;
  GtkBox *                      mid_controls;
  GtkBox *                      bot_controls;

  GtkEventBox *                 event_box;

  Track *                       track; ///< associated track
} TrackWidgetPrivate;

typedef struct _TrackWidgetClass
{
  GtkGridClass parent_class;
} TrackWidgetClass;

/**
 * Sets up the track widget.
 *
 * Sets color, draw callback, etc.
 */
TrackWidget *
track_widget_new (Track * track);

void
track_widget_select (TrackWidget * self,
                     int           select); ///< 1 = select, 0 = unselect

/**
 * Wrapper.
 */
void
track_widget_refresh (TrackWidget * self);

TrackWidgetPrivate *
track_widget_get_private (TrackWidget * self);

void
track_widget_on_show_automation (GtkWidget * widget,
                                 void *      data);

GtkWidget *
track_widget_get_bottom_paned (TrackWidget * self);

#endif
