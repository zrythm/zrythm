// SPDX-FileCopyrightText: © 2020-2023 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense
/*
 * This file incorporates work covered by the following copyright and
 * permission notice:
 *
 * ---
 *
  Copyright 2007-2016 David Robillard <http://drobilla.net>

  Permission to use, copy, modify, and/or distribute this software for any
  purpose with or without fee is hereby granted, provided that the above
  copyright notice and this permission notice appear in all copies.

  THIS SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
  WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
  MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
  ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
  WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
  ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
  OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

  SPDX-License-Identifier: ISC

  ---
 */

#include "zrythm-config.h"

#include <math.h>

#include "dsp/engine.h"
#include "dsp/track.h"
#include "gui/backend/event.h"
#include "gui/backend/event_manager.h"
#include "gui/widgets/file_chooser_button.h"
#include "gui/widgets/main_window.h"
#include "plugins/carla_native_plugin.h"
#include "plugins/plugin.h"
#include "plugins/plugin_gtk.h"
#include "plugins/plugin_manager.h"
#include "project.h"
#include "settings/settings.h"
#include "utils/error.h"
#include "utils/flags.h"
#include "utils/gtk.h"
#include "utils/math.h"
#include "utils/objects.h"
#include "utils/string.h"
#include "zrythm.h"
#include "zrythm_app.h"

#include <glib/gi18n.h>

#define MIN_SCALE_WIDTH 120

#undef Bool

void
plugin_gtk_set_window_title (Plugin * plugin, GtkWindow * window)
{
  g_return_if_fail (plugin && plugin->setting->descr && window);

  char * title = plugin_generate_window_title (plugin);

  gtk_window_set_title (window, title);

  g_free (title);
}

void
plugin_gtk_on_save_preset_activate (GtkWidget * widget, Plugin * plugin)
{
  const PluginSetting *    setting = plugin->setting;
  const PluginDescriptor * descr = setting->descr;
  bool                     open_with_carla = setting->open_with_carla;

  GtkWidget * dialog = gtk_file_chooser_dialog_new (
    _ ("Save Preset"),
    plugin->window ? plugin->window : GTK_WINDOW (MAIN_WINDOW),
    GTK_FILE_CHOOSER_ACTION_SAVE, _ ("_Cancel"), GTK_RESPONSE_REJECT,
    _ ("_Save"), GTK_RESPONSE_ACCEPT, NULL);

  if (open_with_carla)
    {
#ifdef HAVE_CARLA
      char *  homedir = g_build_filename (g_get_home_dir (), NULL);
      GFile * homedir_file = g_file_new_for_path (homedir);
      gtk_file_chooser_set_current_folder (
        GTK_FILE_CHOOSER (dialog), homedir_file, NULL);
      g_object_unref (homedir_file);
      g_free (homedir);
#else
      g_return_if_reached ();
#endif
    }

  /* add additional inputs */
  GtkWidget * content = gtk_dialog_get_content_area (GTK_DIALOG (dialog));
  GtkWidget * add_prefix = add_prefix =
    gtk_check_button_new_with_mnemonic (_ ("_Prefix plugin name"));
  gtk_check_button_set_active (GTK_CHECK_BUTTON (add_prefix), TRUE);
  gtk_box_append (GTK_BOX (content), add_prefix);

  /*gtk_window_present (GTK_WINDOW (dialog));*/
  gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_ACCEPT);
  int ret = z_gtk_dialog_run (GTK_DIALOG (dialog), false);
  if (ret == GTK_RESPONSE_ACCEPT)
    {
      GFile * file = gtk_file_chooser_get_file (GTK_FILE_CHOOSER (dialog));
      char *  path = g_file_get_path (file);
      g_object_unref (file);
      bool add_prefix_active =
        gtk_check_button_get_active (GTK_CHECK_BUTTON (add_prefix));
      if (open_with_carla)
        {
#ifdef HAVE_CARLA
          const char * prefix = "";
          const char * sep = "";
          char *       dirname = g_path_get_dirname (path);
          char *       basename = g_path_get_basename (path);
          char *       sym = string_symbolify (basename);
          if (add_prefix_active)
            {
              prefix = descr->name;
              sep = "_";
            }
          char * sprefix = string_symbolify (prefix);
          char * bundle =
            g_strjoin (NULL, sprefix, sep, sym, ".preset.carla", NULL);
          char *   dir = g_build_filename (dirname, bundle, NULL);
          GError * err = NULL;
          bool     success =
            carla_native_plugin_save_state (plugin->carla, false, dir, &err);
          if (!success)
            {
              HANDLE_ERROR_LITERAL (err, "Failed to save Carla state");
            }
          g_free (dirname);
          g_free (basename);
          g_free (sym);
          g_free (sprefix);
          g_free (bundle);
          g_free (dir);
#endif
        }
      g_free (path);
    }

  gtk_window_destroy (GTK_WINDOW (dialog));

  EVENTS_PUSH (ET_PLUGIN_PRESET_SAVED, plugin);
}

