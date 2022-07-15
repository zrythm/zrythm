// SPDX-FileCopyrightText: © 2018-2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <stdlib.h>

#include "actions/midi_mapping_action.h"
#include "actions/tracklist_selections.h"
#include "audio/channel.h"
#include "audio/fader.h"
#include "audio/midi_mapping.h"
#include "gui/backend/event.h"
#include "gui/backend/event_manager.h"
#include "gui/widgets/bot_bar.h"
#include "gui/widgets/dialogs/bind_cc_dialog.h"
#include "gui/widgets/fader.h"
#include "project.h"
#include "utils/flags.h"
#include "utils/gtk.h"
#include "utils/math.h"
#include "utils/resources.h"
#include "utils/ui.h"
#include "zrythm_app.h"

#include <glib/gi18n.h>

G_DEFINE_TYPE (FaderWidget, fader_widget, GTK_TYPE_WIDGET)

static void
fader_snapshot (GtkWidget * widget, GtkSnapshot * snapshot)
{
  FaderWidget * self = Z_FADER_WIDGET (widget);

  int width = gtk_widget_get_allocated_width (widget);
  int height = gtk_widget_get_allocated_height (widget);

  GtkStyleContext * context =
    gtk_widget_get_style_context (widget);

  gtk_snapshot_render_background (
    snapshot, context, 0, 0, width, height);

  float fader_val = self->fader ? self->fader->fader_val : 1.f;
  float value_px = height * fader_val;

  const float fill_radius = 2.f;

  /* draw background bar */
  GskRoundedRect rounded_rect;
  gsk_rounded_rect_init_from_rect (
    &rounded_rect, &GRAPHENE_RECT_INIT (0, 0, width, height),
    fill_radius);
  gtk_snapshot_push_rounded_clip (snapshot, &rounded_rect);
  gtk_snapshot_append_color (
    snapshot,
    &Z_GDK_RGBA_INIT (
      0.1f, 0.1f, 0.1f, self->hover ? 0.8f : 0.6f),
    &GRAPHENE_RECT_INIT (0, 0, width, height));
  gtk_snapshot_pop (snapshot);

  /*const int padding = 2;*/
  /*graphene_rect_t graphene_rect =*/
  /*GRAPHENE_RECT_INIT (*/
  /*padding, padding, width - padding * 2,*/
  /*height - padding * 2);*/

  /* draw filled in bar */
  double       intensity = fader_val;
  const double intensity_inv = 1.0 - intensity;
  double       r =
    intensity_inv * self->end_color.red
    + intensity * self->start_color.red;
  double g =
    intensity_inv * self->end_color.green
    + intensity * self->start_color.green;
  double b =
    intensity_inv * self->end_color.blue
    + intensity * self->start_color.blue;
  double a =
    intensity_inv * self->end_color.alpha
    + intensity * self->start_color.alpha;

  if (!self->hover)
    a = 0.9f;

  const float inner_line_width = 2.f;

  const int       border_width = 3.f;
  graphene_rect_t value_graphene_rect = GRAPHENE_RECT_INIT (
    border_width, border_width, width - border_width * 2,
    height - border_width * 2);
  gsk_rounded_rect_init_from_rect (
    &rounded_rect, &value_graphene_rect, fill_radius);
  gtk_snapshot_push_rounded_clip (snapshot, &rounded_rect);
  gtk_snapshot_append_color (
    snapshot,
    &Z_GDK_RGBA_INIT (
      (float) r, (float) g, (float) b, (float) a),
    &GRAPHENE_RECT_INIT (
      0, (float) (height - value_px) + inner_line_width * 2,
      width, value_px));
  gtk_snapshot_pop (snapshot);

#if 0
  /* draw fader thick line */
  const int line_width = 12;
  value_graphene_rect =
    GRAPHENE_RECT_INIT (
      border_width,
      (height - value_px) - line_width / 2,
      width - border_width * 2, line_width);
  gsk_rounded_rect_init_from_rect (
    &rounded_rect, &value_graphene_rect, 1.4f);
  gtk_snapshot_push_rounded_clip (
    snapshot, &rounded_rect);
  gtk_snapshot_append_color (
    snapshot,
    &Z_GDK_RGBA_INIT (0, 0, 0, 1),
    &value_graphene_rect);
  gtk_snapshot_pop (snapshot);
#endif

  /* draw fader thin line */
  GdkRGBA color;
  if (self->hover || self->dragging)
    {
      color = Z_GDK_RGBA_INIT (0.8f, 0.8f, 0.8f, 1);
    }
  else
    {
      color = Z_GDK_RGBA_INIT (0.6f, 0.6f, 0.6f, 1);
    }
  gtk_snapshot_append_color (
    snapshot, &color,
    &GRAPHENE_RECT_INIT (
      border_width,
      (height - value_px) - inner_line_width / 2.f,
      width - border_width * 2, inner_line_width));
}

