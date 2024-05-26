// SPDX-FileCopyrightText: © 2019-2023 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "dsp/port.h"
#include "dsp/port_connection.h"
#include "gui/widgets/bar_slider.h"
#include "utils/cairo.h"
#include "utils/gtk.h"
#include "utils/math.h"
#include "utils/objects.h"

#include <gtk/gtk.h>

static void
accessible_range_init (GtkAccessibleRangeInterface * iface);
G_DEFINE_TYPE_WITH_CODE (
  BarSliderWidget,
  bar_slider_widget,
  GTK_TYPE_WIDGET,
  G_IMPLEMENT_INTERFACE (GTK_TYPE_ACCESSIBLE_RANGE, accessible_range_init))

/**
 * Macro to get real value from bar_slider value.
 */
#define REAL_VAL_FROM_BAR_SLIDER(bar_slider) \
  (self->min + (float) bar_slider * (self->max - self->min))

/**
 * Converts from real value to bar_slider value
 */
#define BAR_SLIDER_VAL_FROM_REAL(real) \
  (((float) real - self->min) / (self->max - self->min))

static gboolean
accessible_range_set_current_value (
  GtkAccessibleRange * accessible_range,
  double               value)
{
  BarSliderWidget * self = Z_BAR_SLIDER_WIDGET (accessible_range);
  if (self->end_setter)
    {
      self->end_setter (self->object, REAL_VAL_FROM_BAR_SLIDER (value));
    }

  return TRUE;
}

static void
accessible_range_init (GtkAccessibleRangeInterface * iface)
{
  iface->set_current_value = accessible_range_set_current_value;
}

/**
 * Get the real value.
 */
static float
get_real_val (BarSliderWidget * self, bool snapped)
{
  if (self->type == BarSliderType::BAR_SLIDER_TYPE_PORT_MULTIPLIER)
    {
      PortConnection * conn = (PortConnection *) self->object;
      return conn->multiplier;
    }
  else
    {
      if (snapped && self->snapped_getter)
        {
          return self->snapped_getter (self->object);
        }
      else
        {
          return self->getter (self->object);
        }
    }
}

/**
 * Sets real val
 */
static void
set_real_val (BarSliderWidget * self, float real_val)
{
  if (self->type == BarSliderType::BAR_SLIDER_TYPE_PORT_MULTIPLIER)
    {
      PortConnection * connection = (PortConnection *) self->object;
      connection->multiplier = real_val;
    }
  else
    {
      (*self->setter) (self->object, real_val);
    }
}

static void
recreate_pango_layouts (BarSliderWidget * self)
{
  if (PANGO_IS_LAYOUT (self->layout))
    g_object_unref (self->layout);

  self->layout = z_cairo_create_pango_layout_from_string (
    (GtkWidget *) self, Z_CAIRO_FONT, PANGO_ELLIPSIZE_NONE, -1);
}

/**
 * Draws the bar_slider.
 */
