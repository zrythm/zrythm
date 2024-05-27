// SPDX-FileCopyrightText: © 2018-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/track.h"
#include "gui/widgets/color_area.h"
#include "gui/widgets/dialogs/object_color_chooser_dialog.h"
#include "gui/widgets/main_window.h"
#include "gui/widgets/track.h"
#include "utils/cairo.h"
#include "utils/color.h"
#include "utils/gtk.h"
#include "utils/objects.h"
#include "utils/string.h"
#include "utils/ui.h"
#include "zrythm_app.h"

#include <glib/gi18n.h>

#include "gtk_wrapper.h"

G_DEFINE_TYPE (ColorAreaWidget, color_area_widget, GTK_TYPE_WIDGET)

#define icon_texture_size 16

static void
recreate_icon_texture (ColorAreaWidget * self)
{
  g_return_if_fail (self->track);
  object_free_w_func_and_null (g_object_unref, self->track_icon);
  self->track_icon = z_gdk_texture_new_from_icon_name (
    self->track->icon_name, icon_texture_size, icon_texture_size, 1);
  object_free_w_func_and_null (g_free, self->last_track_icon_name);
  self->last_track_icon_name = g_strdup (self->track->icon_name);
}

/**
 * Draws the color picker.
 */
static void
color_area_snapshot (GtkWidget * widget, GtkSnapshot * snapshot)
{
  ColorAreaWidget * self = Z_COLOR_AREA_WIDGET (widget);

  int width = gtk_widget_get_width (widget);
  int height = gtk_widget_get_height (widget);

  GdkRGBA color;
  color.alpha = 1.0;
  if (self->type == ColorAreaType::COLOR_AREA_TYPE_TRACK)
    {
      Track * track = self->track;
      if (track_is_enabled (track))
        {
          color = self->track->color;
        }
      else
        {
          color.red = 0.5;
          color.green = 0.5;
          color.blue = 0.5;
        }
    }
  else
    color = self->color;

  if (self->hovered)
    {
      color_brighten_default (&color);
    }

  {
    graphene_rect_t tmp_r =
      Z_GRAPHENE_RECT_INIT (0.f, 0.f, (float) width, (float) height);
    gtk_snapshot_append_color (snapshot, &color, &tmp_r);
  }

  if (self->type == ColorAreaType::COLOR_AREA_TYPE_TRACK)
    {
      Track * track = self->track;

      /* draw each parent */
      if (self->parents)
        {
          g_ptr_array_remove_range (self->parents, 0, self->parents->len);
        }
      else
        {
          self->parents = g_ptr_array_sized_new (8);
        }
      track_add_folder_parents (track, self->parents, false);

      size_t len = self->parents->len + 1;
      for (size_t i = 0; i < self->parents->len; i++)
        {
          Track * parent_track =
            static_cast<Track *> (g_ptr_array_index (self->parents, i));

          double start_y = ((double) i / (double) len) * (double) height;
          double h = (double) height / (double) len;

          color = parent_track->color;
          if (self->hovered)
            color_brighten_default (&color);
          {
            graphene_rect_t tmp_r = Z_GRAPHENE_RECT_INIT (
              0.f, (float) start_y, (float) width, (float) h);
            gtk_snapshot_append_color (snapshot, &color, &tmp_r);
          }
        }

      /* also show icon */
      if (width >= icon_texture_size && height >= icon_texture_size)
        {
          if (
            !self->track_icon
            || !string_is_equal (self->last_track_icon_name, track->icon_name))
            {
              recreate_icon_texture (self);
            }

          /* TODO figure out how to draw dark colors */
          GdkRGBA c2;
          ui_get_contrast_color (&track->color, &c2);
          graphene_matrix_t color_matrix;
          const float       float_arr[16] = {
            1, 1, 1, 0, 1, 1, 1, 0, 1, 1, 1, 0, 0, 0, 0, c2.alpha
          };
          graphene_matrix_init_from_float (&color_matrix, float_arr);
          graphene_vec4_t color_offset;
          graphene_vec4_init (&color_offset, c2.red, c2.green, c2.blue, 0);

          gtk_snapshot_push_color_matrix (
            snapshot, &color_matrix, &color_offset);

          const float hspacing = 2.f;
          const float vspacing = (float) (height - icon_texture_size) / 2.f;
          {
            graphene_rect_t tmp_r = Z_GRAPHENE_RECT_INIT (
              hspacing, vspacing, icon_texture_size, icon_texture_size);
            gtk_snapshot_append_texture (snapshot, self->track_icon, &tmp_r);
          }

          gtk_snapshot_pop (snapshot);
        }
    }
}

