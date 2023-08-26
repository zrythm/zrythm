// SPDX-FileCopyrightText: © 2018-2023 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <math.h>
#include <stdlib.h>

#include "actions/tracklist_selections.h"
#include "actions/undo_manager.h"
#include "dsp/midi_mapping.h"
#include "gui/widgets/balance_control.h"
#include "gui/widgets/bot_bar.h"
#include "gui/widgets/dialogs/bind_cc_dialog.h"
#include "gui/widgets/dialogs/string_entry_dialog.h"
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

G_DEFINE_TYPE (
  BalanceControlWidget,
  balance_control_widget,
  GTK_TYPE_WIDGET)

#define GET_VAL ((*self->getter) (self->object))
#define SET_VAL(real) ((*self->setter) (self->object, real))

#define TEXT_FONT "Bold 8"
#define TEXT_PADDING 3.0
#define LINE_WIDTH 3.0

/**
 * Returns the pan string.
 *
 * Must be free'd.
 */
static char *
get_pan_string (BalanceControlWidget * self, bool with_perc)
{
  /* make it from -0.5 to 0.5 */
  float pan_val = GET_VAL - 0.5f;

  /* get as percentage */
  pan_val = (fabsf (pan_val) / 0.5f) * 100.f;

  return g_strdup_printf (
    "%s%.0f%s", GET_VAL < 0.5f ? "-" : "", (double) pan_val,
    with_perc ? "%" : "");
}

static void
balance_control_snapshot (
  GtkWidget *   widget,
  GtkSnapshot * snapshot)
{
  BalanceControlWidget * self =
    Z_BALANCE_CONTROL_WIDGET (widget);

  int width = gtk_widget_get_allocated_width (widget);
  int height = gtk_widget_get_allocated_height (widget);

  if (!MAIN_WINDOW)
    return;

  float pan_val = GET_VAL;
  float value_px = pan_val * (float) width;
  float half_width = (float) width / 2.f;

  /* draw filled bg */
  GdkRGBA color = UI_COLORS->matcha;
  color.alpha = 0.4f;
  if (self->hovered || self->dragged)
    {
      color.alpha = 0.7f;
    }
  if (pan_val < 0.5f)
    {
      gtk_snapshot_append_color (
        snapshot, &color,
        &GRAPHENE_RECT_INIT (
          value_px, 0.f, (float) half_width - value_px,
          (float) height));
    }
  else
    {
      gtk_snapshot_append_color (
        snapshot, &color,
        &GRAPHENE_RECT_INIT (
          half_width, 0.f, value_px - (float) half_width,
          (float) height));
    }

  /* draw vertical line at current val */
  color.alpha = 0.7f;
  if (self->hovered || self->dragged)
    {
      color.alpha = 1.0;
    }
  const int line_width = 2;
  gtk_snapshot_append_color (
    snapshot, &color,
    &GRAPHENE_RECT_INIT (
      value_px, 0.f, (float) line_width, (float) height));

  /* draw text */
  PangoLayout *  layout = self->layout;
  PangoRectangle pangorect;
  color = Z_GDK_RGBA_INIT (
    UI_COLORS->bright_text.red, UI_COLORS->bright_text.green,
    UI_COLORS->bright_text.blue, color.alpha);
  pango_layout_set_text (layout, "L", -1);
  pango_layout_get_pixel_extents (layout, NULL, &pangorect);

  gtk_snapshot_save (snapshot);
  gtk_snapshot_translate (
    snapshot,
    &GRAPHENE_POINT_INIT (
      TEXT_PADDING,
      (float) height / 2.f - (float) pangorect.height / 2.f));
  gtk_snapshot_append_layout (snapshot, layout, &color);
  gtk_snapshot_restore (snapshot);

  pango_layout_set_text (layout, "R", -1);
  pango_layout_get_pixel_extents (layout, NULL, &pangorect);

  gtk_snapshot_save (snapshot);
  gtk_snapshot_translate (
    snapshot,
    &GRAPHENE_POINT_INIT (
      (float) width
        - ((float) TEXT_PADDING + (float) pangorect.width),
      (float) height / 2.f - (float) pangorect.height / 2.f));
  gtk_snapshot_append_layout (snapshot, layout, &color);
  gtk_snapshot_restore (snapshot);

  /* draw value if hovered */
  if (self->hovered)
    {
      char * val_str = get_pan_string (self, true);
      pango_layout_set_text (self->layout, val_str, -1);
      g_free (val_str);
      int x_px, y_px;
      pango_layout_get_pixel_size (self->layout, &x_px, &y_px);
      float   start_x = (float) width / 2.f - x_px / 2.f;
      float   start_y = height / 2.f - y_px / 2.f;
      GdkRGBA text_color = Z_GDK_RGBA_INIT (1, 1, 1, 1);
      gtk_snapshot_translate (
        snapshot, &GRAPHENE_POINT_INIT (start_x, start_y));
      gtk_snapshot_append_layout (
        snapshot, self->layout, &text_color);
    }
}