/**
 * Creates a label for a control.
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
  float        yalign)
{
  GtkWidget * label = gtk_label_new (NULL);
#define PRINTF_FMT (title ? "<b>%s</b>" : "%s: ")
  gchar * str;
  if (preformatted)
    {
      str = g_strdup_printf (PRINTF_FMT, text);
    }
  else
    {
      str = g_markup_printf_escaped (PRINTF_FMT, text);
    }
#undef PRINTF_FMT
  gtk_label_set_markup (GTK_LABEL (label), str);
  g_free (str);
  gtk_label_set_xalign (GTK_LABEL (label), xalign);
  gtk_label_set_yalign (GTK_LABEL (label), yalign);
  return label;
}

void
plugin_gtk_add_control_row (
  GtkWidget *           table,
  int                   row,
  const char *          _name,
  PluginGtkController * controller)
{
  char name[600];
  strcpy (name, _name);
  bool preformatted = false;

  if (controller && controller->port)
    {
      PortIdentifier id = controller->port->id;

      if (id.unit > PORT_UNIT_NONE)
        {
          sprintf (
            name, "%s <small>(%s)</small>", _name,
            port_unit_strings[id.unit].str);
        }

      preformatted = true;
    }

  GtkWidget * label = plugin_gtk_new_label (name, false, preformatted, 1.0, 0.5);
  gtk_grid_attach (GTK_GRID (table), label, 0, row, 1, 1);
  int control_left_attach = 1;
  if (controller->spin)
    {
      control_left_attach = 2;
      gtk_grid_attach (
        GTK_GRID (table), GTK_WIDGET (controller->spin), 1, row, 1, 1);
    }
  gtk_grid_attach (
    GTK_GRID (table), controller->control, control_left_attach, row,
    3 - control_left_attach, 1);
}

/**
 * Called when the plugin window is destroyed.
 */
static void
on_window_destroy (GtkWidget * widget, Plugin * pl)
{
  g_return_if_fail (IS_PLUGIN_AND_NONNULL (pl));
  pl->window = NULL;
  g_message ("destroying window for %s", pl->setting->descr->name);

  /* reinit widget in plugin ports/parameters */
  for (int i = 0; i < pl->num_in_ports; i++)
    {
      Port * port = pl->in_ports[i];
      port->widget = NULL;
    }
}

static gboolean
on_close_request (GtkWindow * window, Plugin * plugin)
{
  plugin->visible = 0;
  plugin->window = NULL;
  EVENTS_PUSH (ET_PLUGIN_VISIBILITY_CHANGED, plugin);

  char pl_str[700];
  plugin_print (plugin, pl_str, 700);
  g_message ("%s: deleted plugin [%s] window", __func__, pl_str);

  return false;
}

/**
 * Creates a new GtkWindow that will be used to
 * either wrap plugin UIs or create generic UIs in.
 */
