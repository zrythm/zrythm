/*
 * Copyright (C) 2018-2021 Alexandros Theodotou <alex at zrythm dot org>
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

#include <stdlib.h>

#include "actions/tracklist_selections.h"
#include "actions/midi_mapping_action.h"
#include "audio/channel.h"
#include "audio/fader.h"
#include "audio/midi_mapping.h"
#include "gui/backend/event_manager.h"
#include "gui/backend/event.h"
#include "gui/widgets/dialogs/bind_cc_dialog.h"
#include "gui/widgets/bot_bar.h"
#include "gui/widgets/fader.h"
#include "project.h"
#include "utils/flags.h"
#include "utils/gtk.h"
#include "utils/math.h"
#include "utils/resources.h"
#include "utils/ui.h"
#include "zrythm_app.h"

#include <glib/gi18n.h>

G_DEFINE_TYPE (
  FaderWidget, fader_widget, GTK_TYPE_DRAWING_AREA)

static void
fader_draw_cb (
  GtkDrawingArea * drawing_area,
  cairo_t *        cr,
  int              width,
  int              height,
  gpointer         user_data)
{
  FaderWidget * self = Z_FADER_WIDGET (user_data);
  GtkWidget * widget = GTK_WIDGET (drawing_area);

  GtkStyleContext *context =
  gtk_widget_get_style_context (widget);

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
  double fader_val =
    self->fader ? self->fader->fader_val : 1.f;
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
  double intensity = fader_val;
  const double intensity_inv = 1.0 - intensity;
  double r = intensity_inv * self->end_color.red   +
            intensity * self->start_color.red;
  double g = intensity_inv * self->end_color.green +
            intensity * self->start_color.green;
  double b = intensity_inv * self->end_color.blue  +
            intensity * self->start_color.blue;
  double a = intensity_inv * self->end_color.alpha  +
            intensity * self->start_color.alpha;

  cairo_set_source_rgba (cr, r,g,b,a);
  cairo_new_sub_path (cr);
  cairo_line_to (
    cr, x + width, y + (height - value_px));
  cairo_arc (
    cr, x + width - radius,
    y + height - radius, radius,
    0 * degrees, 90 * degrees);
  cairo_arc (
    cr, x + radius, y + height - radius,
    radius, 90 * degrees, 180 * degrees);
  cairo_line_to (cr, x, y + (height - value_px));
  cairo_close_path (cr);

  cairo_fill(cr);


  /* draw border line */
  cairo_set_source_rgba (cr, 0.2, 0.2, 0.2, 1);
  cairo_new_sub_path (cr);
  cairo_arc (
    cr, x + width - radius, y + radius,
    radius, -90 * degrees, 0 * degrees);
  cairo_arc (
    cr, x + width - radius,
    y + height - radius, radius,
    0 * degrees, 90 * degrees);
  cairo_arc (
    cr, x + radius, y + height - radius,
    radius, 90 * degrees, 180 * degrees);
  cairo_arc (
    cr, x + radius, y + radius,
    radius, 180 * degrees, 270 * degrees);
  cairo_close_path (cr);

  /*cairo_set_source_rgba (cr, 0.4, 0.2, 0.05, 0.2);*/
  /*cairo_fill_preserve (cr);*/
  cairo_set_line_width (cr, 1.7);
  cairo_stroke(cr);

  /* draw fader line */
  cairo_set_source_rgba (cr, 0, 0, 0, 1);
  cairo_set_line_width (cr, 12.0);
  cairo_set_line_cap  (cr, CAIRO_LINE_CAP_SQUARE);
  cairo_move_to (cr, x, y + (height - value_px));
  cairo_line_to (cr, x+ width, y + (height - value_px));
  cairo_stroke (cr);
  /*cairo_set_source_rgba (cr, 0.4, 0.1, 0.05, 1);*/

  if (self->hover || self->dragging)
    {
#if 0
      GdkRGBA color;
      gdk_rgba_parse (&color, "#9D3955");
      gdk_cairo_set_source_rgba (cr, &color);
#endif
      cairo_set_source_rgba (
        cr, 0.8, 0.8, 0.8, 1.0);
    }
  else
    {
      cairo_set_source_rgba (
        cr, 0.6, 0.6, 0.6, 1.0);
    }
  cairo_set_line_width (cr, 3.0);
  cairo_move_to (
    cr, x, y + (height - value_px));
  cairo_line_to (
    cr, x+ width, y + (height - value_px));
  cairo_stroke (cr);
}