static void
multipress_pressed (
  GtkGestureClick * gesture,
  gint              n_press,
  gdouble           x,
  gdouble           y,
  ColorAreaWidget * self)
{
  if (n_press == 1 && self->track)
    {
      object_color_chooser_dialog_widget_run (
        GTK_WINDOW (MAIN_WINDOW), self->track, NULL, NULL);
    }
  gtk_widget_queue_draw (GTK_WIDGET (self));
}

static void
on_enter (
  GtkEventControllerMotion * motion_controller,
  gdouble                    x,
  gdouble                    y,
  gpointer                   user_data)
{
  ColorAreaWidget * self = Z_COLOR_AREA_WIDGET (user_data);
  self->hovered = true;
  gtk_widget_queue_draw (GTK_WIDGET (self));
}

static void
on_leave (GtkEventControllerMotion * motion_controller, gpointer user_data)
{
  ColorAreaWidget * self = Z_COLOR_AREA_WIDGET (user_data);
  self->hovered = false;
  gtk_widget_queue_draw (GTK_WIDGET (self));
}

/**
 * Creates a generic color widget using the given
 * color pointer.
 */
void
color_area_widget_setup_generic (ColorAreaWidget * self, GdkRGBA * color)
{
  self->color = *color;
  gtk_widget_queue_draw (GTK_WIDGET (self));
}

/**
 * Creates a ColorAreaWidget for use inside
 * TrackWidget implementations.
 */
void
color_area_widget_setup_track (ColorAreaWidget * self, Track * track)
{
  self->track = track;
  self->type = ColorAreaType::COLOR_AREA_TYPE_TRACK;

  /*g_debug ("setting up track %s for %p", track->name, self);*/
  gtk_widget_queue_draw (GTK_WIDGET (self));
}

/**
 * Changes the color.
 *
 * Track types don't need to do this since the
 * color is read directly from the Track.
 */
void
color_area_widget_set_color (ColorAreaWidget * self, GdkRGBA * color)
{
  self->color = *color;

  gtk_widget_queue_draw (GTK_WIDGET (self));
}

static void
finalize (ColorAreaWidget * self)
{
  object_free_w_func_and_null (g_ptr_array_unref, self->parents);
  object_free_w_func_and_null (g_object_unref, self->track_icon);
  object_free_w_func_and_null (g_free, self->last_track_icon_name);

  G_OBJECT_CLASS (color_area_widget_parent_class)->finalize (G_OBJECT (self));
}

static void
color_area_widget_init (ColorAreaWidget * self)
{
  gtk_widget_set_focusable (GTK_WIDGET (self), true);
  gtk_widget_set_overflow (GTK_WIDGET (self), GTK_OVERFLOW_HIDDEN);

  gtk_accessible_update_property (
    GTK_ACCESSIBLE (self), GTK_ACCESSIBLE_PROPERTY_LABEL, "Color", -1);

  GtkGestureClick * mp = GTK_GESTURE_CLICK (gtk_gesture_click_new ());
  g_signal_connect (
    G_OBJECT (mp), "pressed", G_CALLBACK (multipress_pressed), self);
  gtk_widget_add_controller (GTK_WIDGET (self), GTK_EVENT_CONTROLLER (mp));

  GtkEventControllerMotion * motion_controller =
    GTK_EVENT_CONTROLLER_MOTION (gtk_event_controller_motion_new ());
  g_signal_connect (
    G_OBJECT (motion_controller), "enter", G_CALLBACK (on_enter), self);
  g_signal_connect (
    G_OBJECT (motion_controller), "leave", G_CALLBACK (on_leave), self);
  gtk_widget_add_controller (
    GTK_WIDGET (self), GTK_EVENT_CONTROLLER (motion_controller));
}

static void
color_area_widget_class_init (ColorAreaWidgetClass * klass)
{
  GtkWidgetClass * wklass = GTK_WIDGET_CLASS (klass);
  wklass->snapshot = color_area_snapshot;
  gtk_widget_class_set_css_name (wklass, "color-area");
  gtk_widget_class_set_accessible_role (wklass, GTK_ACCESSIBLE_ROLE_BUTTON);

  GObjectClass * oklass = G_OBJECT_CLASS (klass);
  oklass->finalize = (GObjectFinalizeFunc) finalize;
}
