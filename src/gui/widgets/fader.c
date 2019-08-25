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

/** \file
 */

#include <stdlib.h>

#include "audio/channel.h"
#include "audio/fader.h"
#include "gui/widgets/bot_bar.h"
#include "gui/widgets/fader.h"
#include "utils/math.h"

#include <glib/gi18n.h>

G_DEFINE_TYPE (FaderWidget,
               fader_widget,
               GTK_TYPE_DRAWING_AREA)

static int
draw_cb (
  GtkWidget * widget,
  cairo_t * cr,
  void* data)
{
  guint width, height;
  GtkStyleContext *context;
  FaderWidget * self = (FaderWidget *) widget;
  context = gtk_widget_get_style_context (widget);

  width = gtk_widget_get_allocated_width (widget);
  height = gtk_widget_get_allocated_height (widget);

  gtk_render_background (
    context, cr, 0, 0, width, height);

  /* a custom shape that could be wrapped in a function */
  double x         = 0,        /* parameters like cairo_rectangle */
         y         = 0,
         aspect        = 1.0,     /* aspect ratio */
         corner_radius = height / 90.0;   /* and corner curvature radius */

  double radius = corner_radius / aspect;
  double degrees = G_PI / 180.0;
  /* value in pixels = total pixels * val
   * val is percentage */
  double fader_val = self->fader->fader_val;
  double value_px = height * fader_val;

  /* draw background bar */
  cairo_new_sub_path (cr);
  cairo_arc (cr, x + width - radius, y + radius, radius, -90 * degrees, 0 * degrees);
  cairo_line_to (cr, x + width, y + height - value_px);
  /*cairo_arc (cr, x + width - radius, y + height - radius, radius, 0 * degrees, 90 * degrees);*/
  cairo_line_to (cr, x, y + height - value_px);
  /*cairo_arc (cr, x + radius, y + height - radius, radius, 90 * degrees, 180 * degrees);*/
  cairo_arc (cr, x + radius, y + radius, radius, 180 * degrees, 270 * degrees);
  cairo_close_path (cr);

  cairo_set_source_rgba (cr, 0.4, 0.4, 0.4, 0.2);
  cairo_fill(cr);

  /* draw filled in bar */
  float intensity = fader_val;
  const float intensity_inv = 1.0 - intensity;
  float r = intensity_inv * self->end_color.red   +
            intensity * self->start_color.red;
  float g = intensity_inv * self->end_color.green +
            intensity * self->start_color.green;
  float b = intensity_inv * self->end_color.blue  +
            intensity * self->start_color.blue;
  float a = intensity_inv * self->end_color.alpha  +
            intensity * self->start_color.alpha;

  cairo_set_source_rgba (cr, r,g,b,a);
  cairo_new_sub_path (cr);
  cairo_line_to (cr, x + width, y + (height - value_px));
  /*cairo_arc (cr, x + width - radius, y + radius, radius, -90 * degrees, 0 * degrees);*/
  cairo_arc (cr, x + width - radius, y + height - radius, radius, 0 * degrees, 90 * degrees);
  cairo_arc (cr, x + radius, y + height - radius, radius, 90 * degrees, 180 * degrees);
  cairo_line_to (cr, x, y + (height - value_px));
  /*cairo_arc (cr, x + radius, y + radius, radius, 180 * degrees, 270 * degrees);*/
  cairo_close_path (cr);

  cairo_fill(cr);


  /* draw border line */
  cairo_set_source_rgba (cr, 0.2, 0.2, 0.2, 1);
  cairo_new_sub_path (cr);
  cairo_arc (cr, x + width - radius, y + radius, radius, -90 * degrees, 0 * degrees);
  cairo_arc (cr, x + width - radius, y + height - radius, radius, 0 * degrees, 90 * degrees);
  cairo_arc (cr, x + radius, y + height - radius, radius, 90 * degrees, 180 * degrees);
  cairo_arc (cr, x + radius, y + radius, radius, 180 * degrees, 270 * degrees);
  cairo_close_path (cr);

  /*cairo_set_source_rgba (cr, 0.4, 0.2, 0.05, 0.2);*/
  /*cairo_fill_preserve (cr);*/
  cairo_set_line_width (cr, 1.7);
  cairo_stroke(cr);

  /* draw fader line */
  cairo_set_source_rgba (cr, 0, 0, 0, 1);
  cairo_set_line_width (cr, 16.0);
  cairo_set_line_cap  (cr, CAIRO_LINE_CAP_SQUARE);
  cairo_move_to (cr, x, y + (height - value_px));
  cairo_line_to (cr, x+ width, y + (height - value_px));
  cairo_stroke (cr);
  /*cairo_set_source_rgba (cr, 0.4, 0.1, 0.05, 1);*/
  GdkRGBA color;
  gdk_rgba_parse (&color,
                  "#9D3955");
  cairo_set_source_rgba (cr,
                         color.red,
                         color.green,
                         color.blue,
                         1);
  cairo_set_line_width (cr, 3.0);
  cairo_move_to (cr, x, y + (height - value_px));
  cairo_line_to (cr, x+ width, y + (height - value_px));
  cairo_stroke (cr);
	return 0;
}


