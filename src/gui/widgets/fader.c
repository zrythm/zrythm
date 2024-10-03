// SPDX-FileCopyrightText: © 2018-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <stdlib.h>

#include "actions/midi_mapping_action.h"
#include "actions/tracklist_selections.h"
#include "dsp/channel.h"
#include "dsp/fader.h"
#include "dsp/midi_mapping.h"
#include "gui/backend/event.h"
#include "gui/backend/event_manager.h"
#include "gui/widgets/bot_bar.h"
#include "gui/widgets/dialogs/bind_cc_dialog.h"
#include "gui/widgets/dialogs/string_entry_dialog.h"
#include "gui/widgets/fader.h"
#include "project.h"
#include "utils/cairo.h"
#include "utils/flags.h"
#include "utils/gtk.h"
#include "utils/math.h"
#include "utils/resources.h"
#include "utils/ui.h"
#include "zrythm_app.h"

#include <glib/gi18n.h>

static void
accessible_range_init (GtkAccessibleRangeInterface * iface);
G_DEFINE_TYPE_WITH_CODE (
  FaderWidget,
  fader_widget,
  GTK_TYPE_WIDGET,
  G_IMPLEMENT_INTERFACE (GTK_TYPE_ACCESSIBLE_RANGE, accessible_range_init))

static gboolean
accessible_range_set_current_value (
  GtkAccessibleRange * accessible_range,
  double               value)
{
  FaderWidget * self = Z_FADER_WIDGET (accessible_range);
  float         cur_amp = fader_get_amp (self->fader);
  fader_set_amp_with_action (self->fader, cur_amp, (float) value, true);

  return TRUE;
}

static void
accessible_range_init (GtkAccessibleRangeInterface * iface)
{
  iface->set_current_value = accessible_range_set_current_value;
}

/* previous implementation */
static void
fader_snapshot_old (GtkWidget * widget, GtkSnapshot * snapshot)
{
  FaderWidget * self = Z_FADER_WIDGET (widget);

  int width = gtk_widget_get_width (widget);
  int height = gtk_widget_get_height (widget);

  float fader_val = self->fader ? self->fader->fader_val : 1.f;
  float value_px = (float) height * fader_val;

  const float fill_radius = 2.f;

  /* draw background bar */
  GskRoundedRect rounded_rect;
  gsk_rounded_rect_init_from_rect (
    &rounded_rect,
    &GRAPHENE_RECT_INIT (0.f, 0.f, (float) width, (float) height), fill_radius);
  gtk_snapshot_push_rounded_clip (snapshot, &rounded_rect);
  gtk_snapshot_append_color (
    snapshot, &Z_GDK_RGBA_INIT (0.1f, 0.1f, 0.1f, self->hover ? 0.8f : 0.6f),
    &GRAPHENE_RECT_INIT (0.f, 0.f, (float) width, (float) height));
  gtk_snapshot_pop (snapshot);

  /* draw filled in bar */
  GdkRGBA fg_color;
  gtk_widget_get_color (widget, &fg_color);
  double       intensity = fader_val;
  const double intensity_inv = 1.0 - intensity;
  double r = intensity_inv * self->end_color.red + intensity * fg_color.red;
  double g = intensity_inv * self->end_color.green + intensity * fg_color.green;
  double b = intensity_inv * self->end_color.blue + intensity * fg_color.blue;
  double a = intensity_inv * self->end_color.alpha + intensity * fg_color.alpha;

  if (!self->hover)
    a = 0.9f;

  const float inner_line_width = 2.f;

  const float     border_width = 3.f;
  graphene_rect_t value_graphene_rect = GRAPHENE_RECT_INIT (
    border_width, border_width, (float) width - border_width * 2.f,
    (float) height - border_width * 2.f);
  gsk_rounded_rect_init_from_rect (
    &rounded_rect, &value_graphene_rect, fill_radius);
  gtk_snapshot_push_rounded_clip (snapshot, &rounded_rect);
  gtk_snapshot_append_color (
    snapshot, &Z_GDK_RGBA_INIT ((float) r, (float) g, (float) b, (float) a),
    &GRAPHENE_RECT_INIT (
      0.f, ((float) height - value_px) + (float) inner_line_width * 2.f,
      (float) width, value_px));
  gtk_snapshot_pop (snapshot);

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
      border_width, ((float) height - value_px) - (float) inner_line_width / 2.f,
      (float) width - border_width * 2.f, inner_line_width));

  /* draw value */
  if (self->hover)
    {
      char val_str[60];
      ui_get_db_value_as_string (self->fader->volume, val_str);
      pango_layout_set_text (self->layout, val_str, -1);
      int x_px, y_px;
      pango_layout_get_pixel_size (self->layout, &x_px, &y_px);
      float   start_x = (float) width / 2.f - x_px / 2.f;
      bool    show_text_at_bottom = y_px > (height - value_px);
      float   start_y = show_text_at_bottom ? height / 2.f - y_px / 2.f : 0;
      GdkRGBA text_color =
        show_text_at_bottom
          ? Z_GDK_RGBA_INIT (1, 1, 1, 1)
          : Z_GDK_RGBA_INIT (1, 1, 1, 1);
      gtk_snapshot_translate (snapshot, &GRAPHENE_POINT_INIT (start_x, start_y));
      gtk_snapshot_append_layout (snapshot, self->layout, &text_color);
    }
}