void
plugin_gtk_create_window (Plugin * plugin)
{
  g_message ("creating GTK window for %s", plugin->setting->descr->name);

  /* create window */
  plugin->window = GTK_WINDOW (gtk_window_new ());
  plugin_gtk_set_window_title (plugin, plugin->window);
  gtk_window_set_icon_name (plugin->window, "zrythm");
  /*gtk_window_set_role (*/
  /*plugin->window, "plugin_ui");*/

  if (g_settings_get_boolean (S_P_PLUGINS_UIS, "stay-on-top"))
    {
      gtk_window_set_transient_for (plugin->window, GTK_WINDOW (MAIN_WINDOW));
    }

  /* add vbox for stacking elements */
  plugin->vbox = GTK_BOX (gtk_box_new (GTK_ORIENTATION_VERTICAL, 0));
  gtk_window_set_child (GTK_WINDOW (plugin->window), GTK_WIDGET (plugin->vbox));

#if 0
  /* add menu bar */
  plugin_gtk_build_menu (
    plugin, GTK_WIDGET (plugin->window),
    GTK_WIDGET (plugin->vbox));
#endif

  /* Create/show alignment to contain UI (whether
   * custom or generic) */
  plugin->ev_box = GTK_BOX (gtk_box_new (GTK_ORIENTATION_VERTICAL, 0));
  gtk_box_append (plugin->vbox, GTK_WIDGET (plugin->ev_box));

  /* connect signals */
  plugin->destroy_window_id = g_signal_connect (
    plugin->window, "destroy", G_CALLBACK (on_window_destroy), plugin);
  plugin->close_request_id = g_signal_connect (
    G_OBJECT (plugin->window), "close-request", G_CALLBACK (on_close_request),
    plugin);
}

/**
 * Called by generic UI callbacks when e.g. a slider
 * changes value.
 */
static void
set_float_control (Plugin * pl, Port * port, float value)
{
  if (pl->setting->open_with_carla)
    {
      port_set_control_value (port, value, F_NOT_NORMALIZED, F_PUBLISH_EVENTS);
    }

  PluginGtkController * controller = (PluginGtkController *) port->widget;
  if (
    controller && controller->spin
    && !math_floats_equal (
      (float) gtk_spin_button_get_value (controller->spin), value))
    {
      gtk_spin_button_set_value (controller->spin, value);
    }
}

static gboolean
scale_changed (GtkRange * range, Port * port)
{
  /*g_message ("scale changed");*/
  g_return_val_if_fail (IS_PORT_AND_NONNULL (port), false);
  PluginGtkController * controller = port->widget;
  Plugin *              pl = controller->plugin;
  g_return_val_if_fail (IS_PLUGIN_AND_NONNULL (pl), false);

  set_float_control (pl, port, (float) gtk_range_get_value (range));

  return FALSE;
}

static gboolean
spin_changed (GtkSpinButton * spin, Port * port)
{
  PluginGtkController * controller = port->widget;
  GtkRange *            range = GTK_RANGE (controller->control);
  const double          value = gtk_spin_button_get_value (spin);
  if (!math_doubles_equal (gtk_range_get_value (range), value))
    {
      gtk_range_set_value (range, value);
    }

  return FALSE;
}

static gboolean
log_scale_changed (GtkRange * range, Port * port)
{
  /*g_message ("log scale changed");*/
  g_return_val_if_fail (IS_PORT_AND_NONNULL (port), false);
  PluginGtkController * controller = port->widget;
  Plugin *              pl = controller->plugin;
  g_return_val_if_fail (IS_PLUGIN_AND_NONNULL (pl), false);

  set_float_control (pl, port, expf ((float) gtk_range_get_value (range)));

  return FALSE;
}

static gboolean
log_spin_changed (GtkSpinButton * spin, Port * port)
{
  PluginGtkController * controller = port->widget;
  GtkRange *            range = GTK_RANGE (controller->control);
  const double          value = gtk_spin_button_get_value (spin);
  if (!math_floats_equal (
        (float) gtk_range_get_value (range), logf ((float) value)))
    {
      gtk_range_set_value (range, (double) logf ((float) value));
    }

  return FALSE;
}