static gboolean
on_motion (GtkWidget * widget, GdkEvent *event)
{
  FaderWidget * self = Z_FADER_WIDGET (widget);

  if (gdk_event_get_event_type (event) ==
        GDK_ENTER_NOTIFY)
    {
      gtk_widget_set_state_flags (
        GTK_WIDGET (self),
        GTK_STATE_FLAG_PRELIGHT, 0);
      bot_bar_change_status (
        _("Fader - Click and drag to change value"));
    }
  else if (gdk_event_get_event_type (event) ==
             GDK_LEAVE_NOTIFY)
    {
      gtk_widget_unset_state_flags (
        GTK_WIDGET (self),
        GTK_STATE_FLAG_PRELIGHT);
      bot_bar_change_status ("");
    }

  return FALSE;
}

static void
drag_begin (GtkGestureDrag *gesture,
            double           start_x,
            double           start_y,
            gpointer        user_data)
{
  FaderWidget * self = (FaderWidget *) user_data;

  GdkEventSequence *sequence =
    gtk_gesture_single_get_current_sequence (
      GTK_GESTURE_SINGLE (gesture));
  const GdkEvent * event =
    gtk_gesture_get_last_event (
      GTK_GESTURE (gesture), sequence);
  GdkModifierType state_mask;
  gdk_event_get_state (event, &state_mask);
  if (state_mask & GDK_CONTROL_MASK)
    fader_set_amp ((void *) self->fader, 1.0);

  char * string =
    g_strdup_printf (
      "%.1f", self->fader->volume);
  gtk_label_set_text (
    self->tooltip_label, string);
  g_free (string);
  gtk_window_present (self->tooltip_win);
}

static void
drag_update (GtkGestureDrag * gesture,
               gdouble         offset_x,
               gdouble         offset_y,
               gpointer        user_data)
{
  FaderWidget * self = (FaderWidget *) user_data;
  offset_y = - offset_y;
  /*int use_y = abs(offset_y - self->last_y) > abs(offset_x - self->last_x);*/
  int use_y = 1;
  /*double multiplier = 0.005;*/
  double diff =
    use_y ? offset_y - self->last_y :
    offset_x - self->last_x;
  double height =
    gtk_widget_get_allocated_height (
      GTK_WIDGET (self));
  double adjusted_diff = diff / height;
  double new_fader_val =
    CLAMP (self->fader->fader_val + adjusted_diff,
           0.0,
           1.0);
  fader_set_fader_val (self->fader, new_fader_val);
  self->last_x = offset_x;
  self->last_y = offset_y;
  gtk_widget_queue_draw (GTK_WIDGET (self));

  char * string =
    g_strdup_printf (
      "%.1f", self->fader->volume);
  gtk_label_set_text (
    self->tooltip_label, string);
  g_free (string);
  gtk_window_present (self->tooltip_win);
}

static void
drag_end (GtkGestureDrag *gesture,
               gdouble         offset_x,
               gdouble         offset_y,
               gpointer        user_data)
{
  FaderWidget * self = (FaderWidget *) user_data;
  self->last_x = 0;
  self->last_y = 0;
  gtk_widget_hide (GTK_WIDGET (self->tooltip_win));
}

