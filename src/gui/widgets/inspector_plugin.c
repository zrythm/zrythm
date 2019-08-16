/*
 * Copyright (C) 2019 Alexandros Theodotou <alex at zrythm dot org>
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

#include "gui/backend/mixer_selections.h"
#include "gui/widgets/inspector_plugin.h"
#include "gui/widgets/ports_expander.h"
#include "utils/resources.h"

#include <gtk/gtk.h>

#include <glib/gi18n.h>

G_DEFINE_TYPE (InspectorPluginWidget,
               inspector_plugin_widget,
               GTK_TYPE_BOX)

void
inspector_plugin_widget_show (
  InspectorPluginWidget * self,
  MixerSelections *       ms)
{
  /* show info for first track */
  if (ms->num_slots > 0)
    {
      Plugin * pl = ms->plugins[0];

      ports_expander_widget_setup_plugin (
        self->ctrl_ins,
        FLOW_INPUT,
        TYPE_CONTROL,
        pl);
      ports_expander_widget_setup_plugin (
        self->ctrl_outs,
        FLOW_OUTPUT,
        TYPE_CONTROL,
        pl);
      ports_expander_widget_setup_plugin (
        self->midi_ins,
        FLOW_INPUT,
        TYPE_EVENT,
        pl);
      ports_expander_widget_setup_plugin (
        self->midi_outs,
        FLOW_OUTPUT,
        TYPE_EVENT,
        pl);
      ports_expander_widget_setup_plugin (
        self->audio_ins,
        FLOW_INPUT,
        TYPE_AUDIO,
        pl);
      ports_expander_widget_setup_plugin (
        self->audio_outs,
        FLOW_OUTPUT,
        TYPE_AUDIO,
        pl);
      ports_expander_widget_setup_plugin (
        self->cv_ins,
        FLOW_INPUT,
        TYPE_CV,
        pl);
      ports_expander_widget_setup_plugin (
        self->cv_outs,
        FLOW_OUTPUT,
        TYPE_CV,
        pl);
    }
}

static void
inspector_plugin_widget_class_init (
  InspectorPluginWidgetClass * _klass)
{
  GtkWidgetClass * klass =
    GTK_WIDGET_CLASS (_klass);
  resources_set_class_template (
    klass, "inspector_plugin.ui");

#define BIND_CHILD(child) \
  gtk_widget_class_bind_template_child ( \
    GTK_WIDGET_CLASS (klass), \
    InspectorPluginWidget, \
    child);

  BIND_CHILD (ctrl_ins);
  gtk_widget_class_bind_template_child (
    GTK_WIDGET_CLASS (klass),
    InspectorPluginWidget,
    ctrl_outs);
  gtk_widget_class_bind_template_child (
    GTK_WIDGET_CLASS (klass),
    InspectorPluginWidget,
    audio_ins);
  gtk_widget_class_bind_template_child (
    GTK_WIDGET_CLASS (klass),
    InspectorPluginWidget,
    audio_outs);
  gtk_widget_class_bind_template_child (
    GTK_WIDGET_CLASS (klass),
    InspectorPluginWidget,
    midi_ins);
  gtk_widget_class_bind_template_child (
    GTK_WIDGET_CLASS (klass),
    InspectorPluginWidget,
    midi_outs);
  gtk_widget_class_bind_template_child (
    GTK_WIDGET_CLASS (klass),
    InspectorPluginWidget,
    cv_ins);
  gtk_widget_class_bind_template_child (
    GTK_WIDGET_CLASS (klass),
    InspectorPluginWidget,
    cv_outs);

#undef BIND_CHILD
}

static void
inspector_plugin_widget_init (
  InspectorPluginWidget * self)
{
  g_type_ensure (PORTS_EXPANDER_WIDGET_TYPE);

  gtk_widget_init_template (GTK_WIDGET (self));
}