static void
combo_changed (GtkComboBox * box, Port * port)
{
  GtkTreeIter iter;
  if (gtk_combo_box_get_active_iter (box, &iter))
    {
      GtkTreeModel * model = gtk_combo_box_get_model (box);
      GValue         value = { 0, { { 0 } } };

      gtk_tree_model_get_value (model, &iter, 0, &value);
      const double v = g_value_get_float (&value);
      g_value_unset (&value);

      g_return_if_fail (IS_PORT_AND_NONNULL (port));
      Plugin * pl = port_get_plugin (port, true);
      g_return_if_fail (IS_PLUGIN_AND_NONNULL (pl));

      set_float_control (pl, port, (float) v);
    }
}

static gboolean
switch_state_set (GtkSwitch * button, gboolean state, Port * port)
{
  /*g_message ("toggle_changed");*/
  g_return_val_if_fail (IS_PORT_AND_NONNULL (port), false);
  PluginGtkController * controller = port->widget;
  Plugin *              pl = controller->plugin;
  g_return_val_if_fail (IS_PLUGIN_AND_NONNULL (pl), false);

  set_float_control (pl, port, state ? 1.0f : 0.0f);

  return FALSE;
}

static PluginGtkController *
new_controller (GtkSpinButton * spin, GtkWidget * control)
{
  PluginGtkController * controller = object_new (PluginGtkController);

  controller->spin = spin;
  controller->control = control;

  return controller;
}

static PluginGtkController *
make_combo (Port * port, float value)
{
  GtkListStore * list_store =
    gtk_list_store_new (2, G_TYPE_FLOAT, G_TYPE_STRING);
  int active = -1;
  for (int i = 0; i < port->num_scale_points; i++)
    {
      const PortScalePoint * point = port->scale_points[i];

      GtkTreeIter iter;
      gtk_list_store_append (list_store, &iter);
      gtk_list_store_set (
        list_store, &iter, 0, (double) point->val, 1, point->label, -1);
      if (fabsf (value - point->val) < FLT_EPSILON)
        {
          active = i;
        }
    }

  GtkWidget * combo = gtk_combo_box_new_with_model (GTK_TREE_MODEL (list_store));
  gtk_combo_box_set_active (GTK_COMBO_BOX (combo), active);
  g_object_unref (list_store);

  bool is_input = port->id.flow == FLOW_INPUT;
  gtk_widget_set_sensitive (combo, is_input);

  GtkCellRenderer * cell = gtk_cell_renderer_text_new ();
  gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (combo), cell, TRUE);
  gtk_cell_layout_set_attributes (
    GTK_CELL_LAYOUT (combo), cell, "text", 1, NULL);

  if (is_input)
    {
      g_signal_connect (
        G_OBJECT (combo), "changed", G_CALLBACK (combo_changed), port);
    }

  return new_controller (NULL, combo);
}

static PluginGtkController *
make_log_slider (Port * port, float value)
{
  const float min = port->minf;
  const float max = port->maxf;
  const float lmin = logf (min);
  const float lmax = logf (max);
  const float ldft = logf (value);
  GtkWidget * scale =
    gtk_scale_new_with_range (GTK_ORIENTATION_HORIZONTAL, lmin, lmax, 0.001);
  gtk_widget_set_size_request (scale, MIN_SCALE_WIDTH, -1);
  GtkWidget * spin = gtk_spin_button_new_with_range (min, max, 0.000001);
  gtk_widget_set_size_request (GTK_WIDGET (spin), 140, -1);

  bool is_input = port->id.flow == FLOW_INPUT;
  gtk_widget_set_sensitive (scale, is_input);
  gtk_widget_set_sensitive (spin, is_input);

  gtk_widget_set_hexpand (scale, 1);

  gtk_scale_set_draw_value (GTK_SCALE (scale), FALSE);
  gtk_range_set_value (GTK_RANGE (scale), ldft);
  gtk_spin_button_set_value (GTK_SPIN_BUTTON (spin), value);

  if (is_input)
    {
      g_signal_connect (
        G_OBJECT (scale), "value-changed", G_CALLBACK (log_scale_changed), port);
      g_signal_connect (
        G_OBJECT (spin), "value-changed", G_CALLBACK (log_spin_changed), port);
    }

  return new_controller (GTK_SPIN_BUTTON (spin), scale);
}

