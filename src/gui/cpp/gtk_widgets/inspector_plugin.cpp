// SPDX-FileCopyrightText: © 2019-2022, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "common/plugins/plugin.h"
#include "common/utils/logger.h"
#include "common/utils/resources.h"
#include "gui/cpp/backend/mixer_selections.h"
#include "gui/cpp/backend/project.h"
#include "gui/cpp/backend/tracklist_selections.h"
#include "gui/cpp/backend/zrythm.h"
#include "gui/cpp/gtk_widgets/color_area.h"
#include "gui/cpp/gtk_widgets/gtk_wrapper.h"
#include "gui/cpp/gtk_widgets/inspector_plugin.h"
#include "gui/cpp/gtk_widgets/left_dock_edge.h"
#include "gui/cpp/gtk_widgets/plugin_properties_expander.h"
#include "gui/cpp/gtk_widgets/ports_expander.h"
#include "gui/cpp/gtk_widgets/zrythm_app.h"

#include <glib/gi18n.h>

G_DEFINE_TYPE (InspectorPluginWidget, inspector_plugin_widget, GTK_TYPE_BOX)

static void
setup_color (InspectorPluginWidget * self, Track * track)
{
  if (track)
    {
      color_area_widget_setup_track (self->color, track);
    }
  else
    {
      Color color = { 1, 1, 1, 1 };
      color_area_widget_set_color (self->color, color);
    }
}

void
inspector_plugin_widget_show (
  InspectorPluginWidget *  self,
  ProjectMixerSelections * ms,
  bool                     set_notebook_page)
{
  z_debug (
    "showing plugin inspector contents (set notebook page: %d)...",
    set_notebook_page);

  if (set_notebook_page)
    {
      PanelWidget * panel_widget = PANEL_WIDGET (
        gtk_widget_get_ancestor (GTK_WIDGET (self), PANEL_TYPE_WIDGET));
      z_return_if_fail (panel_widget);
      panel_widget_raise (panel_widget);
      z_debug ("raised plugin inspector");
    }

  /* show info for first selected plugin, or first plugin in
   * the selected track */
  Plugin * pl = NULL;
  if (ms->has_any_)
    {
      pl = ms->get_first_plugin ();
    }
  else
    {
      auto tr = dynamic_cast<ChannelTrack *> (
        TRACKLIST_SELECTIONS->get_highest_track ());
      if (tr)
        {
          auto                  ch = tr->get_channel ();
          std::vector<Plugin *> pls;
          ch->get_plugins (pls);
          if (!pls.empty ())
            {
              pl = pls.front ();
              z_debug ("showing info for plugin {}", pl->get_name ());
            }
        }
    }

  if (pl)
    {
      setup_color (self, pl->get_track ());
    }
  else
    {
      setup_color (self, nullptr);
    }

  ports_expander_widget_setup_plugin (
    self->ctrl_ins, PortFlow::Input, PortType::Control, pl);
  ports_expander_widget_setup_plugin (
    self->ctrl_outs, PortFlow::Output, PortType::Control, pl);
  ports_expander_widget_setup_plugin (
    self->midi_ins, PortFlow::Input, PortType::Event, pl);
  ports_expander_widget_setup_plugin (
    self->midi_outs, PortFlow::Output, PortType::Event, pl);
  ports_expander_widget_setup_plugin (
    self->audio_ins, PortFlow::Input, PortType::Audio, pl);
  ports_expander_widget_setup_plugin (
    self->audio_outs, PortFlow::Output, PortType::Audio, pl);
  ports_expander_widget_setup_plugin (
    self->cv_ins, PortFlow::Input, PortType::CV, pl);
  ports_expander_widget_setup_plugin (
    self->cv_outs, PortFlow::Output, PortType::CV, pl);

  plugin_properties_expander_widget_refresh (self->properties, pl);
}

InspectorPluginWidget *
inspector_plugin_widget_new (void)
{
  InspectorPluginWidget * self = Z_INSPECTOR_PLUGIN_WIDGET (
    g_object_new (INSPECTOR_PLUGIN_WIDGET_TYPE, nullptr));

  plugin_properties_expander_widget_setup (self->properties, nullptr);

  return self;
}

static void
inspector_plugin_widget_class_init (InspectorPluginWidgetClass * _klass)
{
  GtkWidgetClass * klass = GTK_WIDGET_CLASS (_klass);
  resources_set_class_template (klass, "inspector_plugin.ui");

#define BIND_CHILD(child) \
  gtk_widget_class_bind_template_child ( \
    GTK_WIDGET_CLASS (klass), InspectorPluginWidget, child);

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
inspector_plugin_widget_init (InspectorPluginWidget * self)
{
  g_type_ensure (PLUGIN_PROPERTIES_EXPANDER_WIDGET_TYPE);
  g_type_ensure (PORTS_EXPANDER_WIDGET_TYPE);

  gtk_widget_init_template (GTK_WIDGET (self));

  gtk_widget_add_css_class (GTK_WIDGET (self), "inspector");
}
