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

#include "gui/widgets/center_dock.h"
#include "gui/widgets/left_dock_edge.h"
#include "gui/widgets/main_window.h"
#include "gui/widgets/timeline_bot_box.h"
#include "gui/widgets/timeline_minimap.h"
#include "utils/resources.h"

G_DEFINE_TYPE (
  TimelineBotBoxWidget,
  timeline_bot_box_widget,
  GTK_TYPE_BOX)

static void
timeline_bot_box_widget_init (
  TimelineBotBoxWidget * self)
{
  g_type_ensure (TIMELINE_MINIMAP_WIDGET_TYPE);

  gtk_widget_init_template (GTK_WIDGET (self));

  // set icons
  gtk_button_set_icon_name (
    self->instrument_add, "add");
}

static void
timeline_bot_box_widget_class_init (
  TimelineBotBoxWidgetClass * _klass)
{
  GtkWidgetClass * klass = GTK_WIDGET_CLASS (_klass);
  resources_set_class_template (
    klass, "timeline_bot_box.ui");

  gtk_widget_class_set_css_name (
    klass, "center-dock-bot-box");

#define BIND_CHILD(x) \
  gtk_widget_class_bind_template_child ( \
    klass, TimelineBotBoxWidget, x)

  BIND_CHILD (left_tb);
  BIND_CHILD (instrument_add);
  BIND_CHILD (timeline_minimap);

#undef BIND_CHILD
}
