// SPDX-FileCopyrightText: © 2020-2024 Alexandros Theodotou <alex@zrythm.org>
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

#include "dsp/engine.h"
#include "dsp/port_identifier.h"
#include "gui/backend/event.h"
#include "gui/backend/event_manager.h"
#include "gui/widgets/main_window.h"
#include "plugins/carla_native_plugin.h"
#include "plugins/plugin.h"
#include "plugins/plugin_gtk.h"
#include "project.h"
#include "settings/g_settings_manager.h"
#include "utils/flags.h"
#include "utils/gtk.h"
#include "utils/math.h"
#include "utils/objects.h"
#include "utils/rt_thread_id.h"
#include "utils/string.h"
#include "zrythm.h"
#include "zrythm_app.h"

#include <glib/gi18n.h>

#define MIN_SCALE_WIDTH 120

#undef Bool

void
plugin_gtk_set_window_title (Plugin * plugin, GtkWindow * window)
{
  z_return_if_fail (plugin && window);

  auto title = plugin->generate_window_title ();
  gtk_window_set_title (window, title.c_str ());
}

void
plugin_gtk_on_save_preset_activate (GtkWidget * widget, Plugin * plugin)
{
  std::visit (
    [&] (auto &&p) {
      using T = base_type<decltype (p)>;
      const PluginSetting *    setting = &p->setting_;
      const PluginDescriptor * descr = &setting->descr_;
      bool                     open_with_carla = setting->open_with_carla_;

      GtkWidget * dialog = gtk_file_chooser_dialog_new (
        _ ("Save Preset"), p->window_ ? p->window_ : GTK_WINDOW (MAIN_WINDOW),
        GTK_FILE_CHOOSER_ACTION_SAVE, _ ("_Cancel"), GTK_RESPONSE_REJECT,
        _ ("_Save"), GTK_RESPONSE_ACCEPT, nullptr);

      if (open_with_carla)
        {
#if HAVE_CARLA
          char *  homedir = g_build_filename (g_get_home_dir (), nullptr);
          GFile * homedir_file = g_file_new_for_path (homedir);
          gtk_file_chooser_set_current_folder (
            GTK_FILE_CHOOSER (dialog), homedir_file, nullptr);
          g_object_unref (homedir_file);
          g_free (homedir);
#else
          z_return_if_reached ();
#endif
        }

      /* add additional inputs */
      GtkWidget * content = gtk_dialog_get_content_area (GTK_DIALOG (dialog));
      GtkWidget * add_prefix =
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
          if constexpr (std::is_same_v<T, CarlaNativePlugin>)
            {
#if HAVE_CARLA
              std::string  prefix;
              const char * sep = "";
              auto         dirname = Glib::path_get_dirname (path);
              auto         basename = Glib::path_get_basename (path);
              char *       sym = string_symbolify (basename.c_str ());
              if (add_prefix_active)
                {
                  prefix = descr->name_;
                  sep = "_";
                }
              char * sprefix = string_symbolify (prefix.c_str ());
              char * bundle = g_strjoin (
                nullptr, sprefix, sep, sym, ".preset.carla", nullptr);
              auto dir = Glib::build_filename (dirname, bundle);
              try
                {
                  p->save_state (false, &dir);
                }
              catch (const ZrythmException &e)
                {
                  e.handle ("Failed to save Carla state");
                }
              g_free (sym);
              g_free (sprefix);
              g_free (bundle);
#endif
            }
          g_free (path);
        }

      gtk_window_destroy (GTK_WINDOW (dialog));

      EVENTS_PUSH (EventType::ET_PLUGIN_PRESET_SAVED, p);
    },
    convert_to_variant<PluginPtrVariant> (plugin));
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
  GtkWidget * label = gtk_label_new (nullptr);
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
      const PortIdentifier &id = controller->port->id_;

      if (id.unit_ > PortUnit::None)
        {
          sprintf (
            name, "%s <small>(%s)</small>", _name, port_unit_to_str (id.unit_));
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
  z_return_if_fail (IS_PLUGIN_AND_NONNULL (pl));
  pl->window_ = nullptr;
  z_info ("destroying window for {}", pl->get_name ());

  /* reinit widget in plugin ports/parameters */
  for (auto port : pl->in_ports_ | type_is<ControlPort> ())
    {
      port->widget_ = nullptr;
    }
}

