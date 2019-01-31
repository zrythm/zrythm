/*
 * gui/widgets/ruler.c- The ruler on top of the timeline
 *
 * Copyright (C) 2019 Alexandros Theodotou
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

#include <math.h>

#include "actions/actions.h"
#include "audio/position.h"
#include "audio/transport.h"
#include "gui/widgets/arranger.h"
#include "gui/widgets/bot_dock_edge.h"
#include "gui/widgets/center_dock.h"
#include "gui/widgets/main_window.h"
#include "gui/widgets/midi_arranger.h"
#include "gui/widgets/midi_modifier_arranger.h"
#include "gui/widgets/midi_ruler.h"
#include "gui/widgets/ruler.h"
#include "gui/widgets/ruler_playhead.h"
#include "gui/widgets/timeline_arranger.h"
#include "gui/widgets/timeline_ruler.h"
#include "project.h"
#include "settings/settings.h"
#include "utils/ui.h"
#include "zrythm.h"

#include <gtk/gtk.h>

G_DEFINE_TYPE_WITH_PRIVATE (RulerWidget,
                            ruler_widget,
                            GTK_TYPE_OVERLAY)

#define Y_SPACING 5
#define FONT "Monospace"
#define FONT_SIZE 14

#define START_MARKER_TRIANGLE_HEIGHT 8
#define START_MARKER_TRIANGLE_WIDTH 8
#define Q_HEIGHT 12
#define Q_WIDTH 7

#define GET_PRIVATE RULER_WIDGET_GET_PRIVATE (self)

/**
 * Gets called to set the position/size of each overlayed widget.
 */
static gboolean
get_child_position (GtkOverlay   *overlay,
                    GtkWidget    *widget,
                    GdkRectangle *allocation,
                    gpointer      user_data)
{
  RulerWidget * self =
    Z_RULER_WIDGET (overlay);

  if (Z_IS_RULER_PLAYHEAD_WIDGET (widget))
    {
      allocation->x =
        ui_pos_to_px (
          &TRANSPORT->playhead_pos,
          1) - (PLAYHEAD_TRIANGLE_WIDTH / 2);
      allocation->y =
        gtk_widget_get_allocated_height (GTK_WIDGET (self)) -
          PLAYHEAD_TRIANGLE_HEIGHT;
      allocation->width = PLAYHEAD_TRIANGLE_WIDTH;
      allocation->height =
        PLAYHEAD_TRIANGLE_HEIGHT;
    }

  return TRUE;
}

RulerWidgetPrivate *
ruler_widget_get_private (RulerWidget * self)
{
  return ruler_widget_get_instance_private (self);
}