static void
on_motion_enter (
  GtkEventControllerMotion * motion_controller,
  gdouble                    x,
  gdouble                    y,
  gpointer                   user_data)
{
  BalanceControlWidget * self =
    Z_BALANCE_CONTROL_WIDGET (user_data);
  self->hovered = true;
}

static void
on_motion_leave (
  GtkEventControllerMotion * motion_controller,
  gpointer                   user_data)
{
  BalanceControlWidget * self =
    Z_BALANCE_CONTROL_WIDGET (user_data);
  self->hovered = false;
}

static void
on_drag_begin (
  GtkGestureDrag *       gesture,
  double                 start_x,
  double                 start_y,
  BalanceControlWidget * self)
{
  self->balance_at_start = GET_VAL;
}

static void
on_drag_update (
  GtkGestureDrag *       gesture,
  gdouble                offset_x,
  gdouble                offset_y,
  BalanceControlWidget * self)
{
  double sensitivity = 0.005;

  /* lower sensitivity if shift held */
  GdkModifierType mask;
  z_gtk_widget_get_mask (GTK_WIDGET (self), &mask);
  if (mask & GDK_SHIFT_MASK)
    {
      sensitivity = 0.002;
    }

  offset_y = -offset_y;
  int use_y =
    fabs (offset_y - self->last_y)
    > fabs (offset_x - self->last_x);
  double diff =
    sensitivity
    * (use_y ? offset_y - self->last_y : offset_x - self->last_x);

  float new_val = CLAMP (GET_VAL + (float) diff, 0.0f, 1.0f);

  SET_VAL (new_val);
  self->last_x = offset_x;
  self->last_y = offset_y;
  gtk_widget_queue_draw (GTK_WIDGET (self));

  char * pan_str = get_pan_string (self, true);
  char * str =
    g_strdup_printf ("%s: %s", _ ("Balance"), pan_str);
  gtk_widget_set_tooltip_text (GTK_WIDGET (self), str);
  g_free (pan_str);
  g_free (str);

  self->dragged = 1;
}

static void
on_drag_end (
  GtkGestureDrag *       gesture,
  gdouble                offset_x,
  gdouble                offset_y,
  BalanceControlWidget * self)
{
  self->last_x = 0;
  self->last_y = 0;
  self->dragged = 0;
  /*gtk_widget_hide (GTK_WIDGET (self->tooltip_win));*/

  if (
    IS_CHANNEL ((Channel *) self->object)
    && !math_floats_equal_epsilon (
      self->balance_at_start, GET_VAL, 0.0001f))
    {
      Track * track =
        channel_get_track ((Channel *) self->object);
      GError * err = NULL;
      bool     ret =
        tracklist_selections_action_perform_edit_single_float (
          EDIT_TRACK_ACTION_TYPE_PAN, track,
          self->balance_at_start, GET_VAL, true, &err);
      if (!ret)
        {
          HANDLE_ERROR_LITERAL (
            err, _ ("Failed to change balance"));
        }
    }
}

