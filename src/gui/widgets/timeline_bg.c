/*
 * Copyright (C) 2018-2019 Alexandros Theodotou <alex at zrythm dot org>
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

/**
 * \file
 *
 * Timeline background inheriting from arranger_bg.
 */

#include "zrythm.h"
#include "audio/automation_lane.h"
#include "audio/automation_track.h"
#include "audio/automation_tracklist.h"
#include "audio/channel.h"
#include "audio/instrument_track.h"
#include "audio/mixer.h"
#include "audio/track.h"
#include "audio/tracklist.h"
#include "audio/transport.h"
#include "gui/widgets/arranger.h"
#include "gui/widgets/automation_lane.h"
#include "gui/widgets/automation_point.h"
#include "gui/widgets/center_dock.h"
#include "gui/widgets/main_window.h"
#include "gui/widgets/ruler.h"
#include "gui/widgets/timeline_arranger.h"
#include "gui/widgets/timeline_bg.h"
#include "gui/widgets/timeline_ruler.h"
#include "gui/widgets/track.h"
#include "gui/widgets/tracklist.h"
#include "project.h"
#include "settings/settings.h"
#include "utils/cairo.h"

#include <gtk/gtk.h>

G_DEFINE_TYPE (TimelineBgWidget,
               timeline_bg_widget,
               ARRANGER_BG_WIDGET_TYPE)

static gboolean
timeline_bg_draw_cb (
  GtkWidget *widget,
  cairo_t *cr,
  gpointer data)
{
  GdkRectangle rect;
  gdk_cairo_get_clip_rectangle (cr,
                                &rect);

  /* handle horizontal drawing for tracks */
  GtkWidget * tw_widget;
  gint wx, wy;
  for (int i = 0; i < TRACKLIST->num_tracks; i++)
    {
      Track * track = TRACKLIST->tracks[i];
      if (!track->visible)
        continue;

      /* draw line below widget */
      TrackWidget * tw = track->widget;
      tw_widget = GTK_WIDGET (tw);
      if (!GTK_IS_WIDGET (tw_widget))
        continue;

      gtk_widget_translate_coordinates(
                tw_widget,
                GTK_WIDGET (MW_TRACKLIST),
                0,
                0,
                &wx,
                &wy);
      int line_y =
        wy + gtk_widget_get_allocated_height (
          GTK_WIDGET (tw_widget));
      if (line_y > rect.y &&
          line_y < (rect.y + rect.height))
        z_cairo_draw_horizontal_line (cr,
                                      line_y,
                                      rect.x,
                                      rect.x + rect.width,
                                      1.0);
    }

  /* draw automation related stuff */
  for (int i = 0; i < TRACKLIST->num_tracks; i++)
    {
      Track * track = TRACKLIST->tracks[i];

      AutomationTracklist * automation_tracklist =
        track_get_automation_tracklist (track);
      if (automation_tracklist)
        {
          for (int j = 0;
               j < automation_tracklist->
                 num_automation_lanes;
               j++)
            {
              AutomationLane * al =
                automation_tracklist->
                  automation_lanes[j];

              if (al->widget &&
                  track->bot_paned_visible &&
                  al->visible)
                {
                  /* horizontal automation track lines */
                  gint wx, wy;
                  gtk_widget_translate_coordinates(
                            GTK_WIDGET (al->widget),
                            GTK_WIDGET (MW_TRACKLIST),
                            0,
                            0,
                            &wx,
                            &wy);
                  if (wy > rect.y &&
                      wy < (rect.y + rect.height))
                    z_cairo_draw_horizontal_line (
                      cr,
                      wy,
                      rect.x,
                      rect.x + rect.width,
                      0.2);

                  float normalized_val =
                    automation_track_get_normalized_val_at_pos (
                      al->at,
                      &PLAYHEAD);
                  if (normalized_val < 0.f)
                    normalized_val =
                      automatable_real_val_to_normalized (
                        al->at->automatable,
                        automatable_get_val (
                          al->at->automatable));

                  int y_px =
                    automation_lane_widget_get_y_px_from_normalized_val (
                      al->widget,
                      normalized_val);

                  /* show line at current val */
                  cairo_set_source_rgba (
                    cr,
                    track->color.red,
                    track->color.green,
                    track->color.blue,
                    0.3);
                  cairo_set_line_width (cr, 1);
                  cairo_move_to (cr, rect.x, wy + y_px);
                  cairo_line_to (cr, rect.x + rect.width, wy + y_px);
                  cairo_stroke (cr);

                  /* show shade under the line */
                  /*cairo_set_source_rgba (*/
                    /*cr,*/
                    /*track->color.red,*/
                    /*track->color.green,*/
                    /*track->color.blue,*/
                    /*0.06);*/
                  /*int allocated_h =*/
                    /*gtk_widget_get_allocated_height (*/
                      /*GTK_WIDGET (al->widget));*/
                  /*cairo_rectangle (*/
                    /*cr,*/
                    /*rect.x, wy + y_px,*/
                    /*rect.width, allocated_h - y_px);*/
                  /*cairo_fill (cr);*/

                }
            }
        }
    }



  return 0;
}

static void
on_motion (TimelineBgWidget * self,
           GdkEventMotion *event)
{
  if (event->type == GDK_MOTION_NOTIFY)
    {
      Track * track =
        timeline_arranger_widget_get_track_at_y (
          event->y);

      if (P_TOOL == TOOL_SELECT_NORMAL ||
          P_TOOL == TOOL_SELECT_STRETCH)
        {
          if (track)
            {
              if (track_widget_is_cursor_in_top_half (
                    track->widget,
                    event->y))
                {
                  /* set cursor to normal */
                  ui_set_cursor_from_icon_name (
                    GTK_WIDGET (self),
                    "edit-select",
                    3, 6);
                }
              else
                {
                  /* set cursor to range selection */
                  ui_set_cursor_from_name (
                    GTK_WIDGET (self),
                    "text");
                }
            }
          else
            {
              /* set cursor to normal */
              ui_set_cursor_from_icon_name (
                GTK_WIDGET (self),
                "edit-select",
                3, 6);
            }
        }
    }
}

TimelineBgWidget *
timeline_bg_widget_new (RulerWidget *    ruler,
                        ArrangerWidget * arranger)
{
  TimelineBgWidget * self =
    g_object_new (TIMELINE_BG_WIDGET_TYPE,
                  NULL);

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
timeline_bg_widget_init (TimelineBgWidget *self )
{
  gtk_widget_add_events (GTK_WIDGET (self),
                         GDK_ALL_EVENTS_MASK);

  g_signal_connect (
    G_OBJECT (self), "draw",
    G_CALLBACK (timeline_bg_draw_cb), NULL);
  g_signal_connect (
    G_OBJECT(self), "motion-notify-event",
    G_CALLBACK (on_motion),  self);
}