static void
on_enter (
  GtkEventControllerMotion * motion_controller,
  gdouble                    x,
  gdouble                    y,
  gpointer                   user_data)
{
  FaderWidget * self = Z_FADER_WIDGET (user_data);
  self->hover = true;
  gtk_widget_queue_draw (GTK_WIDGET (self));
}

static void
on_leave (
  GtkEventControllerMotion * motion_controller,
  gpointer                   user_data)
{
  FaderWidget * self = Z_FADER_WIDGET (user_data);
  self->hover = false;
  gtk_widget_queue_draw (GTK_WIDGET (self));
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

#if 0
  char * string =
    g_strdup_printf (
      "%.1f", (double) self->fader->volume);
  gtk_label_set_text (
    self->tooltip_label, string);
  g_free (string);
  gtk_window_present (self->tooltip_win);
#endif

  self->amp_at_start = fader_get_amp (self->fader);
  self->dragging = true;
}

static void
drag_update (
  GtkGestureDrag * gesture,
  gdouble          offset_x,
  gdouble          offset_y,
  gpointer         user_data)
{
  FaderWidget * self = (FaderWidget *) user_data;
  offset_y = -offset_y;

  g_return_if_fail (IS_FADER (self->fader));

  /*int use_y = abs(offset_y - self->last_y) > abs(offset_x - self->last_x);*/
  int use_y = 1;

  /*double multiplier = 0.005;*/
  double diff =
    use_y ? offset_y - self->last_y : offset_x - self->last_x;
  double height =
    gtk_widget_get_allocated_height (GTK_WIDGET (self));
  double adjusted_diff = diff / height;

  /* lower sensitivity if shift held */
  GdkModifierType mask;
  z_gtk_widget_get_mask (GTK_WIDGET (self), &mask);
  if (mask & GDK_SHIFT_MASK)
    {
      adjusted_diff *= 0.4;
    }

  double new_fader_val = CLAMP (
    (double) self->fader->fader_val + adjusted_diff, 0.0, 1.0);
  fader_set_fader_val (self->fader, (float) new_fader_val);
  self->last_x = offset_x;
  self->last_y = offset_y;
  gtk_widget_queue_draw (GTK_WIDGET (self));

#if 0
  char * string =
    g_strdup_printf (
      "%.1f", (double) self->fader->volume);
  gtk_label_set_text (
    self->tooltip_label, string);
  g_free (string);
  gtk_window_present (self->tooltip_win);
#endif
}

static void
drag_end (
  GtkGestureDrag * gesture,
  gdouble          offset_x,
  gdouble          offset_y,
  gpointer         user_data)
{
  FaderWidget * self = (FaderWidget *) user_data;
  self->last_x = 0;
  self->last_y = 0;
  /*gtk_widget_hide (GTK_WIDGET (self->tooltip_win));*/

  g_return_if_fail (IS_FADER (self->fader));

  float cur_amp = fader_get_amp (self->fader);
  fader_set_amp_with_action (
    self->fader, self->amp_at_start, cur_amp, true);

  self->dragging = false;
}

