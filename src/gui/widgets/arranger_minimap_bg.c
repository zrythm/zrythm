// clang-format off
// SPDX-FileCopyrightText: © 2019, 2021-2023 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense
// clang-format on

#include "dsp/marker_track.h"
#include "dsp/track.h"
#include "dsp/tracklist.h"
#include "gui/widgets/arranger_minimap.h"
#include "gui/widgets/arranger_minimap_bg.h"
#include "gui/widgets/center_dock.h"
#include "gui/widgets/main_notebook.h"
#include "gui/widgets/main_window.h"
#include "gui/widgets/timeline_panel.h"
#include "gui/widgets/tracklist.h"
#include "project.h"
#include "utils/ui.h"
#include "zrythm_app.h"

#include <gtk/gtk.h>

G_DEFINE_TYPE (
  ArrangerMinimapBgWidget,
  arranger_minimap_bg_widget,
  GTK_TYPE_WIDGET)

static void
draw_timeline (GtkWidget * widget, GtkSnapshot * snapshot)
{
  int width = gtk_widget_get_width (widget);
  int height = gtk_widget_get_height (widget);

  Marker *         start = marker_track_get_start_marker (P_MARKER_TRACK);
  ArrangerObject * start_obj = (ArrangerObject *) start;
  Marker *         end = marker_track_get_end_marker (P_MARKER_TRACK);
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
        total_track_height += gtk_widget_get_height (GTK_WIDGET (track->widget));
    }
  int track_height;
  for (int i = 0; i < TRACKLIST->num_tracks; i++)
    {
      track = TRACKLIST->tracks[i];

      if (!track->widget || !track->visible)
        continue;

      graphene_point_t wpt;
      bool             success = gtk_widget_compute_point (
        GTK_WIDGET (track->widget), GTK_WIDGET (MW_TRACKLIST->unpinned_box),
        &GRAPHENE_POINT_INIT (0.f, 0.f), &wpt);
      g_return_if_fail (success);
      track_height = gtk_widget_get_height (GTK_WIDGET (track->widget));

      GdkRGBA color = track->color;
      color.alpha = 0.6f;

      TrackLane *      lane;
      ArrangerObject * r_obj;
      for (int j = 0; j < track->num_lanes; j++)
        {
          lane = track->lanes[j];

          for (int k = 0; k < lane->num_regions; k++)
            {
              r_obj = (ArrangerObject *) lane->regions[k];

              int px_start = ui_pos_to_px_timeline (&r_obj->pos, 1);
              int px_end = ui_pos_to_px_timeline (&r_obj->end_pos, 1);
              int px_length = px_end - px_start;

              gtk_snapshot_append_color (
                snapshot, &color,
                &GRAPHENE_RECT_INIT (
                  ((float) px_start / (float) song_px) * (float) width,
                  ((float) wpt.y / (float) total_track_height) * (float) height,
                  ((float) px_length / (float) song_px) * (float) width,
                  ((float) track_height / (float) total_track_height)
                    * (float) height));
            }
        }
    }
}

static void
arranger_minimap_bg_snapshot (GtkWidget * widget, GtkSnapshot * snapshot)
{
  if (!PROJECT->loaded)
    return;

  ArrangerMinimapBgWidget * self = Z_ARRANGER_MINIMAP_BG_WIDGET (widget);

  if (self->owner->type == ARRANGER_MINIMAP_TYPE_TIMELINE)
    {
      draw_timeline (widget, snapshot);
    }
  else
    {
      /* TODO */
    }
}

static gboolean
arranger_minimap_bg_tick_cb (
  GtkWidget *     widget,
  GdkFrameClock * frame_clock,
  gpointer        user_data)
{
  gtk_widget_queue_draw (widget);

  return G_SOURCE_CONTINUE;
}

ArrangerMinimapBgWidget *
arranger_minimap_bg_widget_new (ArrangerMinimapWidget * owner)
{
  ArrangerMinimapBgWidget * self =
    g_object_new (ARRANGER_MINIMAP_BG_WIDGET_TYPE, NULL);

  self->owner = owner;

  return self;
}

static void
arranger_minimap_bg_widget_class_init (ArrangerMinimapBgWidgetClass * klass)
{
  GtkWidgetClass * wklass = GTK_WIDGET_CLASS (klass);
  wklass->snapshot = arranger_minimap_bg_snapshot;
  gtk_widget_class_set_css_name (wklass, "arranger-minimap-bg");
}

static void
arranger_minimap_bg_widget_init (ArrangerMinimapBgWidget * self)
{
  gtk_widget_add_tick_callback (
    GTK_WIDGET (self), arranger_minimap_bg_tick_cb, self, NULL);
}