static gboolean
on_close_request (GtkWindow * window, Plugin * plugin)
{
  plugin->visible_ = 0;
  plugin->window_ = NULL;
  EVENTS_PUSH (EventType::ET_PLUGIN_VISIBILITY_CHANGED, plugin);

  z_info ("deleted plugin [{}] window", plugin->print ());

  return false;
}

/**
 * Creates a new GtkWindow that will be used to
 * either wrap plugin UIs or create generic UIs in.
 */
void
plugin_gtk_create_window (Plugin * plugin)
{
  z_debug ("creating GTK window for {}", plugin->get_name ());

  /* create window */
  plugin->window_ = GTK_WINDOW (gtk_window_new ());
  plugin_gtk_set_window_title (plugin, plugin->window_);
  gtk_window_set_icon_name (plugin->window_, "zrythm");
  /*gtk_window_set_role (*/
  /*plugin->window, "plugin_ui");*/

  if (g_settings_get_boolean (S_P_PLUGINS_UIS, "stay-on-top"))
    {
      gtk_window_set_transient_for (plugin->window_, GTK_WINDOW (MAIN_WINDOW));
    }

  /* add vbox for stacking elements */
  plugin->vbox_ = GTK_BOX (gtk_box_new (GTK_ORIENTATION_VERTICAL, 0));
  gtk_window_set_child (
    GTK_WINDOW (plugin->window_), GTK_WIDGET (plugin->vbox_));

#if 0
  /* add menu bar */
  plugin_gtk_build_menu (
    plugin, GTK_WIDGET (plugin->window),
    GTK_WIDGET (plugin->vbox));
#endif

  /* Create/show alignment to contain UI (whether
   * custom or generic) */
  plugin->ev_box_ = GTK_BOX (gtk_box_new (GTK_ORIENTATION_VERTICAL, 0));
  gtk_box_append (plugin->vbox_, GTK_WIDGET (plugin->ev_box_));

  /* connect signals */
  plugin->destroy_window_id_ = g_signal_connect (
    plugin->window_, "destroy", G_CALLBACK (on_window_destroy), plugin);
  plugin->close_request_id_ = g_signal_connect (
    G_OBJECT (plugin->window_), "close-request", G_CALLBACK (on_close_request),
    plugin);
}

/**
 * Called by generic UI callbacks when e.g. a slider changes value.
 */
static void
set_float_control (Plugin * pl, ControlPort * port, float value)
{
  port->set_control_value (value, F_NOT_NORMALIZED, true);

  PluginGtkController * controller = (PluginGtkController *) port->widget_;
  if (
    controller && controller->spin
    && !math_floats_equal (
      (float) gtk_spin_button_get_value (controller->spin), value))
    {
      gtk_spin_button_set_value (controller->spin, value);
    }
}

static gboolean
scale_changed (GtkRange * range, ControlPort * port)
{
  /*z_info ("scale changed");*/
  z_return_val_if_fail (IS_PORT_AND_NONNULL (port), false);
  PluginGtkController * controller = port->widget_;
  Plugin *              pl = controller->plugin;
  z_return_val_if_fail (IS_PLUGIN_AND_NONNULL (pl), false);

  set_float_control (pl, port, (float) gtk_range_get_value (range));

  return FALSE;
}

static gboolean
spin_changed (GtkSpinButton * spin, ControlPort * port)
{
  PluginGtkController * controller = port->widget_;
  GtkRange *            range = GTK_RANGE (controller->control);
  const double          value = gtk_spin_button_get_value (spin);
  if (!math_doubles_equal (gtk_range_get_value (range), value))
    {
      gtk_range_set_value (range, value);
    }

  return FALSE;
}

static gboolean
log_scale_changed (GtkRange * range, ControlPort * port)
{
  /*z_info ("log scale changed");*/
  z_return_val_if_fail (IS_PORT_AND_NONNULL (port), false);
  PluginGtkController * controller = port->widget_;
  Plugin *              pl = controller->plugin;
  z_return_val_if_fail (IS_PLUGIN_AND_NONNULL (pl), false);

  set_float_control (pl, port, expf ((float) gtk_range_get_value (range)));

  return FALSE;
}

