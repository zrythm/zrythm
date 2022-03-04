/*
 * Copyright (C) 2019-2022 Alexandros Theodotou <alex at zrythm dot org>
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

/**
 * \file
 *
 */

#include "gui/widgets/main_window.h"
#include "gui/widgets/snap_box.h"
#include "gui/widgets/snap_grid.h"
#include "project.h"
#include "utils/resources.h"

G_DEFINE_TYPE (
  SnapBoxWidget, snap_box_widget,
  GTK_TYPE_BOX)

/**
 * Sets the snap_box toggled states after
 * deactivating the callbacks.
 */
void
snap_box_widget_refresh (
  SnapBoxWidget * self)
{
  gtk_widget_set_sensitive (
    GTK_WIDGET (self->snap_to_grid_keep_offset),
    self->snap_grid->snap_grid->snap_to_grid);

  gtk_toggle_button_set_active (
    self->snap_to_grid,
    self->snap_grid->snap_grid->snap_to_grid);
  gtk_toggle_button_set_active (
    self->snap_to_grid_keep_offset,
    self->snap_grid->snap_grid->snap_to_grid_keep_offset);
  gtk_toggle_button_set_active (
    self->snap_to_events,
    self->snap_grid->snap_grid->snap_to_events);
}

void
snap_box_widget_setup (
  SnapBoxWidget * self,
  SnapGrid *      sg)
{
  snap_grid_widget_setup (
    self->snap_grid, sg);

  gtk_actionable_set_action_name (
    GTK_ACTIONABLE (self->snap_to_grid), NULL);
  gtk_actionable_set_action_name (
    GTK_ACTIONABLE (self->snap_to_grid_keep_offset), NULL);
  gtk_actionable_set_action_name (
    GTK_ACTIONABLE (self->snap_to_events), NULL);

#if 0
  gtk_toggle_button_set_active (
    self->snap_to_grid,
    sg->snap_to_grid);
  gtk_toggle_button_set_active (
    self->snap_to_grid_keep_offset,
    sg->snap_to_grid_keep_offset);
  gtk_toggle_button_set_active (
    self->snap_to_events,
    sg->snap_to_events);
#endif

  snap_box_widget_refresh (self);

  gtk_actionable_set_action_name (
    GTK_ACTIONABLE (self->snap_to_grid),
    "app.snap-to-grid");
  gtk_actionable_set_action_name (
    GTK_ACTIONABLE (self->snap_to_grid_keep_offset),
    "app.snap-keep-offset");
  gtk_actionable_set_action_name (
    GTK_ACTIONABLE (self->snap_to_events),
    "app.snap-events");

  if (sg == SNAP_GRID_TIMELINE)
    {
      gtk_actionable_set_action_target (
        GTK_ACTIONABLE (self->snap_to_grid),
        "s", "timeline");
      gtk_actionable_set_action_target (
        GTK_ACTIONABLE (self->snap_to_grid_keep_offset),
        "s", "timeline");
      gtk_actionable_set_action_target (
        GTK_ACTIONABLE (self->snap_to_events),
        "s", "timeline");
    }
  else if (sg == SNAP_GRID_EDITOR)
    {
      gtk_actionable_set_action_target (
        GTK_ACTIONABLE (self->snap_to_grid),
        "s", "editor");
      gtk_actionable_set_action_target (
        GTK_ACTIONABLE (self->snap_to_grid_keep_offset),
        "s", "editor");
      gtk_actionable_set_action_target (
        GTK_ACTIONABLE (self->snap_to_events),
        "s", "editor");
    }

  snap_box_widget_refresh (self);
}

static void
snap_box_widget_class_init (
  SnapBoxWidgetClass * _klass)
{
  GtkWidgetClass * klass =
    GTK_WIDGET_CLASS (_klass);
  resources_set_class_template (
    klass, "snap_box.ui");
  gtk_widget_class_set_css_name (
    klass, "snap-box");

#define BIND_CHILD(x) \
  gtk_widget_class_bind_template_child ( \
    klass, SnapBoxWidget, x)

  BIND_CHILD (snap_to_grid);
  BIND_CHILD (snap_to_grid_keep_offset);
  BIND_CHILD (snap_to_events);
  BIND_CHILD (snap_grid);

#undef BIND_CHILD
}

static void
snap_box_widget_init (SnapBoxWidget * self)
{
  gtk_widget_init_template (GTK_WIDGET (self));
}