static void
fader_snapshot (GtkWidget * widget, GtkSnapshot * snapshot)
{
  FaderWidget * self = Z_FADER_WIDGET (widget);

  const bool use_old = true;
  if (use_old)
    {
      fader_snapshot_old (widget, snapshot);
      return;
    }

  float width = (float) gtk_widget_get_width (widget);
  float height = (float) gtk_widget_get_height (widget);

  float fader_val = self->fader ? self->fader->fader_val : 1.f;
  float value_px = height * fader_val;

  const float line_thickness = width / 5.f;
  const float line_radius = 4.f;

  const graphene_rect_t line_rect = GRAPHENE_RECT_INIT (
    width / 2.f - line_thickness / 2.f, 0.f, line_thickness, height);

  /* draw background bar */
  GskRoundedRect rounded_rect;
  gsk_rounded_rect_init_from_rect (&rounded_rect, &line_rect, line_radius);
  gtk_snapshot_push_rounded_clip (snapshot, &rounded_rect);
  gtk_snapshot_append_color (
    snapshot, &Z_GDK_RGBA_INIT (0.1f, 0.1f, 0.1f, self->hover ? 0.8f : 0.6f),
    &line_rect);

  /* draw filled in bar */
  double       intensity = fader_val;
  const double intensity_inv = 1.0 - intensity;
  GdkRGBA      fg_color;
  gtk_widget_get_color (widget, &fg_color);
  double r = intensity_inv * self->end_color.red + intensity * fg_color.red;
  double g = intensity_inv * self->end_color.green + intensity * fg_color.green;
  double b = intensity_inv * self->end_color.blue + intensity * fg_color.blue;
  double a = intensity_inv * self->end_color.alpha + intensity * fg_color.alpha;

  if (!self->hover)
    a = 0.9f;

  gtk_snapshot_append_color (
    snapshot, &Z_GDK_RGBA_INIT ((float) r, (float) g, (float) b, (float) a),
    &GRAPHENE_RECT_INIT (
      width / 2.f - line_thickness / 2.f, (height - value_px), line_thickness,
      value_px));
  gtk_snapshot_pop (snapshot);

  /* draw fader handle */
  const float           handle_height = 28.f;
  const float           handle_radius = 3.f;
  const graphene_rect_t handle_rect = GRAPHENE_RECT_INIT (
    0.f, (height - value_px) - handle_height / 2.f, width, handle_height);
  gsk_rounded_rect_init_from_rect (&rounded_rect, &handle_rect, handle_radius);
  gtk_snapshot_push_rounded_clip (snapshot, &rounded_rect);
  float handle_color_brightness = 0.72f;
  if (self->hover || self->dragging)
    {
      handle_color_brightness = 0.84f;
    }
  GdkRGBA color = Z_GDK_RGBA_INIT (
    handle_color_brightness, handle_color_brightness, handle_color_brightness,
    1);
  gtk_snapshot_append_color (snapshot, &color, &handle_rect);
  gtk_snapshot_pop (snapshot);

  /* draw value */
  if (self->hover)
    {
      char val_str[60];
      ui_get_db_value_as_string (self->fader->volume, val_str);
      pango_layout_set_text (self->layout, val_str, -1);
      int x_px, y_px;
      pango_layout_get_pixel_size (self->layout, &x_px, &y_px);
      float start_x = (float) width / 2.f - x_px / 2.f;
      /*bool    show_text_at_bottom = y_px > (height - value_px);*/
      float start_y = ((height - value_px) - handle_height / 2.f) + y_px / 2.f;
      const float text_color_brightness = 0.f;
      GdkRGBA     text_color = Z_GDK_RGBA_INIT (
        text_color_brightness, text_color_brightness, text_color_brightness,
        1.f);
      gtk_snapshot_translate (snapshot, &GRAPHENE_POINT_INIT (start_x, start_y));
      gtk_snapshot_append_layout (snapshot, self->layout, &text_color);
    }
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
on_leave (GtkEventControllerMotion * motion_controller, gpointer user_data)
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
  double diff = use_y ? offset_y - self->last_y : offset_x - self->last_x;
  double height = gtk_widget_get_height (GTK_WIDGET (self));
  double adjusted_diff = diff / height;

  /* lower sensitivity if shift held */
  GdkModifierType mask;
  z_gtk_widget_get_mask (GTK_WIDGET (self), &mask);
  if (mask & GDK_SHIFT_MASK)
    {
      adjusted_diff *= 0.4;
    }

  double new_fader_val =
    CLAMP ((double) self->fader->fader_val + adjusted_diff, 0.0, 1.0);
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
  fader_set_amp_with_action (self->fader, self->amp_at_start, cur_amp, true);

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

  z_gtk_show_context_menu_from_g_menu (self->popover_menu, x, y, menu);
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

static void
set_fader_val_with_action_from_db (void * object, const char * str)
{
  Fader * fader = (Fader *) object;
  bool    is_valid = false;
  float   val;
  if (math_is_string_valid_float (str, &val))
    {
      if (val <= 6.f)
        {
          is_valid = true;
        }
    }

  if (is_valid)
    {
      fader_set_amp_with_action (
        fader, fader_get_amp (object), math_dbfs_to_amp (val), true);
    }
  else
    {
      ui_show_error_message ("Error", _ ("Invalid value"));
    }
}

static const char *
get_fader_db_val_as_string (void * object)
{
  static char db_str[60];
  fader_db_string_getter (object, db_str);
  return db_str;
}

static void
on_click (
  GtkGestureClick * gesture,
  gint              n_press,
  gdouble           x,
  gdouble           y,
  FaderWidget *     self)
{
  if (n_press == 1)
    {
      GdkModifierType state = gtk_event_controller_get_current_event_state (
        GTK_EVENT_CONTROLLER (gesture));
      if (state & GDK_CONTROL_MASK)
        {
          fader_set_amp_with_action (
            self->fader, fader_get_amp (self->fader), 1.f, true);
        }
    }
  else if (n_press == 2)
    {
      StringEntryDialogWidget * dialog = string_entry_dialog_widget_new (
        _ ("Fader Value"), self->fader, get_fader_db_val_as_string,
        set_fader_val_with_action_from_db);
      gtk_window_present (GTK_WINDOW (dialog));
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
  GdkScrollDirection direction = gdk_scroll_event_get_direction (event);
  double             abs_x, abs_y;
  gdk_event_get_position (event, &abs_x, &abs_y);

  GdkModifierType state = gtk_event_controller_get_current_event_state (
    GTK_EVENT_CONTROLLER (scroll_controller));

  /* add/subtract *inc* amount */
  float inc = 0.04f;
  /* lower sensitivity if shift held */
  if (state & GDK_SHIFT_MASK)
    {
      inc = 0.01f;
    }
  int up_down =
    (direction == GDK_SCROLL_UP || direction == GDK_SCROLL_RIGHT) ? 1 : -1;
  float add_val = (float) up_down * inc;
  float current_val = fader_get_fader_val (self->fader);
  float new_val = CLAMP (current_val + add_val, 0.0f, 1.0f);
  fader_set_fader_val (self->fader, new_val);

  Channel * channel = fader_get_channel (self->fader);
  EVENTS_PUSH (ET_CHANNEL_FADER_VAL_CHANGED, channel);

  return true;
}

static gboolean
fader_tick_cb (GtkWidget * widget, GdkFrameClock * frame_clock, gpointer user_data)
{
  if (!gtk_widget_get_mapped (widget))
    {
      return G_SOURCE_CONTINUE;
    }

  /* let accessibility layer know if value changed */
  FaderWidget * self = Z_FADER_WIDGET (widget);
  double        cur_amp = fader_get_amp (self->fader);
  if (!math_doubles_equal (cur_amp, self->last_reported_amp))
    {
      gtk_accessible_update_property (
        GTK_ACCESSIBLE (self), GTK_ACCESSIBLE_PROPERTY_VALUE_NOW, cur_amp, -1);
    }

  gtk_widget_queue_draw (widget);

  return G_SOURCE_CONTINUE;
}

void
fader_widget_setup (FaderWidget * self, Fader * fader, int height)
{
  g_return_if_fail (IS_FADER (fader));

  self->fader = fader;

  /* set size */
  gtk_widget_set_size_request (GTK_WIDGET (self), 36, height);
}

static void
dispose (FaderWidget * self)
{
  gtk_widget_unparent (GTK_WIDGET (self->popover_menu));

  g_object_unref (self->layout);

  G_OBJECT_CLASS (fader_widget_parent_class)->dispose (G_OBJECT (self));
}

static void
fader_widget_init (FaderWidget * self)
{
  self->end_color = UI_COLORS->fader_fill_end;

  gtk_widget_set_focusable (GTK_WIDGET (self), true);
  gtk_widget_set_focus_on_click (GTK_WIDGET (self), true);

  self->popover_menu = GTK_POPOVER_MENU (gtk_popover_menu_new_from_model (NULL));
  gtk_widget_set_parent (GTK_WIDGET (self->popover_menu), GTK_WIDGET (self));

  gtk_widget_set_tooltip_text (GTK_WIDGET (self), _ ("Fader"));

  self->layout = gtk_widget_create_pango_layout (GTK_WIDGET (self), NULL);
  {
    PangoFontDescription * desc = pango_font_description_from_string ("7");
    pango_layout_set_font_description (self->layout, desc);
    pango_font_description_free (desc);
  }

  self->drag = GTK_GESTURE_DRAG (gtk_gesture_drag_new ());
  g_signal_connect (
    G_OBJECT (self->drag), "drag-begin", G_CALLBACK (drag_begin), self);
  g_signal_connect (
    G_OBJECT (self->drag), "drag-update", G_CALLBACK (drag_update), self);
  g_signal_connect (
    G_OBJECT (self->drag), "drag-end", G_CALLBACK (drag_end), self);
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

  GtkGestureClick * right_click = GTK_GESTURE_CLICK (gtk_gesture_click_new ());
  gtk_gesture_single_set_button (
    GTK_GESTURE_SINGLE (right_click), GDK_BUTTON_SECONDARY);
  g_signal_connect (
    G_OBJECT (right_click), "pressed", G_CALLBACK (on_right_click), self);
  gtk_widget_add_controller (
    GTK_WIDGET (self), GTK_EVENT_CONTROLLER (right_click));

  GtkGestureClick * double_click = GTK_GESTURE_CLICK (gtk_gesture_click_new ());
  gtk_gesture_single_set_button (
    GTK_GESTURE_SINGLE (double_click), GDK_BUTTON_PRIMARY);
  g_signal_connect (
    G_OBJECT (double_click), "pressed", G_CALLBACK (on_click), self);
  gtk_widget_add_controller (
    GTK_WIDGET (self), GTK_EVENT_CONTROLLER (double_click));

  GtkEventControllerMotion * motion_controller =
    GTK_EVENT_CONTROLLER_MOTION (gtk_event_controller_motion_new ());
  g_signal_connect (
    G_OBJECT (motion_controller), "enter", G_CALLBACK (on_enter), self);
  g_signal_connect (
    G_OBJECT (motion_controller), "leave", G_CALLBACK (on_leave), self);
  gtk_widget_add_controller (
    GTK_WIDGET (self), GTK_EVENT_CONTROLLER (motion_controller));

  GtkEventControllerScroll * scroll_controller = GTK_EVENT_CONTROLLER_SCROLL (
    gtk_event_controller_scroll_new (GTK_EVENT_CONTROLLER_SCROLL_BOTH_AXES));
  g_signal_connect (
    G_OBJECT (scroll_controller), "scroll", G_CALLBACK (on_scroll), self);
  gtk_widget_add_controller (
    GTK_WIDGET (self), GTK_EVENT_CONTROLLER (scroll_controller));

  gtk_accessible_update_property (
    GTK_ACCESSIBLE (self), GTK_ACCESSIBLE_PROPERTY_VALUE_MAX, 2.0,
    GTK_ACCESSIBLE_PROPERTY_VALUE_MIN, 0.0, GTK_ACCESSIBLE_PROPERTY_VALUE_NOW,
    1.0, GTK_ACCESSIBLE_PROPERTY_LABEL, "Fader", -1);

  gtk_widget_add_tick_callback (GTK_WIDGET (self), fader_tick_cb, self, NULL);
}

static void
fader_widget_class_init (FaderWidgetClass * klass)
{
  GtkWidgetClass * wklass = GTK_WIDGET_CLASS (klass);
  wklass->snapshot = fader_snapshot;
  gtk_widget_class_set_css_name (wklass, "fader");

  gtk_widget_class_set_layout_manager_type (wklass, GTK_TYPE_BIN_LAYOUT);
  gtk_widget_class_set_accessible_role (wklass, GTK_ACCESSIBLE_ROLE_SLIDER);

  GObjectClass * oklass = G_OBJECT_CLASS (klass);
  oklass->dispose = (GObjectFinalizeFunc) dispose;
}