static void
show_context_menu (
  BalanceControlWidget * self,
  double                 x,
  double                 y)
{
  g_return_if_fail (self->port);

  GMenu *     menu = g_menu_new ();
  GMenuItem * menuitem;

  char tmp[600];
  sprintf (tmp, "app.reset-stereo-balance::%p", self->port);
  menuitem = z_gtk_create_menu_item (_ ("Reset"), NULL, tmp);
  g_menu_append_item (menu, menuitem);

  sprintf (tmp, "app.bind-midi-cc::%p", self->port);
  menuitem = CREATE_MIDI_LEARN_MENU_ITEM (tmp);
  g_menu_append_item (menu, menuitem);

  z_gtk_show_context_menu_from_g_menu (
    self->popover_menu, x, y, menu);
}

static void
on_right_click (
  GtkGestureClick *      gesture,
  gint                   n_press,
  gdouble                x,
  gdouble                y,
  BalanceControlWidget * self)
{
  if (n_press == 1)
    {
      show_context_menu (self, x, y);
    }
}

static void
set_val_with_action (void * object, const char * str)
{
  BalanceControlWidget * self =
    (BalanceControlWidget *) object;
  bool  is_valid = false;
  float val;
  if (math_is_string_valid_float (str, &val))
    {
      if (val <= 100.f && val >= -100.f)
        {
          is_valid = true;
        }
    }

  val = (val + 100.f) / 200.f;

  if (is_valid)
    {
      if (!math_floats_equal_epsilon (val, GET_VAL, 0.0001f))
        {
          Track * track =
            channel_get_track ((Channel *) self->object);
          GError * err = NULL;
          bool     ret =
            tracklist_selections_action_perform_edit_single_float (
              EDIT_TRACK_ACTION_TYPE_PAN, track, GET_VAL, val,
              false, &err);
          if (!ret)
            {
              HANDLE_ERROR_LITERAL (
                err, _ ("Failed to change balance"));
            }
        }
    }
  else /* else if not valid */
    {
      ui_show_error_message (
        _ ("Invalid Value"),
        _ ("Invalid value given to balance control"));
    }
}

static const char *
get_val_as_string (void * object)
{
  static char str[60];
  char *      pan_str =
    get_pan_string ((BalanceControlWidget *) object, false);
  strcpy (str, pan_str);
  g_free (pan_str);
  return str;
}

static void
on_click (
  GtkGestureClick *      gesture,
  gint                   n_press,
  gdouble                x,
  gdouble                y,
  BalanceControlWidget * self)
{

  if (n_press == 1)
    {
      GdkModifierType state =
        gtk_event_controller_get_current_event_state (
          GTK_EVENT_CONTROLLER (gesture));
      if (state & GDK_CONTROL_MASK)
        {
          Track * track =
            channel_get_track ((Channel *) self->object);
          GError * err = NULL;
          bool     ret =
            tracklist_selections_action_perform_edit_single_float (
              EDIT_TRACK_ACTION_TYPE_PAN, track, GET_VAL,
              0.5f, false, &err);
          if (!ret)
            {
              HANDLE_ERROR_LITERAL (
                err, _ ("Failed to change balance"));
            }
        }
    }
  else if (n_press == 2)
    {
      StringEntryDialogWidget * dialog =
        string_entry_dialog_widget_new (
          _ ("Balance Value"), self, get_val_as_string,
          set_val_with_action);
      gtk_window_present (GTK_WINDOW (dialog));
    }
}

