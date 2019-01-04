/*
 * gui/widgets/timeline_bg.c - Timeline background
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

#include "zrythm.h"
#include "project.h"
#include "settings.h"
#include "audio/automation_track.h"
#include "audio/automation_tracklist.h"
#include "audio/channel.h"
#include "audio/instrument_track.h"
#include "audio/mixer.h"
#include "audio/track.h"
#include "audio/tracklist.h"
#include "audio/transport.h"
#include "gui/widgets/arranger.h"
#include "gui/widgets/automation_point.h"
#include "gui/widgets/automation_track.h"
#include "gui/widgets/center_dock.h"
#include "gui/widgets/main_window.h"
#include "gui/widgets/ruler.h"
#include "gui/widgets/timeline_arranger.h"
#include "gui/widgets/timeline_bg.h"
#include "gui/widgets/timeline_ruler.h"
#include "gui/widgets/track.h"
#include "gui/widgets/tracklist.h"

#include <gtk/gtk.h>

G_DEFINE_TYPE (TimelineBgWidget, timeline_bg_widget, GTK_TYPE_DRAWING_AREA)


static void
draw_horizontal_line (cairo_t * cr,
                      int       y_offset,
                      double    alpha)
{
  cairo_set_source_rgba (cr, 0.7, 0.7, 0.7, alpha);
  cairo_set_line_width (cr, 0.5);
  cairo_move_to (cr, 0, y_offset);
  RULER_WIDGET_GET_PRIVATE (MW_RULER);
  cairo_line_to (cr, prv->total_px, y_offset);
  cairo_stroke (cr);
}

static gboolean
draw_cb (GtkWidget *widget, cairo_t *cr, gpointer data)
{
  if (!MAIN_WINDOW || !MW_RULER || !MW_TIMELINE)
    return FALSE;

  GtkStyleContext *context;

  context = gtk_widget_get_style_context (widget);

  guint height = gtk_widget_get_allocated_height (widget);

  /* get positions in px */
  static int playhead_pos_in_px;
  RULER_WIDGET_GET_PRIVATE (MW_RULER);
  playhead_pos_in_px =
    (TRANSPORT->playhead_pos.bars - 1) * prv->px_per_bar +
    (TRANSPORT->playhead_pos.beats - 1) * prv->px_per_beat +
    (TRANSPORT->playhead_pos.quarter_beats - 1) * prv->px_per_quarter_beat +
    TRANSPORT->playhead_pos.ticks * prv->px_per_tick;

  gtk_render_background (context, cr, 0, 0, prv->total_px, height);

  /* handle vertical drawing */
  for (int i = SPACE_BEFORE_START; i < prv->total_px; i++)
  {
    int actual_pos = i - SPACE_BEFORE_START;
    if (actual_pos == playhead_pos_in_px)
      {
          cairo_set_source_rgb (cr, 1, 0, 0);
          cairo_set_line_width (cr, 2);
          cairo_move_to (cr, i, 0);
          cairo_line_to (cr, i, height);
          cairo_stroke (cr);
      }
      if (actual_pos % prv->px_per_bar == 0)
      {
          cairo_set_source_rgb (cr, 0.3, 0.3, 0.3);
          cairo_set_line_width (cr, 1);
          cairo_move_to (cr, i, 0);
          cairo_line_to (cr, i, height);
          cairo_stroke (cr);
      }
      else if (actual_pos % prv->px_per_beat == 0)
      {
          cairo_set_source_rgb (cr, 0.25, 0.25, 0.25);
          cairo_set_line_width (cr, 0.5);
          cairo_move_to (cr, i, 0);
          cairo_line_to (cr, i, height);
          cairo_stroke (cr);
      }
  }

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
      draw_horizontal_line (cr,
                            line_y,
                            1.0);
    }

  /* draw automation related stuff */
  for (int i = 0; i < MIXER->num_channels; i++)
    {
      Track * track = MIXER->channels[i]->track;

      AutomationTracklist * automation_tracklist =
        track_get_automation_tracklist (track);
      if (automation_tracklist)
        {
          for (int j = 0;
               j < automation_tracklist->num_automation_tracks;
               j++)
            {
              AutomationTrack * at = automation_tracklist->automation_tracks[j];

              if (at->widget)
                {
                  /* horizontal automation track lines */
                  gint wx, wy;
                  gtk_widget_translate_coordinates(
                            GTK_WIDGET (at->widget),
                            GTK_WIDGET (MW_TRACKLIST),
                            0,
                            0,
                            &wx,
                            &wy);
                  draw_horizontal_line (cr, wy, 0.2);

                }
            }
        }
    }

  /* draw selections */
  arranger_bg_draw_selections (ARRANGER_WIDGET (MW_TIMELINE),
                               cr);

  return 0;
}

static gboolean
multipress_pressed (GtkGestureMultiPress *gesture,
               gint                  n_press,
               gdouble               x,
               gdouble               y,
               gpointer              user_data)
{
  RulerWidget * self = (RulerWidget *) user_data;
  gtk_widget_grab_focus (GTK_WIDGET (self));
  return FALSE;
}

static void
drag_begin (GtkGestureDrag * gesture,
               gdouble         start_x,
               gdouble         start_y,
               gpointer        user_data)
{
  RULER_WIDGET_GET_PRIVATE (MW_RULER);
  prv->start_x = start_x;
}

static void
drag_update (GtkGestureDrag * gesture,
               gdouble         offset_x,
               gdouble         offset_y,
               gpointer        user_data)
{
}

static void
drag_end (GtkGestureDrag *gesture,
               gdouble         offset_x,
               gdouble         offset_y,
               gpointer        user_data)
{
  RULER_WIDGET_GET_PRIVATE (MW_RULER);
  prv->start_x = 0;
}

TimelineBgWidget *
timeline_bg_widget_new ()
{
  TimelineBgWidget * self = g_object_new (TIMELINE_BG_WIDGET_TYPE, NULL);

  gtk_widget_set_can_focus (GTK_WIDGET (self),
                           1);
  gtk_widget_set_focus_on_click (GTK_WIDGET (self),
                                 1);


  g_signal_connect (G_OBJECT (self), "draw",
                    G_CALLBACK (draw_cb), NULL);
  g_signal_connect (G_OBJECT(self->drag), "drag-begin",
                    G_CALLBACK (drag_begin),  self);
  g_signal_connect (G_OBJECT(self->drag), "drag-update",
                    G_CALLBACK (drag_update),  self);
  g_signal_connect (G_OBJECT(self->drag), "drag-end",
                    G_CALLBACK (drag_end),  self);
  g_signal_connect (G_OBJECT (self->multipress), "pressed",
                    G_CALLBACK (multipress_pressed), self);

  return self;
}

static void
timeline_bg_widget_class_init (TimelineBgWidgetClass * _klass)
{
  GtkWidgetClass * klass = GTK_WIDGET_CLASS (_klass);
  gtk_widget_class_set_css_name (klass,
                                 "arranger-bg");
}

static void
timeline_bg_widget_init (TimelineBgWidget *self )
{
  self->drag = GTK_GESTURE_DRAG (
                gtk_gesture_drag_new (GTK_WIDGET (self)));
  self->multipress = GTK_GESTURE_MULTI_PRESS (
                gtk_gesture_multi_press_new (GTK_WIDGET (self)));
}

