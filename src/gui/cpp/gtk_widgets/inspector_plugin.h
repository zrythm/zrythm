// SPDX-FileCopyrightText: © 2019, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef __GUI_WIDGETS_INSPECTOR_PLUGIN_H__
#define __GUI_WIDGETS_INSPECTOR_PLUGIN_H__

#include "gui/cpp/backend/mixer_selections.h"
#include "gui/cpp/gtk_widgets/gtk_wrapper.h"

#include "common/utils/types.h"

#define INSPECTOR_PLUGIN_WIDGET_TYPE (inspector_plugin_widget_get_type ())
G_DECLARE_FINAL_TYPE (
  InspectorPluginWidget,
  inspector_plugin_widget,
  Z,
  INSPECTOR_PLUGIN_WIDGET,
  GtkBox)

/**
 * @addtogroup widgets
 *
 * @{
 */

#define MW_PLUGIN_INSPECTOR MW_LEFT_DOCK_EDGE->plugin_inspector

TYPEDEF_STRUCT_UNDERSCORED (PortsExpanderWidget);
TYPEDEF_STRUCT_UNDERSCORED (PluginPropertiesExpanderWidget);
TYPEDEF_STRUCT_UNDERSCORED (ColorAreaWidget);
class MixerSelections;

/**
 * @brief Inspector section for plugins.
 *
 */
using InspectorPluginWidget = struct _InspectorPluginWidget
{
  GtkBox parent_instance;

  PluginPropertiesExpanderWidget * properties;
  PortsExpanderWidget *            ctrl_ins;
  PortsExpanderWidget *            ctrl_outs;
  PortsExpanderWidget *            audio_ins;
  PortsExpanderWidget *            audio_outs;
  PortsExpanderWidget *            midi_ins;
  PortsExpanderWidget *            midi_outs;
  PortsExpanderWidget *            cv_ins;
  PortsExpanderWidget *            cv_outs;

  ColorAreaWidget * color;
};

/**
 * Shows the inspector page for the given mixer
 * selection (plugin).
 *
 * @param set_notebook_page Whether to set the
 *   current left panel tab to the plugin page.
 */
void
inspector_plugin_widget_show (
  InspectorPluginWidget *  self,
  ProjectMixerSelections * ms,
  bool                     set_notebook_page);

InspectorPluginWidget *
inspector_plugin_widget_new (void);

/**
 * @}
 */

#endif
