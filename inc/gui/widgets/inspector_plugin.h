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

/**
 * \file
 *
 * Inspector section for plugins.
 */

#ifndef __GUI_WIDGETS_INSPECTOR_PLUGIN_H__
#define __GUI_WIDGETS_INSPECTOR_PLUGIN_H__

#include <gtk/gtk.h>

#define INSPECTOR_PLUGIN_WIDGET_TYPE \
  (inspector_plugin_widget_get_type ())
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

#define MW_PLUGIN_INSPECTOR \
  MW_LEFT_DOCK_EDGE->plugin_inspector

typedef struct _PortsExpanderWidget
  PortsExpanderWidget;
typedef struct _PluginPropertiesExpanderWidget
  PluginPropertiesExpanderWidget;
typedef struct _ColorAreaWidget ColorAreaWidget;
typedef struct MixerSelections  MixerSelections;

typedef struct _InspectorPluginWidget
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
} InspectorPluginWidget;

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
  bool                    set_notebook_page);

InspectorPluginWidget *
inspector_plugin_widget_new (void);

/**
 * @}
 */

#endif
