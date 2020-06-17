/*
 * Copyright (C) 2019 Alexandros Theodotou <alex at zrythm dot org>
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

#include "audio/marker_track.h"
#include "audio/track.h"
#include "audio/tracklist.h"
#include "gui/widgets/center_dock.h"
#include "gui/widgets/main_window.h"
#include "gui/widgets/timeline_arranger.h"
#include "gui/widgets/timeline_minimap_bg.h"
#include "gui/widgets/timeline_panel.h"
#include "project.h"
#include "zrythm_app.h"

#include <gtk/gtk.h>

G_DEFINE_TYPE (TimelineMinimapBgWidget,
               timeline_minimap_bg_widget,
               GTK_TYPE_DRAWING_AREA)

static gboolean
timeline_minimap_bg_draw_cb (
  GtkWidget *widget,
  cairo_t *cr,
  gpointer data)
{
  if (!PROJECT->loaded)
    return FALSE;

  GtkStyleContext * context =
    gtk_widget_get_style_context (widget);

  int width =
    gtk_widget_get_allocated_width (widget);
  int height =
    gtk_widget_get_allocated_height (widget);

  gtk_render_background (
    context, cr, 0, 0, width, height);

  Marker * start =
    marker_track_get_start_marker (
      P_MARKER_TRACK);
  ArrangerObject * start_obj =
    (ArrangerObject *) start;
  Marker * end =
    marker_track_get_end_marker (
      P_MARKER_TRACK);
  ArrangerObject * end_obj =
    (ArrangerObject *) end;
  int song_px =
    ui_pos_to_px_timeline (
      &start_obj->pos, 1) -
    ui_pos_to_px_timeline (
      &end_obj->pos, 1);
  /*int region_px;*/

  int total_track_height = 0;
  Track * track;
  for (int i = 0; i < TRACKLIST->num_tracks; i++)
    {
      track = TRACKLIST->tracks[i];

      if (track->widget && track->visible)
        total_track_height +=
          gtk_widget_get_allocated_height (
            GTK_WIDGET (track->widget));
    }
  int track_height;
  for (int i = 0; i < TRACKLIST->num_tracks; i++)
    {
      track = TRACKLIST->tracks[i];

      if (!(track->widget && track->visible))
        continue;

      gint wx, wy;
      gtk_widget_translate_coordinates(
        GTK_WIDGET (track->widget),
        GTK_WIDGET (MW_TIMELINE),
        0, 0,
        &wx, &wy);
      track_height =
        gtk_widget_get_allocated_height (
          GTK_WIDGET (track->widget));

      GdkRGBA * color = &track->color;
      cairo_set_source_rgba (cr,
                             color->red,
                             color->green,
                             color->blue,
                             0.2);

      TrackLane * lane;
      ArrangerObject * r_obj;
      for (int j = 0; j < track->num_lanes; j++)
        {
          lane = track->lanes[j];

          for (int k = 0; k < lane->num_regions; k++)
            {
              r_obj =
                (ArrangerObject *) lane->regions[k];

              int px_start =
                ui_pos_to_px_timeline (
                  &r_obj->pos, 1);
              int px_end =
                ui_pos_to_px_timeline (
                  &r_obj->end_pos, 1);
              int px_length =
                px_end - px_start;

              cairo_rectangle (
                cr,
                ((double) px_start /
                 (double) song_px) * width,
                ((double) wy /
                   (double) total_track_height) * height,
                ((double) px_length /
                  (double) song_px) * width,
                ((double) track_height /
                  (double) total_track_height) * height);
              cairo_fill (cr);
            }
        }
    }


  return FALSE;
}

TimelineMinimapBgWidget *
timeline_minimap_bg_widget_new ()
{
  TimelineMinimapBgWidget * self =
    g_object_new (TIMELINE_MINIMAP_BG_WIDGET_TYPE,
                  NULL);

  return self;
}

static void
timeline_minimap_bg_widget_class_init (
  TimelineMinimapBgWidgetClass * _klass)
{
  GtkWidgetClass * klass = GTK_WIDGET_CLASS (_klass);
  gtk_widget_class_set_css_name (klass,
                                 "timeline-minimap-bg");
}

static void
timeline_minimap_bg_widget_init (
  TimelineMinimapBgWidget * self)
{
  gtk_widget_set_visible (GTK_WIDGET (self),
                          1);

  g_signal_connect (
    G_OBJECT (self), "draw",
    G_CALLBACK (timeline_minimap_bg_draw_cb), NULL);
}
