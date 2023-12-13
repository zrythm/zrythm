// clang-format off
// SPDX-FileCopyrightText: © 2020-2021, 2023 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense
// clang-format on

#include "gui/widgets/main_window.h"
#include "gui/widgets/playhead_scroll_buttons.h"
#include "project.h"
#include "utils/resources.h"

G_DEFINE_TYPE (
  PlayheadScrollButtonsWidget,
  playhead_scroll_buttons_widget,
  GTK_TYPE_WIDGET)

static void
on_dispose (GObject * object)
{
  PlayheadScrollButtonsWidget * self = Z_PLAYHEAD_SCROLL_BUTTONS_WIDGET (object);

  gtk_widget_unparent (GTK_WIDGET (self->scroll_edges));
  gtk_widget_unparent (GTK_WIDGET (self->follow));

  G_OBJECT_CLASS (playhead_scroll_buttons_widget_parent_class)->dispose (object);
}

static void
playhead_scroll_buttons_widget_class_init (
  PlayheadScrollButtonsWidgetClass * _klass)
{
  GtkWidgetClass * klass = GTK_WIDGET_CLASS (_klass);
  resources_set_class_template (klass, "playhead_scroll_buttons.ui");
  gtk_widget_class_set_css_name (klass, "playhead-scroll-buttons");
  gtk_widget_class_set_accessible_role (klass, GTK_ACCESSIBLE_ROLE_GROUP);

#define BIND_CHILD(x) \
  gtk_widget_class_bind_template_child (klass, PlayheadScrollButtonsWidget, x)

  BIND_CHILD (scroll_edges);
  BIND_CHILD (follow);

#undef BIND_CHILD

  GObjectClass * oklass = G_OBJECT_CLASS (_klass);
  oklass->dispose = on_dispose;
}

static void
playhead_scroll_buttons_widget_init (PlayheadScrollButtonsWidget * self)
{
  gtk_widget_init_template (GTK_WIDGET (self));

  GtkBoxLayout * box_layout =
    GTK_BOX_LAYOUT (gtk_box_layout_new (GTK_ORIENTATION_HORIZONTAL));
  gtk_box_layout_set_spacing (box_layout, 1);
  gtk_widget_set_layout_manager (
    GTK_WIDGET (self), GTK_LAYOUT_MANAGER (box_layout));
}