static void
show_context_menu (FaderWidget * self, double x, double y)
{
  GMenu *     menu = g_menu_new ();
  GMenuItem * menuitem;

  char tmp[600];
  sprintf (tmp, "app.reset-fader::%p", self->fader);
  menuitem = z_gtk_create_menu_item (_ ("Reset"), NULL, tmp);
  g_menu_append_item (menu, menuitem);

  sprintf (tmp, "app.bind-midi-cc::%p", self->fader->amp);
  menuitem = CREATE_MIDI_LEARN_MENU_ITEM (tmp);
  g_menu_append_item (menu, menuitem);

  z_gtk_show_context_menu_from_g_menu (
    self->popover_menu, x, y, menu);
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

  GdkEvent * event = gtk_event_controller_get_current_event (
    GTK_EVENT_CONTROLLER (scroll_controller));
  GdkScrollDirection direction =
    gdk_scroll_event_get_direction (event);
  double abs_x, abs_y;
  gdk_event_get_position (event, &abs_x, &abs_y);

  GdkModifierType state =
    gtk_event_controller_get_current_event_state (
      GTK_EVENT_CONTROLLER (scroll_controller));

  /* add/subtract *inc* amount */
  float inc = 0.04f;
  /* lower sensitivity if shift held */
  if (state & GDK_SHIFT_MASK)
    {
      inc = 0.01f;
    }
  int up_down =
    (direction == GDK_SCROLL_UP
     || direction == GDK_SCROLL_RIGHT)
      ? 1
      : -1;
  float add_val = (float) up_down * inc;
  float current_val = fader_get_fader_val (self->fader);
  float new_val = CLAMP (current_val + add_val, 0.0f, 1.0f);
  fader_set_fader_val (self->fader, new_val);

  Channel * channel = fader_get_channel (self->fader);
  EVENTS_PUSH (ET_CHANNEL_FADER_VAL_CHANGED, channel);

  return true;
}

static gboolean
fader_tick_cb (
  GtkWidget *     widget,
  GdkFrameClock * frame_clock,
  gpointer        user_data)
{
  if (!gtk_widget_get_mapped (widget))
    {
      return G_SOURCE_CONTINUE;
    }

  gtk_widget_queue_draw (widget);

  return G_SOURCE_CONTINUE;
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
dispose (FaderWidget * self)
{
  gtk_widget_unparent (GTK_WIDGET (self->popover_menu));

  G_OBJECT_CLASS (fader_widget_parent_class)
    ->dispose (G_OBJECT (self));
}

static void
fader_widget_init (FaderWidget * self)
{
  self->start_color = UI_COLORS->fader_fill_start;
  self->end_color = UI_COLORS->fader_fill_end;

  gtk_widget_set_focusable (GTK_WIDGET (self), true);

  self->popover_menu =
    GTK_POPOVER_MENU (gtk_popover_menu_new_from_model (NULL));
  gtk_widget_set_parent (
    GTK_WIDGET (self->popover_menu), GTK_WIDGET (self));

  gtk_widget_set_tooltip_text (GTK_WIDGET (self), _ ("Fader"));

  self->drag = GTK_GESTURE_DRAG (gtk_gesture_drag_new ());
  g_signal_connect (
    G_OBJECT (self->drag), "drag-begin",
    G_CALLBACK (drag_begin), self);
  g_signal_connect (
    G_OBJECT (self->drag), "drag-update",
    G_CALLBACK (drag_update), self);
  g_signal_connect (
    G_OBJECT (self->drag), "drag-end", G_CALLBACK (drag_end),
    self);
  gtk_widget_add_controller (
    GTK_WIDGET (self), GTK_EVENT_CONTROLLER (self->drag));

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
    GTK_GESTURE_CLICK (gtk_gesture_click_new ());
  gtk_gesture_single_set_button (
    GTK_GESTURE_SINGLE (right_click), GDK_BUTTON_SECONDARY);
  g_signal_connect (
    G_OBJECT (right_click), "pressed",
    G_CALLBACK (on_right_click), self);
  gtk_widget_add_controller (
    GTK_WIDGET (self), GTK_EVENT_CONTROLLER (right_click));

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
    G_CALLBACK (on_scroll), self);
  gtk_widget_add_controller (
    GTK_WIDGET (self),
    GTK_EVENT_CONTROLLER (scroll_controller));

  gtk_widget_add_tick_callback (
    GTK_WIDGET (self), fader_tick_cb, self, NULL);
}

static void
fader_widget_class_init (FaderWidgetClass * klass)
{
  GtkWidgetClass * wklass = GTK_WIDGET_CLASS (klass);
  wklass->snapshot = fader_snapshot;
  gtk_widget_class_set_css_name (wklass, "fader");

  gtk_widget_class_set_layout_manager_type (
    wklass, GTK_TYPE_BIN_LAYOUT);

  GObjectClass * oklass = G_OBJECT_CLASS (klass);
  oklass->dispose = (GObjectFinalizeFunc) dispose;
}