static void
on_enter (
  GtkEventControllerMotion * motion_controller,
  gdouble                    x,
  gdouble                    y,
  gpointer                   user_data)
{
  FaderWidget * self = Z_FADER_WIDGET (user_data);
#if 0
  gtk_widget_set_state_flags (
    GTK_WIDGET (self),
    GTK_STATE_FLAG_PRELIGHT, 0);
#endif
  self->hover = true;
}

static void
on_leave (
  GtkEventControllerMotion * motion_controller,
  gpointer                   user_data)
{
  FaderWidget * self = Z_FADER_WIDGET (user_data);
#if 0
  gtk_widget_unset_state_flags (
    GTK_WIDGET (self),
    GTK_STATE_FLAG_PRELIGHT);
#endif
  self->hover = false;
}

static void
drag_begin (
  GtkGestureDrag * gesture,
  double           start_x,
  double           start_y,
  FaderWidget *    self)
{
  g_return_if_fail (IS_FADER (self->fader));

  GdkModifierType state =
    gtk_event_controller_get_current_event_state (
      GTK_EVENT_CONTROLLER (gesture));
  if (state & GDK_CONTROL_MASK)
    fader_set_amp ((void *) self->fader, 1.0);

  char * string =
    g_strdup_printf (
      "%.1f", (double) self->fader->volume);
  gtk_label_set_text (
    self->tooltip_label, string);
  g_free (string);
  gtk_window_present (self->tooltip_win);

  self->amp_at_start = fader_get_amp (self->fader);
  self->dragging = true;
}

static void
drag_update (
  GtkGestureDrag * gesture,
  gdouble         offset_x,
  gdouble         offset_y,
  gpointer        user_data)
{
  FaderWidget * self = (FaderWidget *) user_data;
  offset_y = - offset_y;

  g_return_if_fail (IS_FADER (self->fader));

  /*int use_y = abs(offset_y - self->last_y) > abs(offset_x - self->last_x);*/
  int use_y = 1;

  /*double multiplier = 0.005;*/
  double diff =
    use_y ?
      offset_y - self->last_y :
      offset_x - self->last_x;
  double height =
    gtk_widget_get_allocated_height (
      GTK_WIDGET (self));
  double adjusted_diff = diff / height;

  /* lower sensitivity if shift held */
  GdkModifierType mask;
  z_gtk_widget_get_mask (
    GTK_WIDGET (self), &mask);
  if (mask & GDK_SHIFT_MASK)
    {
      adjusted_diff *= 0.4;
    }

  double new_fader_val =
    CLAMP (
      (double) self->fader->fader_val +
        adjusted_diff,
      0.0, 1.0);
  fader_set_fader_val (
    self->fader, (float) new_fader_val);
  self->last_x = offset_x;
  self->last_y = offset_y;
  gtk_widget_queue_draw (GTK_WIDGET (self));

  char * string =
    g_strdup_printf (
      "%.1f", (double) self->fader->volume);
  gtk_label_set_text (
    self->tooltip_label, string);
  g_free (string);
  gtk_window_present (self->tooltip_win);
}

static void
drag_end (
  GtkGestureDrag *gesture,
  gdouble         offset_x,
  gdouble         offset_y,
  gpointer        user_data)
{
  FaderWidget * self = (FaderWidget *) user_data;
  self->last_x = 0;
  self->last_y = 0;
  gtk_widget_hide (GTK_WIDGET (self->tooltip_win));

  g_return_if_fail (IS_FADER (self->fader));

  float cur_amp = fader_get_amp (self->fader);
  fader_set_amp_with_action (
    self->fader, self->amp_at_start, cur_amp, true);

  self->dragging = false;
}

static void
show_context_menu (
  FaderWidget * self,
  double        x,
  double        y)
{
  GMenu * menu = g_menu_new ();
  GMenuItem * menuitem;

  char tmp[600];
  sprintf (
    tmp, "app.reset-fader::%p", self->fader);
  menuitem =
    z_gtk_create_menu_item (
      _("Reset"), NULL, tmp);
  g_menu_append_item (menu, menuitem);

  sprintf (
    tmp, "app.bind-midi-cc::%p", self->fader->amp);
  menuitem =
    CREATE_MIDI_LEARN_MENU_ITEM (tmp);
  g_menu_append_item (menu, menuitem);

  z_gtk_show_context_menu_from_g_menu (
    GTK_WIDGET (self), x, y, menu);
}

static void
on_right_click (
  GtkGestureClick * gesture,
  gint              n_press,
  gdouble           x,
  gdouble           y,
  FaderWidget *     self)
{
  if (n_press == 1)
    {
      show_context_menu (self, x, y);
    }
}