static void
bar_slider_snapshot (GtkWidget * widget, GtkSnapshot * snapshot)
{
  /* do normal cast because this gets called a lot */
  BarSliderWidget * self = (BarSliderWidget *) widget;

  int width = gtk_widget_get_width (widget);
  int height = gtk_widget_get_height (widget);

  const float real_min = self->min;
  const float real_max = self->max;
  const float real_zero = self->zero;
  const float real_val = get_real_val (self, true);

  /* get absolute values in pixels */
  /*const float min_px = 0.f;*/
  /*const float max_px = width;*/
  const float zero_px =
    ((real_zero - real_min) / (real_max - real_min)) * (float) width;
  const float val_px =
    ((real_val - real_min) / (real_max - real_min)) * (float) width;

  GdkRGBA color = { 1, 1, 1, 0.3f };

  /* draw from val to zero */
  if (real_val < real_zero)
    {
      {
        graphene_rect_t tmp_r =
          Z_GRAPHENE_RECT_INIT (val_px, 0.f, zero_px - val_px, (float) height);
        gtk_snapshot_append_color (snapshot, &color, &tmp_r);
      }
    }
  /* draw from zero to val */
  else
    {
      {
        graphene_rect_t tmp_r =
          Z_GRAPHENE_RECT_INIT (zero_px, 0.f, val_px - zero_px, (float) height);
        gtk_snapshot_append_color (snapshot, &color, &tmp_r);
      }
    }

  if (!self->layout)
    recreate_pango_layouts (self);

  if (
    self->last_width_extent == 0
    || !math_floats_equal (self->last_real_val, real_val))
    {
      char str[3000];
      if (!self->show_value)
        {
          sprintf (str, "%s%s", self->prefix, self->suffix);
        }
      else if (self->decimals == 0)
        {
          sprintf (
            str, "%s%d%s", self->prefix,
            (int) (self->convert_to_percentage ? real_val * 100 : real_val),
            self->suffix);
        }
      else if (self->decimals < 5)
        {
          sprintf (
            str, "%s%.*f%s", self->prefix, self->decimals,
            (double) (self->convert_to_percentage ? real_val * 100.f : real_val),
            self->suffix);
        }
      else
        {
          g_warn_if_reached ();
        }
      int we, he;
      z_cairo_get_text_extents_for_widget (widget, self->layout, str, &we, &he);
      pango_layout_set_markup (self->layout, str, -1);
      self->last_width_extent = we;
      self->last_height_extent = he;
      strcpy (self->last_extent_str, str);
    }

  int we = self->last_width_extent;
  int he = self->last_height_extent;
  gtk_snapshot_save (snapshot);
  {
    graphene_point_t tmp_pt = Z_GRAPHENE_POINT_INIT (
      (float) width / 2.f - (float) we / 2.f,
      (float) height / 2.f - (float) he / 2.f);
    gtk_snapshot_translate (snapshot, &tmp_pt);
  }
  GdkRGBA tmp_color = Z_GDK_RGBA_INIT (1, 1, 1, 1);
  gtk_snapshot_append_layout (snapshot, self->layout, &color);
  gtk_snapshot_restore (snapshot);

  if (self->hover)
    {
      tmp_color = Z_GDK_RGBA_INIT (1, 1, 1, 0.12f);
      {
        graphene_rect_t tmp_r =
          Z_GRAPHENE_RECT_INIT (0.f, 0.f, (float) width, (float) height);
        gtk_snapshot_append_color (snapshot, &color, &tmp_r);
      }
    }

  self->last_real_val = real_val;
}

static void
on_motion_enter (
  GtkEventControllerMotion * controller,
  gdouble                    x,
  gdouble                    y,
  gpointer                   user_data)
{
  BarSliderWidget * self = Z_BAR_SLIDER_WIDGET (user_data);
  self->hover = true;
  gtk_widget_queue_draw (GTK_WIDGET (self));
}

static void
on_motion_leave (GtkEventControllerMotion * controller, gpointer user_data)
{
  BarSliderWidget * self = Z_BAR_SLIDER_WIDGET (user_data);
  self->hover = false;
  gtk_widget_queue_draw (GTK_WIDGET (self));
}

static void
drag_begin (
  GtkGestureDrag *  gesture,
  double            offset_x,
  double            offset_y,
  BarSliderWidget * self)
{
  if (!self->editable)
    return;

  self->start_x = offset_x;

  if (self->init_setter)
    {
      int    width = gtk_widget_get_width (GTK_WIDGET (self));
      double normalized_val = ui_get_normalized_draggable_value (
        width, BAR_SLIDER_VAL_FROM_REAL (get_real_val (self, false)),
        self->start_x, self->start_x, self->start_x, 1.0, self->mode);
      self->init_setter (
        self->object, REAL_VAL_FROM_BAR_SLIDER (normalized_val));
    }
}

static void
drag_update (
  GtkGestureDrag *  gesture,
  gdouble           offset_x,
  gdouble           offset_y,
  BarSliderWidget * self)
{
  if (!self->editable)
    return;

  int    width = gtk_widget_get_width (GTK_WIDGET (self));
  double new_normalized_val = ui_get_normalized_draggable_value (
    width, BAR_SLIDER_VAL_FROM_REAL (get_real_val (self, false)), self->start_x,
    self->start_x + offset_x, self->start_x + self->last_x, 1.0, self->mode);
  set_real_val (self, REAL_VAL_FROM_BAR_SLIDER (new_normalized_val));
  self->last_x = offset_x;
  gtk_widget_queue_draw ((GtkWidget *) self);
}