void
on_reset_fader (GtkMenuItem *menuitem,
               FaderWidget * self)
{
  if (self->fader->type == FADER_TYPE_AUDIO_CHANNEL)
    channel_reset_fader (
      self->fader->channel);
  else
    fader_set_amp (
      self->fader, 1.0);
}

static void
show_context_menu (
  FaderWidget * self)
{
  GtkWidget *menu, *menuitem;

  menu = gtk_menu_new();

  menuitem = gtk_menu_item_new_with_label("Reset");

  g_signal_connect (
    menuitem, "activate",
    G_CALLBACK (on_reset_fader), self);

  gtk_menu_shell_append (
    GTK_MENU_SHELL(menu), menuitem);

  gtk_widget_show_all(menu);

  gtk_menu_popup_at_pointer (GTK_MENU(menu), NULL);

}

static void
on_right_click (GtkGestureMultiPress *gesture,
               gint                  n_press,
               gdouble               x,
               gdouble               y,
               FaderWidget *         self)
{
  if (n_press == 1)
    {
      show_context_menu (self);
    }
}

/**
 * Creates a new Fader widget and binds it to the
 * given Fader.
 */
void
fader_widget_setup (
  FaderWidget * self,
  Fader *       fader,
  int width)
{
  self->fader = fader;

  /* set size */
  gtk_widget_set_size_request (
    GTK_WIDGET (self), width, -1);
}

static void
fader_widget_init (FaderWidget * self)
{
  gdk_rgba_parse (&self->start_color, "#D68A0C");
  gdk_rgba_parse (&self->end_color, "#F9CA1B");

  /* make it able to notify */
  gtk_widget_set_has_window (
    GTK_WIDGET (self), TRUE);
  int crossing_mask =
    GDK_ENTER_NOTIFY_MASK | GDK_LEAVE_NOTIFY_MASK;
  gtk_widget_add_events (
    GTK_WIDGET (self), crossing_mask);

  self->drag = GTK_GESTURE_DRAG (
    gtk_gesture_drag_new (GTK_WIDGET (self)));

  self->tooltip_win =
    GTK_WINDOW (gtk_window_new (GTK_WINDOW_POPUP));
  gtk_window_set_type_hint (
    self->tooltip_win, GDK_WINDOW_TYPE_HINT_TOOLTIP);
  self->tooltip_label =
    GTK_LABEL (gtk_label_new ("label"));
  gtk_widget_set_visible (
    GTK_WIDGET (self->tooltip_label), 1);
  gtk_container_add (
    GTK_CONTAINER (self->tooltip_win),
    GTK_WIDGET (self->tooltip_label));
  gtk_window_set_position (
    self->tooltip_win, GTK_WIN_POS_MOUSE);

  /* add right mouse multipress */
  GtkGestureMultiPress * right_mouse_mp =
    GTK_GESTURE_MULTI_PRESS (
      gtk_gesture_multi_press_new (
        GTK_WIDGET (self)));
  gtk_gesture_single_set_button (
    GTK_GESTURE_SINGLE (right_mouse_mp),
    GDK_BUTTON_SECONDARY);

  /* connect signals */
  g_signal_connect (
    G_OBJECT (self), "draw",
    G_CALLBACK (draw_cb), self);
  g_signal_connect (
    G_OBJECT (self), "enter-notify-event",
    G_CALLBACK (on_motion),  self);
  g_signal_connect (
    G_OBJECT(self), "leave-notify-event",
    G_CALLBACK (on_motion),  self);
  g_signal_connect (
    G_OBJECT(self->drag), "drag-begin",
    G_CALLBACK (drag_begin),  self);
  g_signal_connect (
    G_OBJECT(self->drag), "drag-update",
    G_CALLBACK (drag_update),  self);
  g_signal_connect (
    G_OBJECT(self->drag), "drag-end",
    G_CALLBACK (drag_end),  self);
  g_signal_connect (
    G_OBJECT (right_mouse_mp), "pressed",
    G_CALLBACK (on_right_click), self);

}

static void
fader_widget_class_init (FaderWidgetClass * _klass)
{
  GtkWidgetClass * klass = GTK_WIDGET_CLASS (_klass);
  gtk_widget_class_set_css_name (
    klass, "fader");
}
