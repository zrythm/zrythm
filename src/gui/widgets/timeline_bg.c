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
 * Timeline background inheriting from arranger_bg.
 */

#include "dsp/automation_track.h"
#include "dsp/automation_tracklist.h"
#include "dsp/channel.h"
#include "dsp/instrument_track.h"
#include "dsp/router.h"
#include "dsp/track.h"
#include "dsp/tracklist.h"
#include "dsp/transport.h"
#include "gui/widgets/arranger.h"
#include "gui/widgets/automation_point.h"
#include "gui/widgets/center_dock.h"
#include "gui/widgets/main_window.h"
#include "gui/widgets/ruler.h"
#include "gui/widgets/timeline_arranger.h"
#include "gui/widgets/timeline_bg.h"
#include "gui/widgets/timeline_panel.h"
#include "gui/widgets/timeline_ruler.h"
#include "gui/widgets/track.h"
#include "gui/widgets/tracklist.h"
#include "project.h"
#include "settings/settings.h"
#include "utils/cairo.h"
#include "zrythm.h"

#include <gtk/gtk.h>

G_DEFINE_TYPE (
  TimelineBgWidget,
  timeline_bg_widget,
  ARRANGER_BG_WIDGET_TYPE)

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
    arranger_bg_widget_get_private (
      Z_ARRANGER_BG_WIDGET (self));

  /* handle horizontal drawing for tracks */
  GtkWidget *   tw_widget;
  gint          track_start_offset;
  Track *       track;
  TrackWidget * tw;
  int           line_y, i, j;
  int           is_unpinned_timeline =
    prv->arranger == (ArrangerWidget *) (MW_TIMELINE);
  int is_pinned_timeline =
    prv->arranger == (ArrangerWidget *) (MW_PINNED_TIMELINE);
  for (i = 0; i < TRACKLIST->num_tracks; i++)
    {
      track = TRACKLIST->tracks[i];

      /* skip tracks in the other timeline (pinned/
       * non-pinned) */
      if (
        !track->visible
        || (is_unpinned_timeline && track->pinned)
        || (is_pinned_timeline && !track->pinned))
        continue;

      /* draw line below widget */
      tw = track->widget;
      if (!GTK_IS_WIDGET (tw))
        continue;
      tw_widget = (GtkWidget *) tw;

      int full_track_height =
        track_get_full_visible_height (track);

      gtk_widget_translate_coordinates (
        tw_widget,
        GTK_WIDGET (
          is_pinned_timeline
            ? MW_TRACKLIST->pinned_box
            : MW_TRACKLIST->unpinned_box),
        0, 0, NULL, &track_start_offset);

      line_y = track_start_offset + full_track_height;

      if (line_y >= rect->y && line_y < rect->y + rect->height)
        {
          z_cairo_draw_horizontal_line (
            cr, line_y - rect->y, 0, rect->width, 1.0);
        }

      int total_height = track->main_height;

#define OFFSET_PLUS_TOTAL_HEIGHT \
  (track_start_offset + total_height)

      /* --- draw lanes --- */

      if (track->lanes_visible)
        {
          for (j = 0; j < track->num_lanes; j++)
            {
              TrackLane * lane = track->lanes[j];

              /* horizontal line above lane */
              if (
                OFFSET_PLUS_TOTAL_HEIGHT > rect->y
                && OFFSET_PLUS_TOTAL_HEIGHT
                     < rect->y + rect->height)
                {
                  z_cairo_draw_horizontal_line (
                    cr, OFFSET_PLUS_TOTAL_HEIGHT - rect->y, 0,
                    rect->width, 0.4);
                }

              total_height += lane->height;
            }
        }

      /* --- draw automation related stuff --- */

      /* skip tracks without visible automation */
      if (!track->automation_visible)
        continue;

      AutomationTracklist * atl =
        track_get_automation_tracklist (track);
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
                && OFFSET_PLUS_TOTAL_HEIGHT
                     < rect->y + rect->height)
                {
                  z_cairo_draw_horizontal_line (
                    cr, OFFSET_PLUS_TOTAL_HEIGHT - rect->y, 0,
                    rect->width, 0.2);
                }

              float normalized_val =
                automation_track_get_normalized_val_at_pos (
                  at, PLAYHEAD);
              if (normalized_val < 0.f)
                normalized_val =
                  automatable_real_val_to_normalized (
                    at->automatable,
                    automatable_get_val (at->automatable));

              int y_px =
                automation_track_get_y_px_from_normalized_val (
                  at, normalized_val);

              /* line at current val */
              cairo_set_source_rgba (
                cr, track->color.red, track->color.green,
                track->color.blue, 0.3);
              cairo_set_line_width (cr, 1);
              cairo_move_to (
                cr, 0,
                (OFFSET_PLUS_TOTAL_HEIGHT + y_px) - rect->y);
              cairo_line_to (
                cr, rect->width,
                (OFFSET_PLUS_TOTAL_HEIGHT + y_px) - rect->y);
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
timeline_bg_widget_new (
  RulerWidget *    ruler,
  ArrangerWidget * arranger)
{
  TimelineBgWidget * self =
    g_object_new (TIMELINE_BG_WIDGET_TYPE, NULL);

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
  gtk_widget_add_events (
    GTK_WIDGET (self), GDK_ALL_EVENTS_MASK);
}
