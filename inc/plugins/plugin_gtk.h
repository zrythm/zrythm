// SPDX-FileCopyrightText: © 2020-2021, 2023 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * @file
 *
 * GTK window management for plugin UIs.
 */

#ifndef __PLUGINS_PLUGIN_GTK_H__
#define __PLUGINS_PLUGIN_GTK_H__

#include "gtk_wrapper.h"

class Plugin;
class ControlPort;

/**
 * @addtogroup plugins
 *
 * @{
 */

/**
 * Widget for a control.
 */
struct PluginGtkController
{
  GtkSpinButton * spin;
  GtkWidget *     control;

  /** Port this control is for. */
  ControlPort * port;

  /** Pointer back to plugin. */
  Plugin * plugin;

  /** Last set control value - used to avoid re-setting the
   * same value on float controls. */
  float last_set_control_val;
};

struct PluginGtkPresetRecord
{
  Plugin * plugin;
  /**
   * This will be a LilvNode * for LV2 and an absolute path for carla.
   */
  void * preset;
};

/**
 * Creates a new GtkWindow that will be used to
 * either wrap plugin UIs or create generic UIs in.
 */
void
plugin_gtk_create_window (Plugin * plugin);

/**
 * Opens the generic UI of the plugin.
 *
 * Assumes plugin_gtk_create_window() has been
 * called.
 */
void
plugin_gtk_open_generic_ui (Plugin * plugin, bool fire_events);

/**
 * Called on each GUI frame to update the GTK UI.
 *
 * @note This is a GSourceFunc.
 */
NONNULL int
plugin_gtk_update_plugin_ui (Plugin * pl);

/**
 * Closes the plugin's UI (either LV2 wrapped with
 * suil, generic or LV2 external).
 */
int
plugin_gtk_close_ui (Plugin * plugin);

void
plugin_gtk_set_window_title (Plugin * plugin, GtkWindow * window);

void
plugin_gtk_add_control_row (
  GtkWidget *           table,
  int                   row,
  const char *          name,
  PluginGtkController * controller);

void
plugin_gtk_on_save_preset_activate (GtkWidget * widget, Plugin * plugin);

gint
plugin_gtk_menu_cmp (gconstpointer a, gconstpointer b, gpointer data);

/**
 * Sets up the combo box with all the banks the
 * plugin has.
 *
 * @return Whether any banks were added.
 */
bool
plugin_gtk_setup_plugin_banks_combo_box (GtkComboBoxText * cb, Plugin * plugin);

/**
 * Sets up the combo box with all the presets the
 * plugin has in the given bank, or all the presets
 * if NULL is given.
 *
 * @return Whether any presets were added.
 */
bool
plugin_gtk_setup_plugin_presets_list_box (GtkListBox * box, Plugin * plugin);

/**
 * Creates a label for a control.
 *
 * TODO move to ui.
 *
 * @param title Whether this is a title text (makes
 *   it bold).
 * @param preformatted Whether the text is
 *   preformatted.
 */
GtkWidget *
plugin_gtk_new_label (
  const char * text,
  bool         title,
  bool         preformatted,
  float        xalign,
  float        yalign);

/**
 * Called when a property changed or when there is a
 * UI port event to set (update) the widget's value.
 */
void
plugin_gtk_generic_set_widget_value (
  Plugin *              pl,
  PluginGtkController * controller,
  float                 control);

#if 0
void
plugin_gtk_build_menu (
  Plugin *    plugin,
  GtkWidget * window,
  GtkWidget * vbox);
#endif

/**
 * @}
 */

#endif
