/*
 * Copyright (C) 2018-2019 Alexandros Theodotou <alex at zrythm dot org>
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

#include "audio/engine.h"
#include "audio/transport.h"
#include "gui/widgets/cpu.h"
#include "gui/widgets/digital_meter.h"
#include "gui/widgets/top_bar.h"
#include "gui/widgets/transport_controls.h"
#include "project.h"
#include "utils/gtk.h"
#include "utils/resources.h"

#include <gtk/gtk.h>

G_DEFINE_TYPE (TopBarWidget,
               top_bar_widget,
               GTK_TYPE_BOX)

void
top_bar_widget_refresh (TopBarWidget * self)
{
  if (self->digital_transport)
    gtk_widget_destroy (
      GTK_WIDGET (self->digital_transport));
  self->digital_transport =
    digital_meter_widget_new_for_position (
      TRANSPORT,
      transport_get_playhead_pos,
      transport_set_playhead_pos);
  gtk_container_add (
    GTK_CONTAINER (self->digital_meters),
    GTK_WIDGET (self->digital_transport));
}

static void
top_bar_widget_class_init (TopBarWidgetClass * _klass)
{
  GtkWidgetClass * klass = GTK_WIDGET_CLASS (_klass);
  resources_set_class_template (klass,
                                "top_bar.ui");

  gtk_widget_class_set_css_name (klass,
                                 "top-bar");

  gtk_widget_class_bind_template_child (
    klass,
    TopBarWidget,
    digital_meters);
  gtk_widget_class_bind_template_child (
    klass,
    TopBarWidget,
    transport_controls);
  gtk_widget_class_bind_template_child (
    klass,
    TopBarWidget,
    cpu_load);
}

static void
top_bar_widget_init (TopBarWidget * self)
{
  gtk_widget_destroy (
    GTK_WIDGET (g_object_new (
      DIGITAL_METER_WIDGET_TYPE, NULL)));
  gtk_widget_destroy (
    GTK_WIDGET (g_object_new (
      TRANSPORT_CONTROLS_WIDGET_TYPE, NULL)));
  gtk_widget_destroy (
    GTK_WIDGET (g_object_new (
      CPU_WIDGET_TYPE, NULL)));

  gtk_widget_init_template (GTK_WIDGET (self));

  /* setup digital meters */
  self->digital_bpm =
    digital_meter_widget_new (
      DIGITAL_METER_TYPE_BPM,
      NULL,
      NULL);
  self->digital_timesig =
    digital_meter_widget_new (
      DIGITAL_METER_TYPE_TIMESIG,
      NULL,
      NULL);
  gtk_container_add (
    GTK_CONTAINER (self->digital_meters),
    GTK_WIDGET (self->digital_bpm));
  gtk_container_add (
    GTK_CONTAINER (self->digital_meters),
    GTK_WIDGET (self->digital_timesig));
  gtk_widget_show_all (
    GTK_WIDGET (self->digital_meters));
  gtk_widget_show_all (GTK_WIDGET (self));

  cpu_widget_setup (
    self->cpu_load);
}
