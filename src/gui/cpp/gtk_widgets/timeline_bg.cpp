/*
 * SPDX-FileCopyrightText: © 2018-2019 Alexandros Theodotou <alex@zrythm.org>
 *
 * SPDX-License-Identifier: LicenseRef-ZrythmLicense
 */

/**
 * @file
 *
 * Timeline background inheriting from arranger_bg.
 */

#include "common/dsp/automation_track.h"
#include "common/dsp/automation_tracklist.h"
#include "common/dsp/channel.h"
#include "common/dsp/instrument_track.h"
#include "common/dsp/router.h"
#include "common/dsp/track.h"
#include "common/dsp/tracklist.h"
#include "common/dsp/transport.h"
#include "common/utils/cairo.h"
#include "gui/cpp/backend/project.h"
#include "gui/cpp/backend/settings/g_settings_manager.h"
#include "gui/cpp/backend/settings/settings.h"
#include "gui/cpp/backend/zrythm.h"
#include "gui/cpp/gtk_widgets/arranger.h"
#include "gui/cpp/gtk_widgets/automation_point.h"
#include "gui/cpp/gtk_widgets/center_dock.h"
#include "gui/cpp/gtk_widgets/gtk_wrapper.h"
#include "gui/cpp/gtk_widgets/main_window.h"
#include "gui/cpp/gtk_widgets/ruler.h"
#include "gui/cpp/gtk_widgets/timeline_arranger.h"
#include "gui/cpp/gtk_widgets/timeline_bg.h"
#include "gui/cpp/gtk_widgets/timeline_panel.h"
#include "gui/cpp/gtk_widgets/timeline_ruler.h"
#include "gui/cpp/gtk_widgets/track.h"
#include "gui/cpp/gtk_widgets/tracklist.h"

G_DEFINE_TYPE (TimelineBgWidget, timeline_bg_widget, ARRANGER_BG_WIDGET_TYPE)

/**
 * To be called by the arranger bg during drawing.
 */
void
timeline_bg_widget_draw (
  TimelineBgWidget * self,
  cairo_t *          cr,
  GdkRectangle *     rect)
{
  ArrangerBgWidgetPrivate * prv =
    arranger_bg_widget_get_private (Z_ARRANGER_BG_WIDGET (self));

  /* handle horizontal drawing for tracks */
  GtkWidget *   tw_widget;
  gint          track_start_offset;
  TrackWidget * tw;
  int           line_y, i, j;
  int is_unpinned_timeline = prv->arranger == (ArrangerWidget *) (MW_TIMELINE);
  int is_pinned_timeline =
    prv->arranger == (ArrangerWidget *) (MW_PINNED_TIMELINE);
  for (auto track : TRACKLIST->tracks)
    {
      /* skip tracks in the other timeline (pinned/
       * non-pinned) */
      if (
        !track->visible || (is_unpinned_timeline && track->pinned)
        || (is_pinned_timeline && !track->pinned))
        continue;

      /* draw line below widget */
      tw = track->widget;
      if (!GTK_IS_WIDGET (tw))
        continue;
      tw_widget = (GtkWidget *) tw;

      int full_track_height = track_get_full_visible_height (track);

      gtk_widget_compute_point (
        tw_widget,
        GTK_WIDGET (
          is_pinned_timeline
            ? MW_TRACKLIST->pinned_box
            : MW_TRACKLIST->unpinned_box),
        0, 0, nullptr, &track_start_offset);

      line_y = track_start_offset + full_track_height;

      if (line_y >= rect->y && line_y < rect->y + rect->height)
        {
          z_cairo_draw_horizontal_line (
            cr, line_y - rect->y, 0, rect->width, 1.0);
        }

      int total_height = track->main_height;

#define OFFSET_PLUS_TOTAL_HEIGHT (track_start_offset + total_height)

      /* --- draw lanes --- */

      if (track->lanes_visible)
        {
          for (j = 0; j < track->num_lanes; j++)
            {
              TrackLane * lane = track->lanes[j];

              /* horizontal line above lane */
              if (
                OFFSET_PLUS_TOTAL_HEIGHT > rect->y
                && OFFSET_PLUS_TOTAL_HEIGHT < rect->y + rect->height)
                {
                  z_cairo_draw_horizontal_line (
                    cr, OFFSET_PLUS_TOTAL_HEIGHT - rect->y, 0, rect->width, 0.4);
                }

              total_height += lane->height;
            }
        }

      /* --- draw automation related stuff --- */

      /* skip tracks without visible automation */
      if (!track->automation_visible)
        continue;

      AutomationTracklist * atl = track_get_automation_tracklist (track);
      if (atl)
        {
          AutomationTrack * at;
          for (j = 0; j < atl->num_ats; j++)
            {
              at = atl->ats[j];

              if (!at->created || !at->visible)
                continue;

              /* horizontal line above automation
               * track */
              if (
                OFFSET_PLUS_TOTAL_HEIGHT > rect->y
                && OFFSET_PLUS_TOTAL_HEIGHT < rect->y + rect->height)
                {
                  z_cairo_draw_horizontal_line (
                    cr, OFFSET_PLUS_TOTAL_HEIGHT - rect->y, 0, rect->width, 0.2);
                }

              float normalized_val =
                automation_track_get_normalized_val_at_pos (at, PLAYHEAD);
              if (normalized_val < 0.f)
                normalized_val = automatable_real_val_to_normalized (
                  at->automatable, automatable_get_val (at->automatable));

              int y_px = automation_track_get_y_px_from_normalized_val (
                at, normalized_val);

              /* line at current val */
              cairo_set_source_rgba (
                cr, track->color.red, track->color.green, track->color.blue,
                0.3);
              cairo_set_line_width (cr, 1);
              cairo_move_to (cr, 0, (OFFSET_PLUS_TOTAL_HEIGHT + y_px) - rect->y);
              cairo_line_to (
                cr, rect->width, (OFFSET_PLUS_TOTAL_HEIGHT + y_px) - rect->y);
              cairo_stroke (cr);

              /* show shade under the line */
              /*cairo_set_source_rgba (*/
              /*cr,*/
              /*track->color.red,*/
              /*track->color.green,*/
              /*track->color.blue,*/
              /*0.06);*/
              /*cairo_rectangle (*/
              /*cr,*/
              /*0,*/
              /*(OFFSET_PLUS_TOTAL_HEIGHT + y_px) -*/
              /*rect->y,*/
              /*rect->width,*/
              /*at->height - y_px);*/
              /*cairo_fill (cr);*/

              total_height += at->height;
            }
        }
    }
}

/*static gboolean*/
/*on_motion (TimelineBgWidget * self,*/
/*GdkEventMotion *event)*/
/*{*/
/*return FALSE;*/
/*}*/

TimelineBgWidget *
timeline_bg_widget_new (RulerWidget * ruler, ArrangerWidget * arranger)
{
  TimelineBgWidget * self = static_cast<TimelineBgWidget *>(g_object_new (TIMELINE_BG_WIDGET_TYPE, nullptr);

  ARRANGER_BG_WIDGET_GET_PRIVATE (self);
  ab_prv->ruler = ruler;
  ab_prv->arranger = arranger;

  return self;
}

static void
timeline_bg_widget_class_init (TimelineBgWidgetClass * _klass)
{
}

static void
timeline_bg_widget_init (TimelineBgWidget * self)
{
  gtk_widget_add_events (GTK_WIDGET (self), GDK_ALL_EVENTS_MASK);
}
