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

#include <math.h>
#include <stdlib.h>

#include "actions/tracklist_selections.h"
#include "actions/undo_manager.h"
#include "audio/midi_mapping.h"
#include "gui/widgets/balance_control.h"
#include "gui/widgets/dialogs/bind_cc_dialog.h"
#include "gui/widgets/bot_bar.h"
#include "gui/widgets/main_window.h"
#include "gui/widgets/track.h"
#include "project.h"
#include "utils/error.h"
#include "utils/gtk.h"
#include "utils/math.h"
#include "utils/objects.h"
#include "utils/resources.h"
#include "utils/ui.h"
#include "zrythm.h"
#include "zrythm_app.h"

#include <glib/gi18n.h>

G_DEFINE_TYPE (BalanceControlWidget,
               balance_control_widget,
               GTK_TYPE_DRAWING_AREA)

#define GET_VAL ((*self->getter) (self->object))
#define SET_VAL(real) ((*self->setter)(self->object, real))

#define TEXT_FONT "Sans SemiBold 8"
#define TEXT_PADDING 3.0
#define LINE_WIDTH 3.0

static int
pan_draw_cb (
  GtkWidget * widget,
  cairo_t * cr,
  BalanceControlWidget * self)
{
  if (!MAIN_WINDOW)
    return false;

  GtkStyleContext *context =
    gtk_widget_get_style_context (widget);

  int width =
    gtk_widget_get_allocated_width (widget);
  int height =
    gtk_widget_get_allocated_height (widget);

  gtk_render_background (
    context, cr, 0, 0, width, height);

  /* draw filled in bar */
  float pan_val = GET_VAL;
  /*float intensity = pan_val;*/
  double value_px =
    (double) pan_val * (double) width;
  double half_width = (double) width / 2.0;
  /*const float intensity_inv = 1.0 - intensity;*/
  /*float r = intensity_inv * self->end_color.red   +*/
            /*intensity * self->start_color.red;*/
  /*float g = intensity_inv * self->end_color.green +*/
            /*intensity * self->start_color.green;*/
  /*float b = intensity_inv * self->end_color.blue  +*/
            /*intensity * self->start_color.blue;*/
  /*float a = intensity_inv * self->end_color.alpha  +*/
            /*intensity * self->start_color.alpha;*/

  gdk_cairo_set_source_rgba (
    cr, &UI_COLORS->dark_text);
  cairo_rectangle (
    cr, 0,
    height / 2.0 - LINE_WIDTH / 2.0,
    width, LINE_WIDTH);
  cairo_fill (cr);

  double alpha = 0.7;
  if (self->hovered || self->dragged)
    {
      alpha = 1.0;
    }
  cairo_set_source_rgba (
    cr,
    UI_COLORS->matcha.red,
    UI_COLORS->matcha.green,
    UI_COLORS->matcha.blue,
    alpha);
  if (pan_val < 0.5f)
    {
      cairo_rectangle (
        cr, value_px,
        height / 2.0 - LINE_WIDTH / 2.0,
        half_width - value_px, LINE_WIDTH);
    }
  else
    {
      cairo_rectangle (
        cr, half_width,
        height / 2.0 - LINE_WIDTH / 2.0,
        value_px - half_width, LINE_WIDTH);
    }
  cairo_fill (cr);

  /* draw text */
  PangoLayout * layout = self->layout;
  PangoRectangle pangorect;
  cairo_set_source_rgba (
    cr,
    UI_COLORS->bright_text.red,
    UI_COLORS->bright_text.green,
    UI_COLORS->bright_text.blue,
    alpha);
#if 0
  if (pan_val >= 0.495f &&
      pan_val < 0.505f)
    {
      pango_layout_set_text (layout, "C", -1);
      pango_layout_get_pixel_extents (
        layout, NULL, &pangorect);
      cairo_move_to (
        cr,
        width / 2.0 - pangorect.width / 2.0,
        height / 2.0 - pangorect.height / 2.0);
      pango_cairo_show_layout (cr, layout);
    }
  char str[40];
  else if (pan_val > 0.5f)
    {
      int perc =
        (int) ((pan_val - 0.5f) * 200.f);
      sprintf (str, "R%d", perc);
      pango_layout_set_text (layout, str, -1);
    }
  else if (pan_val < 0.5f)
    {
      int perc =
        (int) ((0.5f - pan_val) * 200.f);
      sprintf (str, "L%d", perc);
      pango_layout_set_text (layout, str, -1);
    }
#endif
  pango_layout_set_text (layout, "L", -1);
  pango_layout_get_pixel_extents (
    layout, NULL, &pangorect);
  cairo_move_to (
    cr, TEXT_PADDING,
    height / 2.0 - pangorect.height / 2.0);
  pango_cairo_show_layout (cr, layout);
  pango_layout_set_text (layout, "R", -1);
  pango_layout_get_pixel_extents (
    layout, NULL, &pangorect);
  cairo_move_to (
    cr, width - (TEXT_PADDING + pangorect.width),
    height / 2.0 - pangorect.height / 2.0);
  pango_cairo_show_layout (cr, layout);

  return FALSE;
}