static gboolean
balance_control_tick_cb (
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
  BalanceControlWidget * self = g_object_new (
    BALANCE_CONTROL_WIDGET_TYPE, "visible", 1,
    "height-request", height, NULL);
  self->getter = getter;
  self->setter = setter;
  self->object = object;
  self->port = port;

  /* add right mouse multipress */
  GtkGestureClick * right_mouse_mp =
    GTK_GESTURE_CLICK (gtk_gesture_click_new ());
  gtk_widget_add_controller (
    GTK_WIDGET (self), GTK_EVENT_CONTROLLER (right_mouse_mp));
  gtk_gesture_single_set_button (
    GTK_GESTURE_SINGLE (right_mouse_mp), GDK_BUTTON_SECONDARY);

  GtkGestureClick * double_click =
    GTK_GESTURE_CLICK (gtk_gesture_click_new ());
  gtk_gesture_single_set_button (
    GTK_GESTURE_SINGLE (double_click), GDK_BUTTON_PRIMARY);
  g_signal_connect (
    G_OBJECT (double_click), "pressed", G_CALLBACK (on_click),
    self);
  gtk_widget_add_controller (
    GTK_WIDGET (self), GTK_EVENT_CONTROLLER (double_click));

  GtkEventController * motion_controller =
    gtk_event_controller_motion_new ();
  g_signal_connect (
    G_OBJECT (motion_controller), "enter",
    G_CALLBACK (on_motion_enter), self);
  g_signal_connect (
    G_OBJECT (motion_controller), "leave",
    G_CALLBACK (on_motion_leave), self);
  gtk_widget_add_controller (
    GTK_WIDGET (self), motion_controller);

  self->drag = GTK_GESTURE_DRAG (gtk_gesture_drag_new ());
  g_signal_connect (
    G_OBJECT (self->drag), "drag-begin",
    G_CALLBACK (on_drag_begin), self);
  g_signal_connect (
    G_OBJECT (self->drag), "drag-update",
    G_CALLBACK (on_drag_update), self);
  g_signal_connect (
    G_OBJECT (self->drag), "drag-end",
    G_CALLBACK (on_drag_end), self);
  gtk_widget_add_controller (
    GTK_WIDGET (self), GTK_EVENT_CONTROLLER (self->drag));

  g_signal_connect (
    G_OBJECT (right_mouse_mp), "pressed",
    G_CALLBACK (on_right_click), self);

  return self;
}

static void
balance_control_finalize (BalanceControlWidget * self)
{
  G_OBJECT_CLASS (balance_control_widget_parent_class)
    ->finalize (G_OBJECT (self));
}

static void
dispose (BalanceControlWidget * self)
{
  gtk_widget_unparent (GTK_WIDGET (self->popover_menu));

  g_object_unref (self->layout);

  G_OBJECT_CLASS (balance_control_widget_parent_class)
    ->dispose (G_OBJECT (self));
}

static void
balance_control_widget_init (BalanceControlWidget * self)
{
  gtk_widget_set_focusable (GTK_WIDGET (self), true);

  gdk_rgba_parse (&self->start_color, "rgba(0%,100%,0%,1.0)");
  gdk_rgba_parse (&self->end_color, "rgba(0%,50%,50%,1.0)");

  self->popover_menu =
    GTK_POPOVER_MENU (gtk_popover_menu_new_from_model (NULL));
  gtk_widget_set_parent (
    GTK_WIDGET (self->popover_menu), GTK_WIDGET (self));

  gtk_widget_set_margin_start (GTK_WIDGET (self), 2);
  gtk_widget_set_margin_end (GTK_WIDGET (self), 2);

  self->drag = GTK_GESTURE_DRAG (gtk_gesture_drag_new ());
  gtk_widget_add_controller (
    GTK_WIDGET (self), GTK_EVENT_CONTROLLER (self->drag));

  PangoFontDescription * desc;
  self->layout =
    gtk_widget_create_pango_layout (GTK_WIDGET (self), NULL);
  desc = pango_font_description_from_string (TEXT_FONT);
  pango_layout_set_font_description (self->layout, desc);
  pango_font_description_free (desc);

  gtk_widget_add_tick_callback (
    GTK_WIDGET (self), balance_control_tick_cb, self, NULL);
}

static void
balance_control_widget_class_init (
  BalanceControlWidgetClass * klass)
{
  GtkWidgetClass * wklass = GTK_WIDGET_CLASS (klass);
  wklass->snapshot = balance_control_snapshot;
  gtk_widget_class_set_css_name (wklass, "balance-control");
  gtk_widget_class_set_accessible_role (
    wklass, GTK_ACCESSIBLE_ROLE_SLIDER);

  gtk_widget_class_set_layout_manager_type (
    wklass, GTK_TYPE_BIN_LAYOUT);

  GObjectClass * oklass = G_OBJECT_CLASS (klass);
  oklass->dispose = (GObjectFinalizeFunc) dispose;
  oklass->finalize =
    (GObjectFinalizeFunc) balance_control_finalize;
}
