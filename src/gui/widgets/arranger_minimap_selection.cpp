// clang-format off
// SPDX-FileCopyrightText: © 2019, 2021-2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense
// clang-format on

#include "gui/widgets/arranger_minimap.h"
#include "gui/widgets/arranger_minimap_selection.h"
#include "gui/widgets/bot_bar.h"
#include "gui/widgets/center_dock.h"
#include "gui/widgets/main_window.h"
#include "utils/gtk.h"

#include "gtk_wrapper.h"

G_DEFINE_TYPE (
  ArrangerMinimapSelectionWidget,
  arranger_minimap_selection_widget,
  GTK_TYPE_WIDGET)

#define PADDING 2

static void
arranger_minimap_selection_snapshot (GtkWidget * widget, GtkSnapshot * snapshot)
{
  int width = gtk_widget_get_width (widget);
  int height = gtk_widget_get_height (widget);

  GskRoundedRect  rounded_rect;
  graphene_rect_t graphene_rect = Z_GRAPHENE_RECT_INIT (
    0.f, PADDING, (float) width, (float) height - PADDING * 2.f);
  gsk_rounded_rect_init_from_rect (&rounded_rect, &graphene_rect, 0);
  const float border_width = 2.f;
  GdkRGBA     border_color = Z_GDK_RGBA_INIT (0.9, 0.9, 0.9, 0.9);
  float       border_widths[] = {
    border_width, border_width, border_width, border_width
  };
  GdkRGBA border_colors[] = {
    border_color, border_color, border_color, border_color
  };
  GdkRGBA inside_color = {
    border_color.red / 3.f,
    border_color.green / 3.f,
    border_color.blue / 3.f,
    border_color.alpha / 3.f,
  };

  gtk_snapshot_append_color (snapshot, &inside_color, &graphene_rect);
  gtk_snapshot_append_border (
    snapshot, &rounded_rect, border_widths, border_colors);
}

static void
on_leave (GtkEventControllerMotion * motion_controller, gpointer user_data)
{
  ArrangerMinimapSelectionWidget * self =
    Z_ARRANGER_MINIMAP_SELECTION_WIDGET (user_data);

  gtk_widget_unset_state_flags (GTK_WIDGET (self), GTK_STATE_FLAG_PRELIGHT);
}

static void
on_motion (
  GtkEventControllerMotion * motion_controller,
  double                     x,
  double                     y,
  gpointer                   user_data)
{
  ArrangerMinimapSelectionWidget * self =
    Z_ARRANGER_MINIMAP_SELECTION_WIDGET (user_data);
  GtkWidget * widget = GTK_WIDGET (self);
  int         width = gtk_widget_get_width (widget);

  gtk_widget_set_state_flags (GTK_WIDGET (self), GTK_STATE_FLAG_PRELIGHT, 0);
  if (x < UI_RESIZE_CURSOR_SPACE)
    {
      self->cursor = UI_CURSOR_STATE_RESIZE_L;
      if (
        self->parent->action
        != ArrangerMinimapAction::ARRANGER_MINIMAP_ACTION_MOVING)
        ui_set_cursor_from_name (widget, "w-resize");
    }
  else if (x > width - UI_RESIZE_CURSOR_SPACE)
    {
      self->cursor = UI_CURSOR_STATE_RESIZE_R;
      if (
        self->parent->action
        != ArrangerMinimapAction::ARRANGER_MINIMAP_ACTION_MOVING)
        ui_set_cursor_from_name (widget, "e-resize");
    }
  else
    {
      self->cursor = UI_CURSOR_STATE_DEFAULT;
      if (
        self->parent->action
          != ArrangerMinimapAction::ARRANGER_MINIMAP_ACTION_MOVING
        && self->parent->action
             != ArrangerMinimapAction::ARRANGER_MINIMAP_ACTION_STARTING_MOVING
        && self->parent->action
             != ArrangerMinimapAction::ARRANGER_MINIMAP_ACTION_RESIZING_L
        && self->parent->action
             != ArrangerMinimapAction::ARRANGER_MINIMAP_ACTION_RESIZING_R)
        {
          ui_set_cursor_from_name (widget, "default");
        }
    }
}

ArrangerMinimapSelectionWidget *
arranger_minimap_selection_widget_new (ArrangerMinimapWidget * parent)
{
  ArrangerMinimapSelectionWidget * self =
    static_cast<ArrangerMinimapSelectionWidget *> (
      g_object_new (ARRANGER_MINIMAP_SELECTION_WIDGET_TYPE, NULL));

  self->parent = parent;

  return self;
}

static void
arranger_minimap_selection_widget_class_init (
  ArrangerMinimapSelectionWidgetClass * klass)
{
  GtkWidgetClass * wklass = GTK_WIDGET_CLASS (klass);
  wklass->snapshot = arranger_minimap_selection_snapshot;
  gtk_widget_class_set_css_name (wklass, "arranger-minimap-selection");
  gtk_widget_class_set_layout_manager_type (wklass, GTK_TYPE_BIN_LAYOUT);
}

static void
arranger_minimap_selection_widget_init (ArrangerMinimapSelectionWidget * self)
{
  gtk_widget_set_focusable (GTK_WIDGET (self), true);

  gtk_accessible_update_property (
    GTK_ACCESSIBLE (self), GTK_ACCESSIBLE_PROPERTY_LABEL,
    "Arranger Minimap Selection", -1);

  GtkEventController * motion_controller = gtk_event_controller_motion_new ();
  gtk_widget_add_controller (GTK_WIDGET (self), motion_controller);
  g_signal_connect (
    G_OBJECT (motion_controller), "motion", G_CALLBACK (on_motion), self);
  g_signal_connect (
    G_OBJECT (motion_controller), "leave", G_CALLBACK (on_leave), self);
}
