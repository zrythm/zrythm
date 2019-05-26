/*
 * Copyright (C) 2019 Alexandros Theodotou <alex at zrythm dot org>
 *
 * This file is part of Zrythm
 *
 * Zrythm is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Zrythm is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Zrythm.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "gui/widgets/control_room.h"
#include "utils/resources.h"

#include <glib/gi18n.h>

G_DEFINE_TYPE (ControlRoomWidget,
               control_room_widget,
               GTK_TYPE_GRID)

static void
control_room_widget_init (ControlRoomWidget * self)
{
  gtk_widget_init_template (GTK_WIDGET (self));
}

static void
control_room_widget_class_init (ControlRoomWidgetClass * _klass)
{
  GtkWidgetClass * klass = GTK_WIDGET_CLASS (_klass);
  resources_set_class_template (
    klass, "control_room.ui");

  gtk_widget_class_set_css_name (
    klass, "control-room");

  /*gtk_widget_class_bind_template_child (*/
    /*klass,*/
    /*ControlRoomWidget,*/
    /*left_panel);*/
}