static gboolean
on_scroll (
  GtkEventControllerScroll * scroll_controller,
  gdouble                    dx,
  gdouble                    dy,
  gpointer                   user_data)
{
  FaderWidget * self = Z_FADER_WIDGET (user_data);

  GdkEvent * event =
    gtk_event_controller_get_current_event (
      GTK_EVENT_CONTROLLER (scroll_controller));
  GdkScrollDirection direction =
    gdk_scroll_event_get_direction (event);
  double abs_x, abs_y;
  gdk_event_get_position (
    event, &abs_x, &abs_y);

  GdkModifierType state =
    gtk_event_controller_get_current_event_state (
      GTK_EVENT_CONTROLLER (scroll_controller));

  /* add/substract *inc* amount */
  float inc = 0.04f;
  /* lower sensitivity if shift held */
  if (state & GDK_SHIFT_MASK)
    {
      inc = 0.01f;
    }
  int up_down =
    (direction == GDK_SCROLL_UP
     || direction == GDK_SCROLL_RIGHT) ? 1 : -1;
  float add_val = (float) up_down * inc;
  float current_val =
    fader_get_fader_val (self->fader);
  float new_val =
    CLAMP (current_val + add_val, 0.0f, 1.0f);
  fader_set_fader_val (self->fader, new_val);

  Channel * channel =
    fader_get_channel (self->fader);
  EVENTS_PUSH (
    ET_CHANNEL_FADER_VAL_CHANGED, channel);

  return true;
}

/**
 * Creates a new Fader widget and binds it to the
 * given Fader.
 */
void
fader_widget_setup (
  FaderWidget * self,
  Fader *       fader,
  int           width,
  int           height)
{
  g_return_if_fail (IS_FADER (fader));

  self->fader = fader;

  /* set size */
  gtk_widget_set_size_request (
    GTK_WIDGET (self), width, height);
}

static void
fader_widget_init (FaderWidget * self)
{
  self->start_color =  UI_COLORS->fader_fill_start;
  self->end_color =  UI_COLORS->fader_fill_end;

  gtk_widget_set_tooltip_text (
    GTK_WIDGET (self), _("Fader"));

  self->drag = GTK_GESTURE_DRAG (
    gtk_gesture_drag_new ());
  g_signal_connect (
    G_OBJECT(self->drag), "drag-begin",
    G_CALLBACK (drag_begin),  self);
  g_signal_connect (
    G_OBJECT(self->drag), "drag-update",
    G_CALLBACK (drag_update),  self);
  g_signal_connect (
    G_OBJECT(self->drag), "drag-end",
    G_CALLBACK (drag_end),  self);
  gtk_widget_add_controller (
    GTK_WIDGET (self),
    GTK_EVENT_CONTROLLER (self->drag));

#if 0
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
#endif

  GtkGestureClick * right_click =
    GTK_GESTURE_CLICK (
      gtk_gesture_click_new ());
  gtk_gesture_single_set_button (
    GTK_GESTURE_SINGLE (right_click),
    GDK_BUTTON_SECONDARY);
  g_signal_connect (
    G_OBJECT (right_click), "pressed",
    G_CALLBACK (on_right_click), self);
  gtk_widget_add_controller (
    GTK_WIDGET (self),
    GTK_EVENT_CONTROLLER (right_click));

  GtkEventControllerMotion * motion_controller =
    GTK_EVENT_CONTROLLER_MOTION (
      gtk_event_controller_motion_new ());
  g_signal_connect (
    G_OBJECT (motion_controller), "enter",
    G_CALLBACK (on_enter), self);
  g_signal_connect (
    G_OBJECT (motion_controller), "leave",
    G_CALLBACK (on_leave), self);
  gtk_widget_add_controller (
    GTK_WIDGET (self),
    GTK_EVENT_CONTROLLER (motion_controller));

  GtkEventControllerScroll * scroll_controller =
    GTK_EVENT_CONTROLLER_SCROLL (
      gtk_event_controller_scroll_new (
        GTK_EVENT_CONTROLLER_SCROLL_BOTH_AXES));
  g_signal_connect (
    G_OBJECT (scroll_controller), "scroll",
    G_CALLBACK (on_scroll),  self);
  gtk_widget_add_controller (
    GTK_WIDGET (self),
    GTK_EVENT_CONTROLLER (scroll_controller));

  gtk_drawing_area_set_draw_func (
    GTK_DRAWING_AREA (self), fader_draw_cb,
    self, NULL);
}

static void
fader_widget_class_init (FaderWidgetClass * _klass)
{
  GtkWidgetClass * klass = GTK_WIDGET_CLASS (_klass);
  gtk_widget_class_set_css_name (
    klass, "fader");
}