static void
drag_end (
  GtkGestureDrag *  gesture,
  gdouble           offset_x,
  gdouble           offset_y,
  BarSliderWidget * self)
{
  if (!self->editable)
    return;

  if (self->end_setter)
    {
      int    width = gtk_widget_get_width (GTK_WIDGET (self));
      double normalized_val = ui_get_normalized_draggable_value (
        width, BAR_SLIDER_VAL_FROM_REAL (get_real_val (self, false)),
        self->start_x, self->start_x + offset_x, self->start_x + self->last_x,
        1.0, self->mode);
      self->end_setter (self->object, REAL_VAL_FROM_BAR_SLIDER (normalized_val));
    }

  self->last_x = 0;
  self->start_x = 0;
}

/**
 * Creates a bar slider widget for floats.
 */
BarSliderWidget *
_bar_slider_widget_new (
  BarSliderType type,
  float (*get_val) (void *),
  void (*set_val) (void *, float),
  void *       object,
  float        min,
  float        max,
  int          w,
  int          h,
  float        zero,
  int          convert_to_percentage,
  int          decimals,
  UiDragMode   mode,
  const char * prefix,
  const char * suffix)
{
  g_warn_if_fail (object);

  BarSliderWidget * self = static_cast<BarSliderWidget *> (
    g_object_new (BAR_SLIDER_WIDGET_TYPE, NULL));
  self->type = type;
  self->getter = get_val;
  self->setter = set_val;
  self->object = object;
  self->min = min;
  self->max = max;
  self->hover = 0;
  self->zero = zero; /* default 0.05f */
  self->last_x = 0;
  self->start_x = 0;
  strcpy (self->suffix, suffix ? suffix : "");
  strcpy (self->prefix, prefix ? prefix : "");
  self->decimals = decimals;
  self->mode = mode;
  self->convert_to_percentage = convert_to_percentage;

  /* set size */
  gtk_widget_set_size_request (GTK_WIDGET (self), w, h);
  gtk_widget_set_hexpand (GTK_WIDGET (self), 1);
  gtk_widget_set_vexpand (GTK_WIDGET (self), 1);

  GtkEventController * motion_controller = gtk_event_controller_motion_new ();
  g_signal_connect (
    G_OBJECT (motion_controller), "enter", G_CALLBACK (on_motion_enter), self);
  g_signal_connect (
    G_OBJECT (motion_controller), "leave", G_CALLBACK (on_motion_leave), self);
  gtk_widget_add_controller (GTK_WIDGET (self), motion_controller);

  g_signal_connect (
    G_OBJECT (self->drag), "drag-begin", G_CALLBACK (drag_begin), self);
  g_signal_connect (
    G_OBJECT (self->drag), "drag-update", G_CALLBACK (drag_update), self);
  g_signal_connect (
    G_OBJECT (self->drag), "drag-end", G_CALLBACK (drag_end), self);

  return self;
}

static void
finalize (BarSliderWidget * self)
{
  g_return_if_fail (Z_IS_BAR_SLIDER_WIDGET (self));

  object_free_w_func_and_null (g_object_unref, self->layout);

  G_OBJECT_CLASS (bar_slider_widget_parent_class)->finalize (G_OBJECT (self));
}

static void
bar_slider_widget_init (BarSliderWidget * self)
{
  self->show_value = 1;
  self->editable = 1;

  /* make it able to notify */
  self->drag = GTK_GESTURE_DRAG (gtk_gesture_drag_new ());
  gtk_widget_add_controller (
    GTK_WIDGET (self), GTK_EVENT_CONTROLLER (self->drag));

  gtk_accessible_update_property (
    GTK_ACCESSIBLE (self), GTK_ACCESSIBLE_PROPERTY_VALUE_MAX, 1.0,
    GTK_ACCESSIBLE_PROPERTY_VALUE_MIN, 0.0, GTK_ACCESSIBLE_PROPERTY_LABEL,
    "Slider", -1);

  gtk_widget_set_focusable (GTK_WIDGET (self), true);
}

static void
bar_slider_widget_class_init (BarSliderWidgetClass * _klass)
{
  GtkWidgetClass * wklass = GTK_WIDGET_CLASS (_klass);
  wklass->snapshot = bar_slider_snapshot;
  gtk_widget_class_set_accessible_role (wklass, GTK_ACCESSIBLE_ROLE_SLIDER);

  GObjectClass * klass = G_OBJECT_CLASS (_klass);

  klass->finalize = (GObjectFinalizeFunc) finalize;
}
