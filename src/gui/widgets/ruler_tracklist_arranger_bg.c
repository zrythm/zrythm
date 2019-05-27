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
 * along with Zrythm.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "audio/ruler_tracklist.h"
#include "audio/track.h"
#include "gui/widgets/arranger.h"
#include "gui/widgets/center_dock.h"
#include "gui/widgets/main_window.h"
#include "gui/widgets/ruler_tracklist_arranger_bg.h"
#include "gui/widgets/ruler_tracklist.h"
#include "gui/widgets/track.h"
#include "project.h"
#include "utils/cairo.h"

#include <gtk/gtk.h>

G_DEFINE_TYPE (
  RulerTracklistArrangerBgWidget,
  ruler_tracklist_arranger_bg_widget,
  ARRANGER_BG_WIDGET_TYPE)

static gboolean
ruler_tracklist_arranger_bg_draw_cb (
  GtkWidget *widget,
  cairo_t *cr,
  gpointer data)
{
  GdkRectangle rect;
  gdk_cairo_get_clip_rectangle (
    cr, &rect);

  /* handle horizontal drawing for tracks */
  GtkWidget * tw_widget;
  gint wx, wy;
  Track * tracks[3];
  tracks[0] = RULER_TRACKLIST->chord_track;
  tracks[1] = RULER_TRACKLIST->marker_track;
  int num_tracks = 2;
  Track * track;
  TrackWidget * tw;
  for (int i = 0; i < num_tracks; i++)
    {
      track = tracks[i];
      if (!track->visible)
        continue;

      /* draw line below widget */
      tw = track->widget;
      tw_widget = GTK_WIDGET (tw);
      if (!GTK_IS_WIDGET (tw_widget))
        continue;

      gtk_widget_translate_coordinates (
        tw_widget,
        GTK_WIDGET (MW_RULER_TRACKLIST),
        0, 0, &wx, &wy);
      int line_y =
        wy + gtk_widget_get_allocated_height (
          GTK_WIDGET (tw_widget));
      if (line_y > rect.y &&
          line_y < (rect.y + rect.height))
        z_cairo_draw_horizontal_line (
          cr, line_y, rect.x,
          rect.x + rect.width, 1.0);
    }

  return 0;
}

RulerTracklistArrangerBgWidget *
ruler_tracklist_arranger_bg_widget_new (
  RulerWidget *    ruler,
  ArrangerWidget * arranger)
{
  RulerTracklistArrangerBgWidget * self =
    g_object_new (
      RULER_TRACKLIST_ARRANGER_BG_WIDGET_TYPE,
      NULL);

  ARRANGER_BG_WIDGET_GET_PRIVATE (self);
  ab_prv->ruler = ruler;
  ab_prv->arranger = arranger;

  return self;
}

static void
ruler_tracklist_arranger_bg_widget_class_init (
  RulerTracklistArrangerBgWidgetClass * _klass)
{
}

static void
ruler_tracklist_arranger_bg_widget_init (
  RulerTracklistArrangerBgWidget *self )
{
  gtk_widget_add_events (GTK_WIDGET (self),
                         GDK_ALL_EVENTS_MASK);

  gtk_widget_set_visible (
    GTK_WIDGET (self), 1);

  g_signal_connect (
    G_OBJECT (self), "draw",
    G_CALLBACK (ruler_tracklist_arranger_bg_draw_cb),
    NULL);
}