/**
 * Returns the pan string.
 *
 * Must be free'd.
 */
static char *
get_pan_string (
  BalanceControlWidget * self)
{
  /* make it from -0.5 to 0.5 */
  float pan_val = GET_VAL - 0.5f;

  /* get as percentage */
  pan_val = (fabsf (pan_val) / 0.5f) * 100.f;

  return
    g_strdup_printf (
      "%s%.0f%%",
      GET_VAL < 0.5f ? "-" : "",
      (double) pan_val);
}

static gboolean
on_motion (
  GtkWidget * widget,
  GdkEvent *  event,
  BalanceControlWidget * self)
{
  if (gdk_event_get_event_type (event) ==
      GDK_ENTER_NOTIFY)
    {
      /*gtk_widget_set_state_flags (*/
        /*GTK_WIDGET (self),*/
        /*GTK_STATE_FLAG_PRELIGHT, 0);*/
      self->hovered = 1;
    }
  else if (gdk_event_get_event_type (event) ==
           GDK_LEAVE_NOTIFY)
    {
      /*gtk_widget_unset_state_flags (*/
        /*GTK_WIDGET (self),*/
        /*GTK_STATE_FLAG_PRELIGHT);*/
      self->hovered = 0;
    }

  return FALSE;
}

static void
drag_begin (
  GtkGestureDrag * gesture,
  double           start_x,
  double           start_y,
  BalanceControlWidget * self)
{
  self->balance_at_start = GET_VAL;
}

static void
drag_update (
  GtkGestureDrag * gesture,
  gdouble         offset_x,
  gdouble         offset_y,
  BalanceControlWidget *     self)
{
  double sensitivity = 0.005;

  /* lower sensitivity if shift held */
  GdkModifierType mask;
  z_gtk_widget_get_mask (
    GTK_WIDGET (self), &mask);
  if (mask & GDK_SHIFT_MASK)
    {
      sensitivity = 0.002;
    }

  offset_y = - offset_y;
  int use_y =
    fabs (offset_y - self->last_y) >
    fabs (offset_x - self->last_x);
  double diff =
    sensitivity *
    (use_y ?
       offset_y - self->last_y :
       offset_x - self->last_x);

  float new_val =
    CLAMP (
      GET_VAL + (float) diff, 0.0f, 1.0f);

  SET_VAL (new_val);
  self->last_x = offset_x;
  self->last_y = offset_y;
  gtk_widget_queue_draw (GTK_WIDGET (self));

  char * str =
    get_pan_string (self);
  gtk_label_set_text (
    self->tooltip_label, str);
  gtk_widget_set_tooltip_text (
    GTK_WIDGET (self), str);
  g_free (str);
  gtk_window_present (self->tooltip_win);

  self->dragged = 1;
}

static void
drag_end (
  GtkGestureDrag *       gesture,
  gdouble                offset_x,
  gdouble                offset_y,
  BalanceControlWidget * self)
{
  self->last_x = 0;
  self->last_y = 0;
  self->dragged = 0;
  gtk_widget_hide (GTK_WIDGET (self->tooltip_win));

  if (IS_CHANNEL ((Channel *) self->object) &&
      !math_floats_equal_epsilon (
        self->balance_at_start, GET_VAL, 0.0001f))
    {
      Track * track =
        channel_get_track ((Channel *) self->object);
      GError * err = NULL;
      bool ret =
        tracklist_selections_action_perform_edit_single_float (
          EDIT_TRACK_ACTION_TYPE_PAN,
          track, self->balance_at_start,
          GET_VAL, true, &err);
      if (!ret)
        {
          HANDLE_ERROR (
            err, "%s",
            _("Failed to change balance"));
        }
    }
}

