/*
 * Copyright (C) 2020-2021 Alexandros Theodotou <alex at zrythm dot org>
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
#include "gui/widgets/range_action_buttons.h"
#include "project.h"
#include "utils/resources.h"

G_DEFINE_TYPE (
  RangeActionButtonsWidget,
  range_action_buttons_widget,
  GTK_TYPE_BOX)

static void
range_action_buttons_widget_class_init (
  RangeActionButtonsWidgetClass * _klass)
{
  GtkWidgetClass * klass = GTK_WIDGET_CLASS (_klass);
  resources_set_class_template (
    klass, "range_action_buttons.ui");
  gtk_widget_class_set_css_name (
    klass, "range-action-buttons");

#define BIND_CHILD(x) \
  gtk_widget_class_bind_template_child ( \
    klass, RangeActionButtonsWidget, x)

  BIND_CHILD (insert_silence);
  BIND_CHILD (remove);

#undef BIND_CHILD
}

static void
range_action_buttons_widget_init (
  RangeActionButtonsWidget * self)
{
  gtk_widget_init_template (GTK_WIDGET (self));
}
