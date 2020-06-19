/*
 * Copyright (C) 2019-2020 Alexandros Theodotou <alex at zrythm dot org>
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

#include "audio/control_room.h"
#include "audio/engine.h"
#include "audio/fader.h"
#include "gui/widgets/control_room.h"
#include "gui/widgets/knob.h"
#include "gui/widgets/knob_with_name.h"
#include "gui/widgets/slider_bar.h"
#include "project.h"
#include "utils/resources.h"
#include "zrythm_app.h"

#include <glib/gi18n.h>

G_DEFINE_TYPE (ControlRoomWidget,
               control_room_widget,
               GTK_TYPE_GRID)

void
control_room_widget_setup (
  ControlRoomWidget * self,
  ControlRoom *       control_room)
{
  self->control_room = control_room;

  self->listen_dim_slider =
    slider_bar_widget_new_simple (
      fader_get_amp,
      fader_set_amp,
      &control_room->listen_vol_fader,
      0.f, 2.f, -1, -1, 0.f,
      _("Listen dim level"));
  gtk_widget_set_tooltip_text (
    GTK_WIDGET (self->listen_dim_slider),
    _("The level to set other channels to when "
      "Listen is enabled on a channel"));
  gtk_widget_set_visible (
    GTK_WIDGET (self->listen_dim_slider), 1);
  /* FIXME doesn't work for some reason */
  /*gtk_container_add (*/
    /*GTK_CONTAINER (*/
      /*self->listen_dim_slider_placeholder),*/
    /*GTK_WIDGET (self->listen_dim_slider));*/

  KnobWidget * knob =
    knob_widget_new_simple (
      fader_get_fader_val,
      fader_set_fader_val,
      MONITOR_FADER,
      0.f, 1.f, 78, 0.f);
  self->volume =
    knob_with_name_widget_new (
      _("Monitor out"),
      knob);
  gtk_container_add (
    GTK_CONTAINER (self->main_knob_placeholder),
    GTK_WIDGET (self->volume));
}

/**
 * Creates a ControlRoomWidget.
 */
ControlRoomWidget *
control_room_widget_new (void)
{
  ControlRoomWidget * self =
    g_object_new (CONTROL_ROOM_WIDGET_TYPE, NULL);

  return self;
}

static void
control_room_widget_init (
  ControlRoomWidget * self)
{
  gtk_widget_init_template (GTK_WIDGET (self));
}

static void
control_room_widget_class_init (
  ControlRoomWidgetClass * _klass)
{
  GtkWidgetClass * klass = GTK_WIDGET_CLASS (_klass);
  resources_set_class_template (
    klass, "control_room.ui");

  gtk_widget_class_set_css_name (
    klass, "control-room");

  gtk_widget_class_bind_template_child (
    klass,
    ControlRoomWidget,
    listen_dim_slider_placeholder);
  gtk_widget_class_bind_template_child (
    klass,
    ControlRoomWidget,
    main_knob_placeholder);
  gtk_widget_class_bind_template_child (
    klass,
    ControlRoomWidget,
    dim_output);
  gtk_widget_class_bind_template_child (
    klass,
    ControlRoomWidget,
    left_of_main_knob_toolbar);
}