static void
show_context_menu (
  BalanceControlWidget * self)
{
  g_return_if_fail (self->port);

  GMenu * menu = g_menu_new ();
  GMenuItem * menuitem;

  char tmp[600];
  sprintf (
    tmp, "app.reset-stereo-balance::%p", self->port);
  menuitem =
    z_gtk_create_menu_item (
      _("Reset"), NULL, tmp);
  g_menu_append_item (menu, menuitem);

  sprintf (
    tmp, "app.bind-midi-cc::%p", self->port);
  menuitem =
    CREATE_MIDI_LEARN_MENU_ITEM (tmp);
  g_menu_append_item (menu, menuitem);

  z_gtk_show_context_menu_from_g_menu (
    GTK_WIDGET (self), menu);
}

static void
on_right_click (
  GtkGestureClick *gesture,
  gint                  n_press,
  gdouble               x,
  gdouble               y,
  BalanceControlWidget * self)
{
  if (n_press == 1)
    {
      show_context_menu (self);
    }
}

/**
 * Creates a new BalanceControl widget and binds it
 * to the given value.
 *
 * @param port Optional port to use in MIDI CC
 *   binding dialog.
 */
BalanceControlWidget *
balance_control_widget_new (
  GenericFloatGetter getter,
  GenericFloatSetter setter,
  void *             object,
  Port *             port,
  int                height)
{
  BalanceControlWidget * self =
    g_object_new (
      BALANCE_CONTROL_WIDGET_TYPE,
      "visible", 1,
      "height-request", height,
      NULL);
  self->getter = getter;
  self->setter = setter;
  self->object = object;
  self->port = port;

  /* add right mouse multipress */
  GtkGestureClick * right_mouse_mp =
    GTK_GESTURE_CLICK (
      gtk_gesture_click_new ());
  gtk_widget_add_controller (
    GTK_WIDGET (self),
    GTK_EVENT_CONTROLLER (right_mouse_mp));
  gtk_gesture_single_set_button (
    GTK_GESTURE_SINGLE (right_mouse_mp),
    GDK_BUTTON_SECONDARY);

  /* connect signals */
  g_signal_connect (
    G_OBJECT (self), "draw",
    G_CALLBACK (pan_draw_cb), self);
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

  return self;
}

static void
balance_control_finalize (
  BalanceControlWidget * self)
{
  g_debug ("finalizing...");

  G_OBJECT_CLASS (
    balance_control_widget_parent_class)->
      finalize (G_OBJECT (self));

  g_debug ("done");
}

static void
balance_control_widget_init (
  BalanceControlWidget * self)
{
  gdk_rgba_parse (
    &self->start_color, "rgba(0%,100%,0%,1.0)");
  gdk_rgba_parse (
    &self->end_color, "rgba(0%,50%,50%,1.0)");

  gtk_widget_set_margin_start (
    GTK_WIDGET (self), 2);
  gtk_widget_set_margin_end (
    GTK_WIDGET (self), 2);

  /* TODO port to gtk_tooltip_set_custom() */
#if 0
  self->tooltip_win =
    GTK_WINDOW (gtk_window_new (GTK_WINDOW_POPUP));
  gtk_window_set_type_hint (
    self->tooltip_win,
    GDK_WINDOW_TYPE_HINT_TOOLTIP);
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

  self->drag =
    GTK_GESTURE_DRAG (
      gtk_gesture_drag_new ());
  gtk_widget_add_controller (
    GTK_WIDGET (self),
    GTK_EVENT_CONTROLLER (self->drag));

  PangoFontDescription * desc;
  self->layout =
    gtk_widget_create_pango_layout (
      GTK_WIDGET (self), NULL);
  desc =
    pango_font_description_from_string (TEXT_FONT);
  pango_layout_set_font_description (
    self->layout, desc);
  pango_font_description_free (desc);
}

static void
balance_control_widget_class_init (BalanceControlWidgetClass * _klass)
{
  GtkWidgetClass * klass =
    GTK_WIDGET_CLASS (_klass);
  gtk_widget_class_set_css_name (
    klass, "balance-control");

  GObjectClass * oklass =
    G_OBJECT_CLASS (klass);
  oklass->finalize =
    (GObjectFinalizeFunc) balance_control_finalize;
}
