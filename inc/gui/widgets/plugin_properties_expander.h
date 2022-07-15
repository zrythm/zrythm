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
 * PluginProperties expander widget.
 */

#ifndef __GUI_WIDGETS_PLUGIN_PROPERTIES_EXPANDER_H__
#define __GUI_WIDGETS_PLUGIN_PROPERTIES_EXPANDER_H__

#include "audio/port.h"
#include "gui/widgets/two_col_expander_box.h"

#include <gtk/gtk.h>

typedef struct _EditableLabelWidget EditableLabelWidget;
typedef struct _PluginPresetSelectorWidget
                      PluginPresetSelectorWidget;
typedef struct Track  Track;
typedef struct Plugin Plugin;

#define PLUGIN_PROPERTIES_EXPANDER_WIDGET_TYPE \
  (plugin_properties_expander_widget_get_type ())
G_DECLARE_FINAL_TYPE (
  PluginPropertiesExpanderWidget,
  plugin_properties_expander_widget,
  Z,
  PLUGIN_PROPERTIES_EXPANDER_WIDGET,
  TwoColExpanderBoxWidget);

/**
 * @addtogroup widgets
 *
 * @{
 */

/**
 * A widget for selecting plugin_properties in the plugin
 * inspector.
 */
typedef struct _PluginPropertiesExpanderWidget
{
  TwoColExpanderBoxWidget parent_instance;

  /** Plugin name. */
  GtkLabel * name;

  /** Plugin class. */
  GtkLabel * type;

  GtkComboBoxText * banks;
  GtkListBox *      presets;

  GtkButton * save_preset_btn;
  GtkButton * load_preset_btn;

  /** Currently selected plugin. */
  Plugin * plugin;

  gulong bank_changed_handler;
  gulong pset_changed_handler;
} PluginPropertiesExpanderWidget;

/**
 * Refreshes each field.
 */
void
plugin_properties_expander_widget_refresh (
  PluginPropertiesExpanderWidget * self,
  Plugin *                         pl);

/**
 * Sets up the PluginPropertiesExpanderWidget for a Plugin.
 */
void
plugin_properties_expander_widget_setup (
  PluginPropertiesExpanderWidget * self,
  Plugin *                         pl);

/**
 * @}
 */

#endif
