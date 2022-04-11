/*
 * Copyright (C) 2019, 2021-2022 Alexandros Theodotou <alex at zrythm dot org>
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
#include "gui/widgets/main_notebook.h"
#include "gui/widgets/main_window.h"
#include "gui/widgets/timeline_arranger.h"
#include "gui/widgets/timeline_minimap_bg.h"
#include "gui/widgets/timeline_panel.h"
#include "project.h"
#include "zrythm_app.h"

#include <gtk/gtk.h>

G_DEFINE_TYPE (
  TimelineMinimapBgWidget,
  timeline_minimap_bg_widget,
  GTK_TYPE_WIDGET)

static void
timeline_minimap_bg_snapshot (
  GtkWidget *   widget,
  GtkSnapshot * snapshot)
{
  /*TimelineMinimapBgWidget * self =*/
  /*Z_TIMELINE_MINIMAP_BG_WIDGET (widget);*/

  int width =
    gtk_widget_get_allocated_width (widget);
  int height =
    gtk_widget_get_allocated_height (widget);

  GtkStyleContext * context =
    gtk_widget_get_style_context (widget);

  gtk_snapshot_render_background (
    snapshot, context, 0, 0, width, height);

  if (!PROJECT->loaded)
    return;

  Marker * start =
    marker_track_get_start_marker (P_MARKER_TRACK);
  ArrangerObject * start_obj =
    (ArrangerObject *) start;
  Marker * end =
    marker_track_get_end_marker (P_MARKER_TRACK);
  ArrangerObject * end_obj = (ArrangerObject *) end;
  int              song_px =
    ui_pos_to_px_timeline (&end_obj->pos, 1)
    - ui_pos_to_px_timeline (&start_obj->pos, 1);
  /*int region_px;*/

  int     total_track_height = 0;
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

      if (!track->widget || !track->visible)
        continue;

      double wy;
      gtk_widget_translate_coordinates (
        GTK_WIDGET (track->widget),
        GTK_WIDGET (MW_TIMELINE), 0, 0, NULL, &wy);
      track_height =
        gtk_widget_get_allocated_height (
          GTK_WIDGET (track->widget));

      GdkRGBA color = track->color;
      color.alpha = 0.6f;

      TrackLane *      lane;
      ArrangerObject * r_obj;
      for (int j = 0; j < track->num_lanes; j++)
        {
          lane = track->lanes[j];

          for (int k = 0; k < lane->num_regions; k++)
            {
              r_obj =
                (ArrangerObject *) lane->regions[k];

              int px_start = ui_pos_to_px_timeline (
                &r_obj->pos, 1);
              int px_end = ui_pos_to_px_timeline (
                &r_obj->end_pos, 1);
              int px_length = px_end - px_start;

              gtk_snapshot_append_color (
                snapshot, &color,
                &GRAPHENE_RECT_INIT (
                  ((float) px_start / (float) song_px)
                    * width,
                  ((float) wy
                   / (float) total_track_height)
                    * height,
                  ((float) px_length
                   / (float) song_px)
                    * width,
                  ((float) track_height
                   / (float) total_track_height)
                    * height));
            }
        }
    }
}

static gboolean
timeline_minimap_bg_tick_cb (
  GtkWidget *     widget,
  GdkFrameClock * frame_clock,
  gpointer        user_data)
{
  gtk_widget_queue_draw (widget);

  return G_SOURCE_CONTINUE;
}

TimelineMinimapBgWidget *
timeline_minimap_bg_widget_new ()
{
  TimelineMinimapBgWidget * self = g_object_new (
    TIMELINE_MINIMAP_BG_WIDGET_TYPE, NULL);

  return self;
}

static void
timeline_minimap_bg_widget_class_init (
  TimelineMinimapBgWidgetClass * klass)
{
  GtkWidgetClass * wklass =
    GTK_WIDGET_CLASS (klass);
  wklass->snapshot = timeline_minimap_bg_snapshot;
  gtk_widget_class_set_css_name (
    wklass, "timeline-minimap-bg");
}

static void
timeline_minimap_bg_widget_init (
  TimelineMinimapBgWidget * self)
{
  gtk_widget_add_tick_callback (
    GTK_WIDGET (self), timeline_minimap_bg_tick_cb,
    self, NULL);
}