static PluginGtkController *
make_slider (Port * port, float value)
{
  const float  min = port->minf;
  const float  max = port->maxf;
  bool         is_integer = port->id.flags & PORT_FLAG_INTEGER;
  const double step = is_integer ? 1.0 : ((double) (max - min) / 100.0);
  GtkWidget *  scale =
    gtk_scale_new_with_range (GTK_ORIENTATION_HORIZONTAL, min, max, step);
  gtk_widget_set_size_request (scale, MIN_SCALE_WIDTH, -1);
  GtkWidget * spin = gtk_spin_button_new_with_range (min, max, step);
  gtk_widget_set_size_request (GTK_WIDGET (spin), 140, -1);

  bool is_input = port->id.flow == FLOW_INPUT;
  gtk_widget_set_sensitive (scale, is_input);
  gtk_widget_set_sensitive (spin, is_input);

  gtk_widget_set_hexpand (scale, 1);

  if (is_integer)
    {
      gtk_spin_button_set_digits (GTK_SPIN_BUTTON (spin), 0);
    }
  else
    {
      gtk_spin_button_set_digits (GTK_SPIN_BUTTON (spin), 7);
    }

  gtk_scale_set_draw_value (GTK_SCALE (scale), FALSE);
  gtk_range_set_value (GTK_RANGE (scale), value);
  gtk_spin_button_set_value (GTK_SPIN_BUTTON (spin), value);
  if (port->num_scale_points > 0)
    {
      for (int i = 0; i < port->num_scale_points; i++)
        {
          const PortScalePoint * point = port->scale_points[i];

          char * str = g_markup_printf_escaped (
            "<span font_size=\"small\">"
            "%s</span>",
            point->label);
          gtk_scale_add_mark (GTK_SCALE (scale), point->val, GTK_POS_TOP, str);
        }
    }

  if (is_input)
    {
      g_signal_connect (
        G_OBJECT (scale), "value-changed", G_CALLBACK (scale_changed), port);
      g_signal_connect (
        G_OBJECT (spin), "value-changed", G_CALLBACK (spin_changed), port);
    }

  return new_controller (GTK_SPIN_BUTTON (spin), scale);
}

static PluginGtkController *
make_toggle (Port * port, float value)
{
  GtkWidget * check = gtk_switch_new ();
  gtk_widget_set_halign (check, GTK_ALIGN_START);

  bool is_input = port->id.flow == FLOW_INPUT;
  gtk_widget_set_sensitive (check, is_input);

  if (!math_floats_equal (value, 0))
    {
      gtk_switch_set_active (GTK_SWITCH (check), TRUE);
    }

  if (is_input)
    {
      g_signal_connect (
        G_OBJECT (check), "state-set", G_CALLBACK (switch_state_set), port);
    }

  return new_controller (NULL, check);
}

#if 0
static PluginGtkController *
make_entry (Port * port)
{
  GtkWidget * entry = gtk_entry_new ();

  bool is_input = port->id.flow == FLOW_INPUT;
  gtk_widget_set_sensitive (entry, is_input);
  if (is_input)
    {
      g_signal_connect (
        G_OBJECT (entry), "activate", G_CALLBACK (string_changed), port);
    }

  return new_controller (NULL, entry);
}

static PluginGtkController *
make_file_chooser (Port * port)
{
  GtkWidget * button = GTK_WIDGET (file_chooser_button_widget_new (
    GTK_WINDOW (MAIN_WINDOW), _ ("Open File"), GTK_FILE_CHOOSER_ACTION_OPEN));

  bool is_input = port->id.flow == FLOW_INPUT;
  gtk_widget_set_sensitive (button, is_input);

  if (is_input)
    {
      FileChangedData * data = object_new (FileChangedData);
      data->port = port;
      data->fc_btn = Z_FILE_CHOOSER_BUTTON_WIDGET (button);
      file_chooser_button_widget_set_response_callback (
        Z_FILE_CHOOSER_BUTTON_WIDGET (button), G_CALLBACK (file_changed), data,
        file_changed_data_closure_notify);
    }

  return new_controller (NULL, button);
}
#endif

