/*
 * Copyright (C) 2019-2021 Alexandros Theodotou <alex at zrythm dot org>
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
#include "gui/widgets/center_dock.h"
#include "gui/widgets/color_area.h"
#include "gui/widgets/inspector_plugin.h"
#include "gui/widgets/left_dock_edge.h"
#include "gui/widgets/main_window.h"
#include "gui/widgets/plugin_properties_expander.h"
#include "gui/widgets/ports_expander.h"
#include "plugins/plugin.h"
#include "utils/gtk.h"
#include "utils/resources.h"
#include "zrythm_app.h"

#include <glib/gi18n.h>
#include <gtk/gtk.h>

G_DEFINE_TYPE (
  InspectorPluginWidget,
  inspector_plugin_widget,
  GTK_TYPE_BOX)

static void
setup_color (
  InspectorPluginWidget * self,
  Track *                 track)
{
  if (track)
    {
      color_area_widget_setup_track (
        self->color, track);
    }
  else
    {
      GdkRGBA color = { 1, 1, 1, 1 };
      color_area_widget_set_color (
        self->color, &color);
    }
}

/**
 * Shows the inspector page for the given mixer
 * selection (plugin).
 *
 * @param set_notebook_page Whether to set the
 *   current left panel tab to the plugin page.
 */
void
inspector_plugin_widget_show (
  InspectorPluginWidget * self,
  MixerSelections *       ms,
  bool                    set_notebook_page)
{
  g_debug ("showing plugin inspector contents...");

  if (set_notebook_page)
    {
      gtk_notebook_set_current_page (
        GTK_NOTEBOOK (
          MW_LEFT_DOCK_EDGE->inspector_notebook),
        1);
    }

  /* show info for first plugin */
  Plugin * pl = NULL;
  if (ms->has_any)
    {
      pl = mixer_selections_get_first_plugin (ms);
    }

  if (pl)
    {
      setup_color (self, plugin_get_track (pl));
    }
  else
    {
      setup_color (self, NULL);
    }

  ports_expander_widget_setup_plugin (
    self->ctrl_ins, FLOW_INPUT, TYPE_CONTROL, pl);
  ports_expander_widget_setup_plugin (
    self->ctrl_outs, FLOW_OUTPUT, TYPE_CONTROL, pl);
  ports_expander_widget_setup_plugin (
    self->midi_ins, FLOW_INPUT, TYPE_EVENT, pl);
  ports_expander_widget_setup_plugin (
    self->midi_outs, FLOW_OUTPUT, TYPE_EVENT, pl);
  ports_expander_widget_setup_plugin (
    self->audio_ins, FLOW_INPUT, TYPE_AUDIO, pl);
  ports_expander_widget_setup_plugin (
    self->audio_outs, FLOW_OUTPUT, TYPE_AUDIO, pl);
  ports_expander_widget_setup_plugin (
    self->cv_ins, FLOW_INPUT, TYPE_CV, pl);
  ports_expander_widget_setup_plugin (
    self->cv_outs, FLOW_OUTPUT, TYPE_CV, pl);

  plugin_properties_expander_widget_refresh (
    self->properties, pl);
}

InspectorPluginWidget *
inspector_plugin_widget_new (void)
{
  InspectorPluginWidget * self = g_object_new (
    INSPECTOR_PLUGIN_WIDGET_TYPE, NULL);

  plugin_properties_expander_widget_setup (
    self->properties, NULL);

  return self;
}

static void
inspector_plugin_widget_class_init (
  InspectorPluginWidgetClass * _klass)
{
  GtkWidgetClass * klass = GTK_WIDGET_CLASS (_klass);
  resources_set_class_template (
    klass, "inspector_plugin.ui");

#define BIND_CHILD(child) \
  gtk_widget_class_bind_template_child ( \
    GTK_WIDGET_CLASS (klass), \
    InspectorPluginWidget, child);

  BIND_CHILD (color);
  BIND_CHILD (properties);
  BIND_CHILD (ctrl_ins);
  BIND_CHILD (ctrl_outs);
  BIND_CHILD (audio_ins);
  BIND_CHILD (audio_outs);
  BIND_CHILD (midi_ins);
  BIND_CHILD (midi_outs);
  BIND_CHILD (cv_ins);
  BIND_CHILD (cv_outs);

#undef BIND_CHILD
}

static void
inspector_plugin_widget_init (
  InspectorPluginWidget * self)
{
  g_type_ensure (
    PLUGIN_PROPERTIES_EXPANDER_WIDGET_TYPE);
  g_type_ensure (PORTS_EXPANDER_WIDGET_TYPE);

  gtk_widget_init_template (GTK_WIDGET (self));

  gtk_widget_add_css_class (
    GTK_WIDGET (self), "inspector");
}