static gboolean
log_spin_changed (GtkSpinButton * spin, ControlPort * port)
{
  PluginGtkController * controller = port->widget_;
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
combo_changed (GtkComboBox * box, ControlPort * port)
{
  GtkTreeIter iter;
  if (gtk_combo_box_get_active_iter (box, &iter))
    {
      GtkTreeModel * model = gtk_combo_box_get_model (box);
      GValue         value = { 0, { {} } };

      gtk_tree_model_get_value (model, &iter, 0, &value);
      const double v = g_value_get_float (&value);
      g_value_unset (&value);

      z_return_if_fail (IS_PORT_AND_NONNULL (port));
      Plugin * pl = port->get_plugin (true);
      z_return_if_fail (IS_PLUGIN_AND_NONNULL (pl));

      set_float_control (pl, port, (float) v);
    }
}

static gboolean
switch_state_set (GtkSwitch * button, gboolean state, ControlPort * port)
{
  /*z_info ("toggle_changed");*/
  z_return_val_if_fail (IS_PORT_AND_NONNULL (port), false);
  PluginGtkController * controller = port->widget_;
  Plugin *              pl = controller->plugin;
  z_return_val_if_fail (IS_PLUGIN_AND_NONNULL (pl), false);

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
make_combo (ControlPort * port, float value)
{
  GtkListStore * list_store =
    gtk_list_store_new (2, G_TYPE_FLOAT, G_TYPE_STRING);
  int active = -1;
  int count = 0;
  for (auto &point : port->scale_points_)
    {
      GtkTreeIter iter;
      gtk_list_store_append (list_store, &iter);
      gtk_list_store_set (
        list_store, &iter, 0, (double) point.val_, 1, point.label_.c_str (), -1);
      if (fabsf (value - point.val_) < FLT_EPSILON)
        {
          active = count;
        }
      count++;
    }

  GtkWidget * combo = gtk_combo_box_new_with_model (GTK_TREE_MODEL (list_store));
  gtk_combo_box_set_active (GTK_COMBO_BOX (combo), active);
  g_object_unref (list_store);

  bool is_input = port->id_.flow_ == PortFlow::Input;
  gtk_widget_set_sensitive (combo, is_input);

  GtkCellRenderer * cell = gtk_cell_renderer_text_new ();
  gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (combo), cell, TRUE);
  gtk_cell_layout_set_attributes (
    GTK_CELL_LAYOUT (combo), cell, "text", 1, nullptr);

  if (is_input)
    {
      g_signal_connect (
        G_OBJECT (combo), "changed", G_CALLBACK (combo_changed), port);
    }

  return new_controller (nullptr, combo);
}

static PluginGtkController *
make_log_slider (Port * port, float value)
{
  const float min = port->minf_;
  const float max = port->maxf_;
  const float lmin = logf (min);
  const float lmax = logf (max);
  const float ldft = logf (value);
  GtkWidget * scale =
    gtk_scale_new_with_range (GTK_ORIENTATION_HORIZONTAL, lmin, lmax, 0.001);
  gtk_widget_set_size_request (scale, MIN_SCALE_WIDTH, -1);
  GtkWidget * spin = gtk_spin_button_new_with_range (min, max, 0.000001);
  gtk_widget_set_size_request (GTK_WIDGET (spin), 140, -1);

  bool is_input = port->id_.flow_ == PortFlow::Input;
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
make_slider (ControlPort * port, float value)
{
  const float min = port->minf_;
  const float max = port->maxf_;
  bool        is_integer = ENUM_BITSET_TEST (
    PortIdentifier::Flags, port->id_.flags_, PortIdentifier::Flags::Integer);
  const double step = is_integer ? 1.0 : ((double) (max - min) / 100.0);
  GtkWidget *  scale =
    gtk_scale_new_with_range (GTK_ORIENTATION_HORIZONTAL, min, max, step);
  gtk_widget_set_size_request (scale, MIN_SCALE_WIDTH, -1);
  GtkWidget * spin = gtk_spin_button_new_with_range (min, max, step);
  gtk_widget_set_size_request (GTK_WIDGET (spin), 140, -1);

  bool is_input = port->id_.flow_ == PortFlow::Input;
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
  if (port->scale_points_.size () > 0)
    {
      for (auto &point : port->scale_points_)
        {
          char * str = g_markup_printf_escaped (
            "<span font_size=\"small\">%s</span>", point.label_.c_str ());
          gtk_scale_add_mark (GTK_SCALE (scale), point.val_, GTK_POS_TOP, str);
          g_free (str);
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

  bool is_input = port->id_.flow_ == PortFlow::Input;
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

  return new_controller (nullptr, check);
}

#if 0
static PluginGtkController *
make_entry (Port * port)
{
  GtkWidget * entry = gtk_entry_new ();

  bool is_input = port->id_.flow == PortFlow::Input;
  gtk_widget_set_sensitive (entry, is_input);
  if (is_input)
    {
      g_signal_connect (
        G_OBJECT (entry), "activate", G_CALLBACK (string_changed), port);
    }

  return new_controller (nullptr, entry);
}

static PluginGtkController *
make_file_chooser (Port * port)
{
  GtkWidget * button = GTK_WIDGET (file_chooser_button_widget_new (
    GTK_WINDOW (MAIN_WINDOW), _ ("Open File"), GTK_FILE_CHOOSER_ACTION_OPEN));

  bool is_input = port->id_.flow == PortFlow::Input;
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

  return new_controller (nullptr, button);
}
#endif

static PluginGtkController *
make_controller (Plugin * pl, ControlPort * port, float value)
{
  PluginGtkController * controller = NULL;

  if (ENUM_BITSET_TEST (
        PortIdentifier::Flags, port->id_.flags_, PortIdentifier::Flags::Toggle))
    {
      controller = make_toggle (port, value);
    }
  else if (
    ENUM_BITSET_TEST (
      PortIdentifier::Flags2, port->id_.flags2_,
      PortIdentifier::Flags2::Enumeration))
    {
      controller = make_combo (port, value);
    }
  else if (
    ENUM_BITSET_TEST (
      PortIdentifier::Flags, port->id_.flags_,
      PortIdentifier::Flags::Logarithmic))
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
  for (auto port : pl->in_ports_ | type_is<ControlPort> ())
    {
      g_array_append_val (controls, port);
    }
  g_array_sort (controls, PortIdentifier::port_group_cmp);

  /* Add controls in group order */
  const char * last_group = NULL;
  int          n_rows = 0;
  int          num_ctrls = (int) controls->len;
  for (int i = 0; i < num_ctrls; ++i)
    {
      auto                  port = g_array_index (controls, ControlPort *, i);
      PluginGtkController * controller = NULL;
      const char *          group = port->id_.port_group_.c_str ();

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
      controller = make_controller (pl, port, port->deff_);

      port->widget_ = controller;
      if (controller)
        {
          controller->port = port;
          controller->plugin = pl;

          /* Add row to table for this controller */
          plugin_gtk_add_control_row (
            port_table, n_rows++,
            (port->id_.label_.empty ()
               ? port->id_.sym_.c_str ()
               : port->get_label ().c_str ()),
            controller);

          /* Set tooltip text from comment, if available */
          if (!port->id_.comment_.empty ())
            {
              gtk_widget_set_tooltip_text (
                controller->control, port->id_.comment_.c_str ());
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
  z_return_if_fail (IS_PORT_AND_NONNULL (controller->port));
  if (math_floats_equal (
        controller->port->control_, controller->last_set_control_val))
    return;

  controller->last_set_control_val = controller->port->control_;

  if (!is_nan)
    {
      if (GTK_IS_COMBO_BOX (widget))
        {
          GtkTreeModel * model =
            gtk_combo_box_get_model (GTK_COMBO_BOX (widget));
          GValue      value = { 0, { {} } };
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
          z_warning (_ ("Unknown widget type for value"));
        }

      if (controller->spin)
        {
          /* Update spinner for numeric control */
          gtk_spin_button_set_value (GTK_SPIN_BUTTON (controller->spin), fvalue);
        }
    }
  else
    z_warning (_ ("Unknown widget type for value\n"));
}

/**
 * Called on each GUI frame to update the GTK UI.
 *
 * @note This is a GSourceFunc.
 */
int
plugin_gtk_update_plugin_ui (Plugin * pl)
{
  if (pl->setting_.open_with_carla_)
    {
      /* fetch port values */
      for (auto port : pl->in_ports_ | type_is<ControlPort> ())
        {
          if (!port->widget_)
            continue;

          plugin_gtk_generic_set_widget_value (
            pl, port->widget_, port->control_);
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
  z_info ("opening generic GTK window..");
  GtkWidget * controls = build_control_widget (plugin, plugin->window_);
  GtkWidget * scroll_win = gtk_scrolled_window_new ();
  gtk_scrolled_window_set_child (GTK_SCROLLED_WINDOW (scroll_win), controls);
  gtk_scrolled_window_set_policy (
    GTK_SCROLLED_WINDOW (scroll_win), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
  gtk_box_append (GTK_BOX (plugin->ev_box_), scroll_win);
  gtk_widget_set_vexpand (GTK_WIDGET (scroll_win), true);

  GtkRequisition controls_size, box_size;
  gtk_widget_get_preferred_size (GTK_WIDGET (controls), nullptr, &controls_size);
  gtk_widget_get_preferred_size (GTK_WIDGET (plugin->vbox_), nullptr, &box_size);

  gtk_window_set_default_size (
    GTK_WINDOW (plugin->window_),
    MAX (MAX (box_size.width, controls_size.width) + 24, 640),
    box_size.height + controls_size.height);
  gtk_window_present (GTK_WINDOW (plugin->window_));

  if (fire_events)
    {
      EVENTS_PUSH (EventType::ET_PLUGIN_VISIBILITY_CHANGED, plugin);
    }

  z_info (
    "plugin window shown, adding idle timeout. "
    "Update frequency (Hz): %.01f",
    (double) plugin->ui_update_hz_);
  z_return_if_fail (plugin->ui_update_hz_ >= Plugin::MIN_REFRESH_RATE);

  plugin->update_ui_source_id_ = g_timeout_add (
    (guint) (1000.f / plugin->ui_update_hz_),
    (GSourceFunc) plugin_gtk_update_plugin_ui, plugin);
}

/**
 * Closes the plugin's UI (either LV2 wrapped with
 * suil, generic or LV2 external).
 */
int
plugin_gtk_close_ui (Plugin * pl)
{
  z_return_val_if_fail (ZRYTHM_HAVE_UI, -1);

  if (pl->update_ui_source_id_)
    {
      g_source_remove (pl->update_ui_source_id_);
      pl->update_ui_source_id_ = 0;
    }

  z_info ("{} called", __func__);
  if (pl->window_)
    {
      if (pl->destroy_window_id_)
        {
          g_signal_handler_disconnect (pl->window_, pl->destroy_window_id_);
          pl->destroy_window_id_ = 0;
        }
      if (pl->close_request_id_)
        {
          g_signal_handler_disconnect (pl->window_, pl->close_request_id_);
          pl->close_request_id_ = 0;
        }
      gtk_widget_set_sensitive (GTK_WIDGET (pl->window_), 0);
      /*gtk_window_close (*/
      /*GTK_WINDOW (pl->window));*/
      gtk_window_destroy (GTK_WINDOW (pl->window_));
      pl->window_ = nullptr;
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

  for (auto &bank : plugin->banks_)
    {
      if (bank.name_.empty ())
        {
          z_warning ("plugin bank has no name, skipping...");
          continue;
        }

      gtk_combo_box_text_append (cb, bank.uri_.c_str (), bank.name_.c_str ());
      ret = true;
    }

  gtk_combo_box_set_active (
    GTK_COMBO_BOX (cb), plugin->selected_bank_.bank_idx_);

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
  z_debug ("setting up...");

  z_gtk_list_box_remove_all_children (box);

  if (!plugin || plugin->selected_bank_.bank_idx_ == -1)
    {
      z_debug (
        "no plugin ({}) or selected bank ({})", fmt::ptr (plugin),
        plugin ? plugin->selected_bank_.bank_idx_ : -100);
      return false;
    }

  if (!plugin->instantiated_)
    {
      z_info ("plugin {} not instantiated", plugin->print ());
      return false;
    }

  bool  ret = false;
  auto &bank = plugin->banks_[plugin->selected_bank_.bank_idx_];
  for (auto &preset : bank.presets_)
    {
      GtkWidget * label = gtk_label_new (preset.name_.c_str ());
      gtk_list_box_insert (box, label, -1);
      ret = true;
    }

  GtkListBoxRow * row =
    gtk_list_box_get_row_at_index (box, plugin->selected_preset_.idx_);
  gtk_list_box_select_row (box, row);

  z_debug ("{}: done", __func__);

  return ret;
}