static PluginGtkController *
make_controller (Plugin * pl, Port * port, float value)
{
  PluginGtkController * controller = NULL;

  if (port->id.flags & PORT_FLAG_TOGGLE)
    {
      controller = make_toggle (port, value);
    }
  else if (port->id.flags2 & PORT_FLAG2_ENUMERATION)
    {
      controller = make_combo (port, value);
    }
  else if (port->id.flags & PORT_FLAG_LOGARITHMIC)
    {
      controller = make_log_slider (port, value);
    }
  else
    {
      controller = make_slider (port, value);
    }

  if (controller)
    {
      controller->port = port;
      controller->plugin = pl;
    }

  return controller;
}

static GtkWidget *
build_control_widget (Plugin * pl, GtkWindow * window)
{
  GtkWidget * port_table = gtk_grid_new ();

  /* Make an array of ports sorted by group */
  GArray * controls = g_array_new (false, true, sizeof (Port *));
  for (int i = 0; i < pl->num_in_ports; i++)
    {
      Port * port = pl->in_ports[i];
      if (port->id.type != TYPE_CONTROL)
        continue;

      g_array_append_val (controls, port);
    }
  g_array_sort (controls, port_identifier_port_group_cmp);

  /* Add controls in group order */
  const char * last_group = NULL;
  int          n_rows = 0;
  int          num_ctrls = (int) controls->len;
  for (int i = 0; i < num_ctrls; ++i)
    {
      Port *                port = g_array_index (controls, Port *, i);
      PluginGtkController * controller = NULL;
      const char *          group = port->id.port_group;

      /* Check group and add new heading if
       * necessary */
      if (group && !string_is_equal (group, last_group))
        {
          const char * group_name = group;
          GtkWidget *  group_label =
            plugin_gtk_new_label (group_name, true, false, 0.0f, 1.0f);
          gtk_grid_attach (GTK_GRID (port_table), group_label, 0, n_rows, 2, 1);
          ++n_rows;
        }
      last_group = group;

      /* Make control widget */
      /* TODO handle non-float carla params */
      controller = make_controller (pl, port, port->deff);

      port->widget = controller;
      if (controller)
        {
          controller->port = port;
          controller->plugin = pl;

          /* Add row to table for this controller */
          plugin_gtk_add_control_row (
            port_table, n_rows++,
            (port->id.label ? port->id.label : port->id.sym), controller);

          /* Set tooltip text from comment,
           * if available */
          if (port->id.comment)
            {
              gtk_widget_set_tooltip_text (
                controller->control, port->id.comment);
            }
        }
    }

  if (n_rows > 0)
    {
      gtk_window_set_resizable (GTK_WINDOW (window), TRUE);
      GtkWidget * box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0.0);
      /*gtk_alignment_set_padding(GTK_ALIGNMENT(alignment), 0, 0, 8, 8);*/
      gtk_widget_set_margin_start (GTK_WIDGET (port_table), 8);
      gtk_widget_set_margin_end (GTK_WIDGET (port_table), 8);
      gtk_box_append (GTK_BOX (box), port_table);
      return box;
    }
  else
    {
      g_object_unref (port_table);
      GtkWidget * button = gtk_button_new_with_label (_ ("Close"));
      g_signal_connect_swapped (
        button, "clicked", G_CALLBACK (gtk_window_destroy), window);
      gtk_window_set_resizable (GTK_WINDOW (window), FALSE);
      return button;
    }
}

/**
 * Called when a property changed or when there is a
 * UI port event to set (update) the widget's value.
 */
