// SPDX-FileCopyrightText: © 2020-2021 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * \file
 *
 * PluginProperties expander widget.
 */

#ifndef __GUI_WIDGETS_PLUGIN_PROPERTIES_EXPANDER_H__
#define __GUI_WIDGETS_PLUGIN_PROPERTIES_EXPANDER_H__

#include "dsp/port.h"
#include "gui/widgets/two_col_expander_box.h"

#include "gtk_wrapper.h"

typedef struct _EditableLabelWidget        EditableLabelWidget;
typedef struct _PluginPresetSelectorWidget PluginPresetSelectorWidget;
typedef struct Track                       Track;
typedef struct Plugin                      Plugin;

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