static gboolean
draw_cb (GtkWidget * widget,
         cairo_t *cr,
         RulerWidget * self)
{
  /* engine is run only set after everything is set up
   * so this is a good way to decide if we should draw
   * or not */
  if (!AUDIO_ENGINE->run)
    return FALSE;

  GdkRectangle rect;
  gdk_cairo_get_clip_rectangle (cr,
                                &rect);

  GET_PRIVATE;

  GtkStyleContext *context;

  context = gtk_widget_get_style_context (GTK_WIDGET (self));

  /*width = gtk_widget_get_allocated_width (widget);*/
  guint height =
    gtk_widget_get_allocated_height (widget);

  /* get positions in px */
  int cue_pos_in_px;
  cue_pos_in_px =
    ui_pos_to_px (
      &TRANSPORT->cue_pos,
      0);
  int start_marker_pos_px;
  start_marker_pos_px =
    ui_pos_to_px (
      &TRANSPORT->start_marker_pos,
      0);
  int end_marker_pos_px;
  end_marker_pos_px =
    ui_pos_to_px (
      &TRANSPORT->end_marker_pos,
      0);
  int loop_start_pos_px;
  loop_start_pos_px =
    ui_pos_to_px (
      &TRANSPORT->loop_start_pos,
      0);
  int loop_end_pos_px;
  loop_end_pos_px =
    ui_pos_to_px (
      &TRANSPORT->loop_end_pos,
      0);

  gtk_render_background (context, cr,
                         rect.x, rect.y,
                         rect.width, rect.height);

  /* draw lines */
  for (int i = rect.x; i < rect.x + rect.width; i++)
    {
      int draw_pos = i + SPACE_BEFORE_START;

      if (i % rw_prv->px_per_bar == 0)
      {
        cairo_set_source_rgb (cr, 1, 1, 1);
        cairo_set_line_width (cr, 1);
        cairo_move_to (cr, draw_pos, 0);
        cairo_line_to (cr, draw_pos, height / 3);
        cairo_stroke (cr);
        cairo_set_source_rgb (cr, 0.8, 0.8, 0.8);
        cairo_select_font_face(cr, FONT,
            CAIRO_FONT_SLANT_NORMAL,
            CAIRO_FONT_WEIGHT_NORMAL);
        cairo_set_font_size(cr, FONT_SIZE);
        gchar * label =
          g_strdup_printf ("%d", i / rw_prv->px_per_bar + 1);
        static cairo_text_extents_t extents;
        cairo_text_extents(cr, label, &extents);
        cairo_move_to (cr,
                       (draw_pos ) - extents.width / 2,
                       (height / 2) + Y_SPACING);
        cairo_show_text(cr, label);
      }
      else if (i % rw_prv->px_per_beat == 0)
      {
          cairo_set_source_rgb (cr, 0.7, 0.7, 0.7);
          cairo_set_line_width (cr, 0.5);
          cairo_move_to (cr, draw_pos, 0);
          cairo_line_to (cr, draw_pos, height / 4);
          cairo_stroke (cr);
      }
      else if (0)
        {

        }
    if (i == cue_pos_in_px)
      {
        cairo_set_source_rgb (cr, 0, 0.6, 0.9);
        cairo_set_line_width (cr, 2);
        cairo_move_to (
          cr,
          draw_pos,
          ((height - PLAYHEAD_TRIANGLE_HEIGHT) - Q_HEIGHT) - 1);
        cairo_line_to (
          cr,
          draw_pos + Q_WIDTH,
          ((height - PLAYHEAD_TRIANGLE_HEIGHT) - Q_HEIGHT / 2) - 1);
        cairo_line_to (
          cr,
          draw_pos,
          (height - PLAYHEAD_TRIANGLE_HEIGHT) - 1);
        cairo_fill (cr);
      }
    if (i == start_marker_pos_px)
      {
        cairo_set_source_rgb (cr, 1, 0, 0);
        cairo_set_line_width (cr, 2);
        cairo_move_to (cr, draw_pos, 0);
        cairo_line_to (cr, draw_pos + START_MARKER_TRIANGLE_WIDTH,
                       0);
        cairo_line_to (cr, draw_pos,
                       START_MARKER_TRIANGLE_HEIGHT);
        cairo_fill (cr);
      }
    if (i == end_marker_pos_px)
      {
        cairo_set_source_rgb (cr, 1, 0, 0);
        cairo_set_line_width (cr, 2);
        cairo_move_to (cr, draw_pos - START_MARKER_TRIANGLE_WIDTH, 0);
        cairo_line_to (cr, draw_pos, 0);
        cairo_line_to (cr, draw_pos, START_MARKER_TRIANGLE_HEIGHT);
        cairo_fill (cr);
      }
    if (i == loop_start_pos_px)
      {
        cairo_set_source_rgb (cr, 0, 0.9, 0.7);
        cairo_set_line_width (cr, 2);
        cairo_move_to (cr, draw_pos, START_MARKER_TRIANGLE_HEIGHT + 1);
        cairo_line_to (cr, draw_pos, START_MARKER_TRIANGLE_HEIGHT * 2 + 1);
        cairo_line_to (cr, draw_pos + START_MARKER_TRIANGLE_WIDTH,
                       START_MARKER_TRIANGLE_HEIGHT + 1);
        cairo_fill (cr);
      }
    if (i == loop_end_pos_px)
      {
        cairo_set_source_rgb (cr, 0, 0.9, 0.7);
        cairo_set_line_width (cr, 2);
        cairo_move_to (cr, draw_pos, START_MARKER_TRIANGLE_HEIGHT + 1);
        cairo_line_to (cr, draw_pos - START_MARKER_TRIANGLE_WIDTH,
                       START_MARKER_TRIANGLE_HEIGHT + 1);
        cairo_line_to (cr, draw_pos,
                       START_MARKER_TRIANGLE_HEIGHT * 2 + 1);
        cairo_fill (cr);
      }
  }

 return FALSE;
}

/**
 * TODO: for updating the global static variables
 * when needed
 */
void
reset_ruler ()
{

}

static gboolean
multipress_pressed (GtkGestureMultiPress *gesture,
               gint                  n_press,
               gdouble               x,
               gdouble               y,
               gpointer              user_data)
{
  if (n_press == 2)
    {
      Position pos;
      ui_px_to_pos (
        x,
        &pos,
        1);
      position_set_to_pos (&TRANSPORT->cue_pos,
                           &pos);

    }
  return FALSE;
}

static void
drag_begin (GtkGestureDrag * gesture,
               gdouble         start_x,
               gdouble         start_y,
               gpointer        user_data)
{
  RulerWidget * self = Z_RULER_WIDGET (user_data);
  GET_PRIVATE;

  rw_prv->start_x = start_x;
  Position pos;
  ui_px_to_pos (
    start_x,
    &pos,
    1);
  transport_move_playhead (&pos, 1);
}

static void
drag_update (GtkGestureDrag * gesture,
               gdouble         offset_x,
               gdouble         offset_y,
               gpointer        user_data)
{
  RulerWidget * self = Z_RULER_WIDGET (user_data);
  GET_PRIVATE;

  Position pos;
  ui_px_to_pos (
    rw_prv->start_x + offset_x,
    &pos,
    1);
  transport_move_playhead (&pos, 1);
}

