// SPDX-FileCopyrightText: © 2019, 2021-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

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
#include "utils/gtk.h"
#include "utils/ui.h"
#include "zrythm.h"
#include "zrythm_app.h"

#include "gtk_wrapper.h"

G_DEFINE_TYPE (
  ArrangerMinimapBgWidget,
  arranger_minimap_bg_widget,
  GTK_TYPE_WIDGET)

static void
draw_timeline (GtkWidget * widget, GtkSnapshot * snapshot)
{
  int width = gtk_widget_get_width (widget);
  int height = gtk_widget_get_height (widget);

  auto start = P_MARKER_TRACK->get_start_marker ();
  auto end = (P_MARKER_TRACK->get_end_marker ());
  int  song_px =
    ui_pos_to_px_timeline (end->pos_, 1)
    - ui_pos_to_px_timeline (start->pos_, 1);
  /*int region_px;*/

  int total_track_height = 0;
  for (auto &track : TRACKLIST->tracks_)
    {
      if (track->widget_ && track->visible_)
        total_track_height +=
          gtk_widget_get_height (GTK_WIDGET (track->widget_));
    }
  int track_height;
  for (auto &track : TRACKLIST->tracks_)
    {
      if (!track->widget_ || !track->visible_)
        continue;

      graphene_point_t wpt;
      graphene_point_t tmp_pt = Z_GRAPHENE_POINT_INIT (0, 0);
      bool             success = gtk_widget_compute_point (
        GTK_WIDGET (track->widget_), GTK_WIDGET (MW_TRACKLIST->unpinned_box),
        &tmp_pt, &wpt);
      z_return_if_fail (success);
      track_height = gtk_widget_get_height (GTK_WIDGET (track->widget_));

      if (track->has_lanes ())
        {
          auto laned_track = dynamic_cast<LanedTrack *> (track.get ());
          auto variant = convert_to_variant<LanedTrackPtrVariant> (laned_track);
          std::visit (
            [&] (const auto &t) {
              GdkRGBA color = t->color_.to_gdk_rgba ();
              color.alpha = 0.6f;

              for (auto &lane : t->lanes_)
                {
                  for (auto &region : lane->regions_)
                    {
                      int px_start = ui_pos_to_px_timeline (region->pos_, true);
                      int px_end =
                        ui_pos_to_px_timeline (region->end_pos_, true);
                      int px_length = px_end - px_start;

                      {
                        graphene_rect_t tmp_r = Z_GRAPHENE_RECT_INIT (
                          ((float) px_start / (float) song_px) * (float) width,
                          ((float) wpt.y / (float) total_track_height)
                            * (float) height,
                          ((float) px_length / (float) song_px) * (float) width,
                          ((float) track_height / (float) total_track_height)
                            * (float) height);
                        gtk_snapshot_append_color (snapshot, &color, &tmp_r);
                      }
                    }
                }
            },
            variant);
        }
    }
}

static void
arranger_minimap_bg_snapshot (GtkWidget * widget, GtkSnapshot * snapshot)
{
  if (!PROJECT->loaded_)
    return;

  ArrangerMinimapBgWidget * self = Z_ARRANGER_MINIMAP_BG_WIDGET (widget);

  if (self->owner->type == ArrangerMinimapType::ARRANGER_MINIMAP_TYPE_TIMELINE)
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
  ArrangerMinimapBgWidget * self = static_cast<ArrangerMinimapBgWidget *> (
    g_object_new (ARRANGER_MINIMAP_BG_WIDGET_TYPE, nullptr));

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
    GTK_WIDGET (self), arranger_minimap_bg_tick_cb, self, nullptr);
}