void
plugin_gtk_generic_set_widget_value (
  Plugin *              pl,
  PluginGtkController * controller,
  float                 control)
{
  GtkWidget * widget = controller->control;
  bool        is_nan = false;
  double      fvalue = (double) control;

  /* skip setting a value if it's already set */
  g_return_if_fail (IS_PORT_AND_NONNULL (controller->port));
  if (math_floats_equal (
        controller->port->control, controller->last_set_control_val))
    return;

  controller->last_set_control_val = controller->port->control;

  if (!is_nan)
    {
      if (GTK_IS_COMBO_BOX (widget))
        {
          GtkTreeModel * model =
            gtk_combo_box_get_model (GTK_COMBO_BOX (widget));
          GValue      value = { 0, { { 0 } } };
          GtkTreeIter i;
          bool        valid = gtk_tree_model_get_iter_first (model, &i);
          while (valid)
            {
              gtk_tree_model_get_value (model, &i, 0, &value);
              const double v = g_value_get_float (&value);
              g_value_unset (&value);
              if (fabs (v - fvalue) < DBL_EPSILON)
                {
                  gtk_combo_box_set_active_iter (GTK_COMBO_BOX (widget), &i);
                  return;
                }
              valid = gtk_tree_model_iter_next (model, &i);
            }
        }
      else if (GTK_IS_TOGGLE_BUTTON (widget))
        {
          gtk_toggle_button_set_active (
            GTK_TOGGLE_BUTTON (widget), fvalue > 0.0);
        }
      else if (GTK_IS_RANGE (widget))
        {
          gtk_range_set_value (GTK_RANGE (widget), fvalue);
        }
      else if (GTK_IS_SWITCH (widget))
        {
          gtk_switch_set_active (GTK_SWITCH (widget), fvalue > 0.0);
        }
      else
        {
          g_warning (_ ("Unknown widget type for value"));
        }

      if (controller->spin)
        {
          /* Update spinner for numeric control */
          gtk_spin_button_set_value (GTK_SPIN_BUTTON (controller->spin), fvalue);
        }
    }
  else
    g_warning (_ ("Unknown widget type for value\n"));
}

/**
 * Called on each GUI frame to update the GTK UI.
 *
 * @note This is a GSourceFunc.
 */
int
plugin_gtk_update_plugin_ui (Plugin * pl)
{
  if (pl->setting->open_with_carla)
    {
      /* fetch port values */
      for (int i = 0; i < pl->num_in_ports; i++)
        {
          Port * port = pl->in_ports[i];
          if (port->id.type != TYPE_CONTROL || !port->widget)
            {
              continue;
            }

          plugin_gtk_generic_set_widget_value (pl, port->widget, port->control);
        }
    }

  return G_SOURCE_CONTINUE;
}

/**
 * Opens the generic UI of the plugin.
 *
 * Assumes plugin_gtk_create_window() has been
 * called.
 */