static void
drag_end (GtkGestureDrag *gesture,
               gdouble         offset_x,
               gdouble         offset_y,
               gpointer        user_data)
{
  RulerWidget * self = (RulerWidget *) user_data;
  GET_PRIVATE;

  rw_prv->start_x = 0;
}

void
ruler_widget_refresh (RulerWidget * self)
{
  RULER_WIDGET_GET_PRIVATE (self);

  /*adjust for zoom level*/
  rw_prv->px_per_tick =
    DEFAULT_PX_PER_TICK * rw_prv->zoom_level;
  rw_prv->px_per_sixteenth =
    rw_prv->px_per_tick * TICKS_PER_SIXTEENTH_NOTE;
  rw_prv->px_per_beat =
    rw_prv->px_per_tick * TICKS_PER_BEAT;
  rw_prv->px_per_bar =
    rw_prv->px_per_beat * TRANSPORT->beats_per_bar;

  Position pos;
  position_set_to_bar (&pos,
                       TRANSPORT->total_bars + 1);
  rw_prv->total_px =
    rw_prv->px_per_tick * position_to_ticks (&pos);

  // set the size
  gtk_widget_set_size_request (
    GTK_WIDGET (self),
    rw_prv->total_px,
    -1);

  gtk_widget_queue_draw (
    GTK_WIDGET (self));
}

static void
ruler_widget_class_init (RulerWidgetClass * _klass)
{
  GtkWidgetClass * klass = GTK_WIDGET_CLASS (_klass);
  gtk_widget_class_set_css_name (klass,
                                 "ruler");
}

/**
 * Sets zoom level and disables/enables buttons
 * accordingly.
 *
 * Returns if the zoom level was set or not.
 */
int
ruler_widget_set_zoom_level (RulerWidget * self,
                             float         zoom_level)
{
  if (zoom_level > MAX_ZOOM_LEVEL)
    {
      action_disable_window_action ("zoom-in");
    }
  else
    {
      action_enable_window_action ("zoom-in");
    }
  if (zoom_level < MIN_ZOOM_LEVEL)
    {
      action_enable_window_action ("zoom-out");
    }
  else
    {
      action_enable_window_action ("zoom-out");
    }

  int update = zoom_level >= MIN_ZOOM_LEVEL &&
    zoom_level <= MAX_ZOOM_LEVEL;

  if (update)
    {
      RULER_WIDGET_GET_PRIVATE (self);
      rw_prv->zoom_level = zoom_level;
      ruler_widget_refresh (self);
      return 1;
    }
  else
    {
      return 0;
    }
}

static void
ruler_widget_init (RulerWidget * self)
{
  g_message ("initing ruler...");
  GET_PRIVATE;

  rw_prv->zoom_level = DEFAULT_ZOOM_LEVEL;

  rw_prv->bg =
    GTK_DRAWING_AREA (gtk_drawing_area_new ());
  gtk_widget_set_visible (GTK_WIDGET (rw_prv->bg),
                          1);
  gtk_container_add (GTK_CONTAINER (self),
                     GTK_WIDGET (rw_prv->bg));

  rw_prv->playhead =
    ruler_playhead_widget_new ();
  gtk_overlay_add_overlay (GTK_OVERLAY (self),
                           GTK_WIDGET (rw_prv->playhead));

  /* make it able to notify */
  gtk_widget_add_events (GTK_WIDGET (rw_prv->bg),
                         GDK_ALL_EVENTS_MASK);

  rw_prv->drag = GTK_GESTURE_DRAG (
    gtk_gesture_drag_new (GTK_WIDGET (rw_prv->bg)));
  rw_prv->multipress = GTK_GESTURE_MULTI_PRESS (
    gtk_gesture_multi_press_new (GTK_WIDGET (rw_prv->bg)));

  /* FIXME drags */
  g_signal_connect (G_OBJECT (rw_prv->bg), "draw",
                    G_CALLBACK (draw_cb), self);
  /*g_signal_connect (G_OBJECT(self), "button_press_event",*/
                    /*G_CALLBACK (button_press_cb),  self);*/
  g_signal_connect (G_OBJECT (self), "get-child-position",
                    G_CALLBACK (get_child_position), NULL);
  g_signal_connect (G_OBJECT(rw_prv->drag), "drag-begin",
                    G_CALLBACK (drag_begin),  self);
  g_signal_connect (G_OBJECT(rw_prv->drag), "drag-update",
                    G_CALLBACK (drag_update),  self);
  g_signal_connect (G_OBJECT(rw_prv->drag), "drag-end",
                    G_CALLBACK (drag_end),  self);
  g_signal_connect (G_OBJECT (rw_prv->multipress), "pressed",
                    G_CALLBACK (multipress_pressed), self);
}