void
plugin_gtk_open_generic_ui (Plugin * plugin, bool fire_events)
{
  g_message ("opening generic GTK window..");
  GtkWidget * controls = build_control_widget (plugin, plugin->window);
  GtkWidget * scroll_win = gtk_scrolled_window_new ();
  gtk_scrolled_window_set_child (GTK_SCROLLED_WINDOW (scroll_win), controls);
  gtk_scrolled_window_set_policy (
    GTK_SCROLLED_WINDOW (scroll_win), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
  gtk_box_append (GTK_BOX (plugin->ev_box), scroll_win);
  gtk_widget_set_vexpand (GTK_WIDGET (scroll_win), true);

  GtkRequisition controls_size, box_size;
  gtk_widget_get_preferred_size (GTK_WIDGET (controls), NULL, &controls_size);
  gtk_widget_get_preferred_size (GTK_WIDGET (plugin->vbox), NULL, &box_size);

  gtk_window_set_default_size (
    GTK_WINDOW (plugin->window),
    MAX (MAX (box_size.width, controls_size.width) + 24, 640),
    box_size.height + controls_size.height);
  gtk_window_present (GTK_WINDOW (plugin->window));

  plugin->ui_instantiated = true;
  if (fire_events)
    {
      EVENTS_PUSH (ET_PLUGIN_VISIBILITY_CHANGED, plugin);
    }

  g_message (
    "plugin window shown, adding idle timeout. "
    "Update frequency (Hz): %.01f",
    (double) plugin->ui_update_hz);
  g_return_if_fail (plugin->ui_update_hz >= PLUGIN_MIN_REFRESH_RATE);

  plugin->update_ui_source_id = g_timeout_add (
    (guint) (1000.f / plugin->ui_update_hz),
    (GSourceFunc) plugin_gtk_update_plugin_ui, plugin);
}

/**
 * Closes the plugin's UI (either LV2 wrapped with
 * suil, generic or LV2 external).
 */
int
plugin_gtk_close_ui (Plugin * pl)
{
  g_return_val_if_fail (ZRYTHM_HAVE_UI, -1);

  if (pl->update_ui_source_id)
    {
      g_source_remove (pl->update_ui_source_id);
      pl->update_ui_source_id = 0;
    }

  g_message ("%s called", __func__);
  if (pl->window)
    {
      if (pl->destroy_window_id)
        {
          g_signal_handler_disconnect (pl->window, pl->destroy_window_id);
          pl->destroy_window_id = 0;
        }
      if (pl->close_request_id)
        {
          g_signal_handler_disconnect (pl->window, pl->close_request_id);
          pl->close_request_id = 0;
        }
      gtk_widget_set_sensitive (GTK_WIDGET (pl->window), 0);
      /*gtk_window_close (*/
      /*GTK_WINDOW (pl->window));*/
      gtk_window_destroy (GTK_WINDOW (pl->window));
      pl->window = NULL;
    }

  return 0;
}

typedef struct PresetInfo
{
  char *            name;
  GtkComboBoxText * cb;
} PresetInfo;

bool
plugin_gtk_setup_plugin_banks_combo_box (GtkComboBoxText * cb, Plugin * plugin)
{
  bool ret = false;
  gtk_combo_box_text_remove_all (cb);

  if (!plugin)
    {
      return false;
    }

  for (int i = 0; i < plugin->num_banks; i++)
    {
      PluginBank * bank = plugin->banks[i];
      if (!bank->name)
        {
          g_warning ("plugin bank at index %d has no name, skipping...", i);
          continue;
        }

      gtk_combo_box_text_append (cb, bank->uri, bank->name);
      ret = true;
    }

  gtk_combo_box_set_active (GTK_COMBO_BOX (cb), plugin->selected_bank.bank_idx);

  return ret;
}

/**
 * Sets up the combo box with all the presets the
 * plugin has in the given bank, or all the presets
 * if NULL is given.
 *
 * @return Whether any presets were added.
 */
bool
plugin_gtk_setup_plugin_presets_list_box (GtkListBox * box, Plugin * plugin)
{
  g_debug ("%s: setting up...", __func__);

  z_gtk_list_box_remove_all_children (box);

  if (!plugin || plugin->selected_bank.bank_idx == -1)
    {
      g_debug (
        "%s: no plugin (%p) or selected bank (%d)", __func__, plugin,
        plugin ? plugin->selected_bank.bank_idx : -100);
      return false;
    }

  char pl_str[800];
  plugin_print (plugin, pl_str, 800);
  if (!plugin->instantiated)
    {
      g_message ("plugin %s not instantiated", pl_str);
      return false;
    }

  bool         ret = false;
  PluginBank * bank = plugin->banks[plugin->selected_bank.bank_idx];
  for (int j = 0; j < bank->num_presets; j++)
    {
      PluginPreset * preset = bank->presets[j];
      GtkWidget *    label = gtk_label_new (preset->name);
      gtk_list_box_insert (box, label, -1);
      ret = true;
    }

  GtkListBoxRow * row =
    gtk_list_box_get_row_at_index (box, plugin->selected_preset.idx);
  gtk_list_box_select_row (box, row);

  g_debug ("%s: done", __func__);

  return ret;
}
