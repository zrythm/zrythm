/*
 * Copyright (C) 2018-2020 Alexandros Theodotou <alex at zrythm dot org>
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

/*
  Copyright 2007-2017 David Robillard <http://drobilla.net>

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
*/

#include <math.h>

#include "audio/engine.h"
#include "audio/track.h"
#include "gui/backend/event.h"
#include "gui/backend/event_manager.h"
#include "gui/widgets/instrument_track.h"
#include "gui/widgets/main_window.h"
#include "plugins/lv2_plugin.h"
#include "plugins/lv2/lv2_gtk.h"
#include "plugins/lv2/lv2_state.h"
#include "plugins/lv2/lv2_ui.h"
#include "plugins/plugin_gtk.h"
#include "plugins/plugin_manager.h"
#include "project.h"
#include "settings/settings.h"
#include "utils/lilv.h"
#include "utils/math.h"
#include "zrythm.h"
#include "zrythm_app.h"

#include <gtk/gtk.h>

#include <glib/gi18n.h>
#include <lv2/patch/patch.h>
#include <lv2/port-props/port-props.h>
#include <suil/suil.h>

#define MIN_SCALE_WIDTH 120

/**
 * To be called by plugin_gtk both on generic UIs
 * and normal UIs when a plugin window is destroyed.
 */
void
lv2_gtk_on_window_destroy (
  Lv2Plugin * plugin)
{
  /* reinit widget in plugin ports and controls */
  int num_ctrls = plugin->controls.n_controls;
  int i;
  for (i = 0; i < num_ctrls; i++)
    {
      Lv2Control * control =
        plugin->controls.controls[i];

      control->widget = NULL;
    }

  for (i = 0; i < plugin->num_ports; i++)
    {
      Lv2Port * port =
        &plugin->ports[i];

      port->widget = NULL;
    }

  suil_instance_free (plugin->ui_instance);
  plugin->ui_instance = NULL;
}

void
lv2_gtk_on_save_activate (
  Lv2Plugin * plugin)
{
  return;

# if 0
  GtkWidget* dialog = gtk_file_chooser_dialog_new(
    _("Save State"),
    plugin->plugin->window,
    GTK_FILE_CHOOSER_ACTION_CREATE_FOLDER,
    _("_Cancel"), GTK_RESPONSE_CANCEL,
    _("_Save"), GTK_RESPONSE_ACCEPT,
    NULL);

  if (gtk_dialog_run (
        GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
    char* path =
      gtk_file_chooser_get_filename (
        GTK_FILE_CHOOSER(dialog));
    char* base =
      g_build_filename (path, "/", NULL);
    lv2_state_save_to_file (plugin, base);
    g_free (path);
    g_free (base);
  }

  gtk_widget_destroy(dialog);
#endif
}

static char*
symbolify (const char* in)
{
  const size_t len = strlen(in);
  char*        out = (char*)calloc(len + 1, 1);
  for (size_t i = 0; i < len; ++i) {
    if (g_ascii_isalnum(in[i])) {
      out[i] = in[i];
    } else {
      out[i] = '_';
    }
  }
  return out;
}

PluginGtkPresetMenu*
lv2_gtk_get_bank_menu (
  Lv2Plugin* plugin,
  PluginGtkPresetMenu* menu,
  const LilvNode* bank)
{
  LilvNode* label =
    lilv_world_get (
      LILV_WORLD, bank,
      PM_LILV_NODES.rdfs_label, NULL);

  const char* uri = lilv_node_as_string(bank);
  const char* str =
    label ? lilv_node_as_string(label) : uri;
  PluginGtkPresetMenu key = { NULL, (char*)str, NULL, NULL };
  GSequenceIter* i =
    g_sequence_lookup (
      menu->banks, &key, plugin_gtk_menu_cmp, NULL);
  if (!i)
    {
      PluginGtkPresetMenu* bank_menu =
        plugin_gtk_preset_menu_new(str);
      gtk_menu_item_set_submenu(bank_menu->item, GTK_WIDGET(bank_menu->menu));
      g_sequence_insert_sorted(menu->banks, bank_menu, plugin_gtk_menu_cmp, NULL);
      return bank_menu;
    }

  return (PluginGtkPresetMenu*) g_sequence_get (i);
}

int
lv2_gtk_add_preset_to_menu (
  Lv2Plugin*      plugin,
  const LilvNode* node,
  const LilvNode* title,
  void*           data)
{
  PluginGtkPresetMenu* menu  = (PluginGtkPresetMenu*)data;
  const char* label =
    lilv_node_as_string(title);
  GtkWidget*  item  =
    gtk_check_menu_item_new_with_label(label);
  gtk_check_menu_item_set_draw_as_radio (
    GTK_CHECK_MENU_ITEM(item), TRUE);
  if (plugin->preset &&
      lilv_node_equals (
        lilv_state_get_uri (
          plugin->preset), node))
    {
      gtk_check_menu_item_set_active (
        GTK_CHECK_MENU_ITEM(item), TRUE);
      plugin->plugin->active_preset_item =
        GTK_CHECK_MENU_ITEM(item);
    }

  LilvNode* bank =
    lilv_world_get (
      LILV_WORLD, node,
      PM_LILV_NODES.pset_bank, NULL);

  if (bank)
    {
      PluginGtkPresetMenu* bank_menu =
        lv2_gtk_get_bank_menu (plugin, menu, bank);
      gtk_menu_shell_append (
        GTK_MENU_SHELL (bank_menu->menu), item);
    }
  else
    gtk_menu_shell_append (
      GTK_MENU_SHELL(menu->menu), item);

  PluginGtkPresetRecord* record =
    calloc (1, sizeof(PluginGtkPresetRecord));
  record->plugin = plugin->plugin;
  record->preset = lilv_node_duplicate (node);

  g_signal_connect_data (
    G_OBJECT (item), "activate",
    G_CALLBACK (plugin_gtk_on_preset_activate),
    record,
    (GClosureNotify) plugin_gtk_on_preset_destroy,
    (GConnectFlags) 0);

  return 0;
}

void
lv2_gtk_on_save_preset_activate (
  GtkWidget* widget,
  Lv2Plugin * plugin)
{
  GtkWidget* dialog = gtk_file_chooser_dialog_new(
    "Save Preset",
    plugin->plugin->window,
    GTK_FILE_CHOOSER_ACTION_SAVE,
    "_Cancel", GTK_RESPONSE_REJECT,
    "_Save", GTK_RESPONSE_ACCEPT,
    NULL);

  char* dot_lv2 =
    g_build_filename (
      g_get_home_dir(), ".lv2", NULL);
  gtk_file_chooser_set_current_folder (
    GTK_FILE_CHOOSER(dialog), dot_lv2);
  free(dot_lv2);

  GtkWidget* content =
    gtk_dialog_get_content_area (
      GTK_DIALOG(dialog));
  GtkBox* box =
    GTK_BOX (
      gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 8));
  GtkWidget* uri_label =
    gtk_label_new (_("URI (Optional):"));
  GtkWidget* uri_entry = gtk_entry_new();
  GtkWidget* add_prefix =
    gtk_check_button_new_with_mnemonic (
      _("_Prefix plugin name"));

  gtk_toggle_button_set_active (
    GTK_TOGGLE_BUTTON(add_prefix), TRUE);
  gtk_box_pack_start (
    box, uri_label, FALSE, TRUE, 2);
  gtk_box_pack_start (
    box, uri_entry, TRUE, TRUE, 2);
  gtk_box_pack_start (
    GTK_BOX(content), GTK_WIDGET(box),
    FALSE, FALSE, 6);
  gtk_box_pack_start (
    GTK_BOX(content), add_prefix, FALSE, FALSE, 6);

  gtk_widget_show_all (GTK_WIDGET(dialog));
  gtk_entry_set_activates_default (
    GTK_ENTRY(uri_entry), TRUE);
  gtk_dialog_set_default_response (
    GTK_DIALOG(dialog), GTK_RESPONSE_ACCEPT);
  if (gtk_dialog_run (GTK_DIALOG(dialog)) ==
        GTK_RESPONSE_ACCEPT)
    {
      LilvNode* plug_name =
        lilv_plugin_get_name (plugin->lilv_plugin);
      const char* path =
        gtk_file_chooser_get_filename (
          GTK_FILE_CHOOSER(dialog));
      const char* uri =
        gtk_entry_get_text (GTK_ENTRY(uri_entry));
      const char* prefix = "";
      const char* sep = "";
      if (gtk_toggle_button_get_active (
            GTK_TOGGLE_BUTTON(add_prefix)))
        {
          prefix = lilv_node_as_string(plug_name);
          sep = "_";
        }

      char* dirname = g_path_get_dirname(path);
      char* basename = g_path_get_basename(path);
      char* sym = symbolify(basename);
      char* sprefix = symbolify(prefix);
      char* bundle =
        g_strjoin (NULL, sprefix, sep, sym,
                   ".preset.lv2/", NULL);
      char* file =
        g_strjoin (NULL, sym, ".ttl", NULL);
      char* dir =
        g_build_filename (dirname, bundle, NULL);

      lv2_state_save_preset (
        plugin, dir, (strlen(uri) ? uri : NULL),
        basename, file);

      // Reload bundle into the world
      LilvNode* ldir =
        lilv_new_file_uri (LILV_WORLD, NULL, dir);
      lilv_world_unload_bundle(LILV_WORLD, ldir);
      lilv_world_load_bundle(LILV_WORLD, ldir);
      lilv_node_free(ldir);

      // Rebuild preset menu and update window title
      plugin_gtk_rebuild_preset_menu (
        plugin->plugin,
        GTK_CONTAINER (
          gtk_widget_get_parent (widget)));
      plugin_gtk_set_window_title (
        plugin->plugin, plugin->plugin->window);

      g_free(dir);
      g_free(file);
      g_free(bundle);
      free(sprefix);
      free(sym);
      g_free(basename);
      g_free(dirname);
      lilv_node_free(plug_name);
    }

  gtk_widget_destroy (GTK_WIDGET (dialog));
}

void
lv2_gtk_on_delete_preset_activate (
  GtkWidget* widget,
  Lv2Plugin * plugin)
{
  if (!plugin->preset)
    return;

  GtkWidget* dialog =
    gtk_dialog_new_with_buttons(
      _("Delete Preset?"),
      plugin->plugin->window,
      (GtkDialogFlags) (
        GTK_DIALOG_MODAL |
        GTK_DIALOG_DESTROY_WITH_PARENT),
      _("_Cancel"), GTK_RESPONSE_REJECT,
      _("_OK"), GTK_RESPONSE_ACCEPT,
      NULL);

  char* msg =
    g_strdup_printf (
      "Delete preset \"%s\" from the file system?",
      lilv_state_get_label(plugin->preset));

  GtkWidget* content =
    gtk_dialog_get_content_area(GTK_DIALOG(dialog));
  GtkWidget* text = gtk_label_new(msg);
  gtk_box_pack_start (
    GTK_BOX(content), text, TRUE, TRUE, 4);

  gtk_widget_show_all (dialog);
  if (gtk_dialog_run (GTK_DIALOG(dialog)) ==
        GTK_RESPONSE_ACCEPT)
    {
      lv2_state_delete_current_preset (plugin);
      plugin_gtk_rebuild_preset_menu (
        plugin->plugin,
        GTK_CONTAINER (
          gtk_widget_get_parent (widget)));
    }

  lilv_state_free (plugin->preset);
  plugin->preset = NULL;
  plugin_gtk_set_window_title (
    plugin->plugin, plugin->plugin->window);

  g_free (msg);
  gtk_widget_destroy (text);
  gtk_widget_destroy (dialog);
}

/**
 * Eventually called by the callbacks when the user
 * changes a widget value (e.g., the slider changed
 * callback).
 *
 * Calls lv2_control_set_control to change the
 * value.
 */
static void
set_control (
  const Lv2Control* control,
  uint32_t         size,
  LV2_URID         type,
  const void*      body)
{
  /*g_message ("set_control");*/
  if (!control->plugin->updating)
    {
      lv2_control_set_control (
        control, size, type, body);
    }
}

/**
 * Called by generic UI callbacks when e.g. a slider
 * changes value.
 */
void
lv2_gtk_set_float_control (
  const Lv2Control* control,
  float value)
{
  /*g_message ("lv2_gtk_set_float_control");*/
  if (control->value_type ==
      control->plugin->forge.Int)
    {
      const int32_t ival = lrint(value);
      set_control (control,
                   sizeof(ival),
                   control->plugin->forge.Int,
                   &ival);
    }
  else if (control->value_type ==
           control->plugin->forge.Long)
    {
      const int64_t lval = lrint(value);
      set_control (control,
                   sizeof(lval),
                   control->plugin->forge.Long,
                   &lval);
    }
  else if (control->value_type ==
           control->plugin->forge.Float)
    {
      set_control (control,
                   sizeof(value),
                   control->plugin->forge.Float,
                   &value);
    }
  else if (control->value_type ==
           control->plugin->forge.Double)
    {
      const double dval = value;
      set_control (control,
                   sizeof(dval),
                   control->plugin->forge.Double,
                   &dval);
    }
  else if (control->value_type ==
           control->plugin->forge.Bool)
    {
      const int32_t ival = value;
      set_control (control,
                   sizeof(ival),
                   control->plugin->forge.Bool,
                   &ival);
    }
  /*else*/
    /*{*/
      /*[> FIXME ? <]*/
      /*set_control (control,*/
                   /*sizeof(value),*/
                   /*control->plugin->forge.Float,*/
                   /*&value);*/
    /*}*/

  PluginGtkController * controller =
    (PluginGtkController *) control->widget;
  if (controller && controller->spin &&
      !math_floats_equal (
        gtk_spin_button_get_value (
          controller->spin), value))
    gtk_spin_button_set_value (
      controller->spin, value);
}

static double
get_atom_double (
  Lv2Plugin* plugin,
  uint32_t size,
  LV2_URID type,
  const void* body)
{
  if (type == plugin->forge.Int ||
      type == plugin->forge.Bool)
    return *(const int32_t*)body;
  else if (type == plugin->forge.Long)
    return *(const int64_t*)body;
  else if (type == plugin->forge.Float)
    return *(const float*)body;
  else if (type == plugin->forge.Double)
    return *(const double*)body;

  return NAN;
}

/**
 * Called when a property changed or when there is a
 * UI port event.
 */
static void
control_changed (
  Lv2Plugin*       plugin,
  PluginGtkController* controller,
  uint32_t    size,
  LV2_URID    type,
  const void* body)
{
  GtkWidget * widget = controller->control;
  const double fvalue =
    get_atom_double (plugin, size, type, body);

  if (!isnan(fvalue))
    {
      if (GTK_IS_COMBO_BOX(widget))
        {
          GtkTreeModel* model =
            gtk_combo_box_get_model (
              GTK_COMBO_BOX (widget));
          GValue value = { 0, { { 0 } } };
          GtkTreeIter   i;
          bool valid =
            gtk_tree_model_get_iter_first (model, &i);
          while (valid)
            {
              gtk_tree_model_get_value (
                model, &i, 0, &value);
              const double v =
                g_value_get_float(&value);
              g_value_unset (&value);
              if (fabs(v - fvalue) < FLT_EPSILON)
                {
                  gtk_combo_box_set_active_iter (
                    GTK_COMBO_BOX (widget), &i);
                  return;
                }
              valid =
                gtk_tree_model_iter_next(model, &i);
            }
        }
      else if (GTK_IS_TOGGLE_BUTTON (widget))
        {
          gtk_toggle_button_set_active (
            GTK_TOGGLE_BUTTON (widget),
            fvalue > 0.0f);
        }
      else if (GTK_IS_RANGE (widget))
        {
          gtk_range_set_value (
            GTK_RANGE (widget), fvalue);
        }
      else if (GTK_IS_SWITCH (widget))
        {
          gtk_switch_set_active (
            GTK_SWITCH (widget), fvalue > 0.f);
        }
      else
        {
          g_warning (
            _("Unknown widget type for value"));
        }

      if (controller->spin)
        {
          // Update spinner for numeric control
          gtk_spin_button_set_value (
            GTK_SPIN_BUTTON (controller->spin),
            fvalue);
        }
    }
  else if (GTK_IS_ENTRY(widget) &&
           type == PM_URIDS.atom_String)
    {
      gtk_entry_set_text (
        GTK_ENTRY(widget), (const char*)body);
    }
  else if (GTK_IS_FILE_CHOOSER (widget) &&
           type == PM_URIDS.atom_Path)
    {
      gtk_file_chooser_set_filename (
        GTK_FILE_CHOOSER(widget),
        (const char*)body);
    }
  else
    g_warning (
      _("Unknown widget type for value\n"));
}

static int
patch_set_get (
  Lv2Plugin *            plugin,
  const LV2_Atom_Object* obj,
  const LV2_Atom_URID**  property,
  const LV2_Atom**       value)
{
  lv2_atom_object_get (
    obj,
    PM_URIDS.patch_property,
    (const LV2_Atom*)property,
    PM_URIDS.patch_value,
    value, 0);
  if (!*property)
    {
      g_warning (
        "patch:Set message with no property");
      return 1;
    }
  else if ((*property)->atom.type !=
           plugin->forge.URID)
    {
      g_warning (
        "patch:Set property is not a URID");
      return 1;
    }

  return 0;
}

static int
patch_put_get(
  Lv2Plugin*                   plugin,
  const LV2_Atom_Object*  obj,
  const LV2_Atom_Object** body)
{
  lv2_atom_object_get (
    obj,
    PM_URIDS.patch_body,
    (const LV2_Atom*)body,
    0);
  if (!*body)
    {
      g_warning (
        "patch:Put message with no body");
      return 1;
    }
  else if (!lv2_atom_forge_is_object_type (
              &plugin->forge,
              (*body)->atom.type))
    {
      g_warning (
        "patch:Put body is not an object");
      return 1;
    }

  return 0;
}

static void
property_changed (
  Lv2Plugin* plugin,
  LV2_URID key,
  const LV2_Atom* value)
{
  g_message ("property changed");
  Lv2Control* control =
    lv2_get_property_control (
      &plugin->controls, key);
  if (control)
    {
      control_changed (
        plugin,
        (PluginGtkController*)control->widget,
        value->size,
        value->type,
        value + 1);
    }
}

/**
 * Called to deliver a port event to the plugin
 * UI.
 *
 * This applies to both generic and custom UIs.
 */
void
lv2_gtk_ui_port_event (
  Lv2Plugin *  plugin,
  uint32_t     port_index,
  uint32_t     buffer_size,
  uint32_t     protocol,
  const void * buffer)
{
  if (plugin->ui_instance)
    {
      suil_instance_port_event (
        plugin->ui_instance, port_index,
        buffer_size, protocol, buffer);
      return;
    }
  else if (protocol == 0 &&
           (PluginGtkController*)plugin->ports[
             port_index].widget)
    {
      control_changed (
        plugin,
        (PluginGtkController*)plugin->ports[port_index].widget,
        buffer_size,
        plugin->forge.Float,
        buffer);
      return;
    }
  else if (protocol == 0)
    return;  // No widget (probably notOnGUI)
  else if (protocol !=
           PM_URIDS.atom_eventTransfer)
    {
      g_warning ("Unknown port event protocol");
      return;
    }

  const LV2_Atom* atom = (const LV2_Atom*)buffer;
  if (lv2_atom_forge_is_object_type (
        &plugin->forge, atom->type))
    {
      plugin->updating = true;
      const LV2_Atom_Object* obj =
        (const LV2_Atom_Object*)buffer;
      if (obj->body.otype ==
          PM_URIDS.patch_Set)
        {
          const LV2_Atom_URID* property = NULL;
          const LV2_Atom*      value    = NULL;
          if (!patch_set_get (
                 plugin, obj, &property, &value))
            property_changed (
              plugin, property->body, value);
        }
      else if (obj->body.otype ==
               PM_URIDS.patch_Put)
        {
          const LV2_Atom_Object* body = NULL;
          if (!patch_put_get(plugin, obj, &body))
            {
              LV2_ATOM_OBJECT_FOREACH (body, prop)
                {
                  property_changed (
                    plugin, prop->key, &prop->value);
                }
            }
        }
      else
        g_warning ("Unknown object type?");
      plugin->updating = false;
    }
}

static gboolean
scale_changed (
  GtkRange* range, gpointer data)
{
  /*g_message ("scale changed");*/
  lv2_gtk_set_float_control (
    (const Lv2Control*)data,
    gtk_range_get_value(range));
  return FALSE;
}

static gboolean
spin_changed (
  GtkSpinButton* spin, gpointer data)
{
  const Lv2Control* control =
    (const Lv2Control*)data;
  PluginGtkController* controller =
    (PluginGtkController*)control->widget;
  GtkRange* range =
    GTK_RANGE (controller->control);
  const double value =
    gtk_spin_button_get_value(spin);
  if (!math_doubles_equal (
        gtk_range_get_value (range), value))
    gtk_range_set_value (range, value);

  return FALSE;
}

static gboolean
log_scale_changed (GtkRange* range, gpointer data)
{
  /*g_message ("log scale changed");*/
  lv2_gtk_set_float_control (
    (const Lv2Control*)data,
    expf (gtk_range_get_value (range)));

  return FALSE;
}

static gboolean
log_spin_changed (
  GtkSpinButton* spin, gpointer data)
{
  const Lv2Control* control =
    (const Lv2Control*)data;
  PluginGtkController* controller =
    (PluginGtkController*)control->widget;
  GtkRange* range =
    GTK_RANGE(controller->control);
  const double value =
    gtk_spin_button_get_value (spin);
  if (!math_doubles_equal (
        gtk_range_get_value (range),
        logf (value)))
    gtk_range_set_value (range, logf(value));

  return FALSE;
}

static void
combo_changed (GtkComboBox* box, gpointer data)
{
  const Lv2Control* control =
    (const Lv2Control*)data;

  GtkTreeIter iter;
  if (gtk_combo_box_get_active_iter (box, &iter))
    {
      GtkTreeModel* model =
        gtk_combo_box_get_model (box);
      GValue value = { 0, { { 0 } } };

      gtk_tree_model_get_value (
        model, &iter, 0, &value);
      const double v = g_value_get_float(&value);
      g_value_unset(&value);

      lv2_gtk_set_float_control(control, v);
    }
}

static gboolean
switch_state_set (
  GtkSwitch * button,
  gboolean    state,
  gpointer    data)
{
  /*g_message ("toggle_changed");*/
  lv2_gtk_set_float_control (
    (const Lv2Control*)data,
    state ? 1.0f : 0.0f);
  return FALSE;
}

static void
string_changed (
  GtkEntry* widget, gpointer data)
{
  Lv2Control* control = (Lv2Control*)data;
  const char* string =
    gtk_entry_get_text(widget);

  set_control (control,
               strlen(string) + 1,
               control->plugin->forge.String,
               string);
}

static void
file_changed (
  GtkFileChooserButton* widget,
  gpointer              data)
{
  Lv2Control* control = (Lv2Control*)data;
  Lv2Plugin*       plugin     = control->plugin;
  const char* filename = gtk_file_chooser_get_filename(
    GTK_FILE_CHOOSER(widget));

  set_control(control, strlen(filename), plugin->forge.Path, filename);
}

static PluginGtkController*
new_controller(GtkSpinButton* spin, GtkWidget* control)
{
  PluginGtkController* controller = (PluginGtkController*)calloc(1, sizeof(PluginGtkController));
  controller->spin    = spin;
  controller->control = control;
  return controller;
}

static PluginGtkController*
make_combo(Lv2Control* record, float value)
{
  GtkListStore* list_store = gtk_list_store_new(
    2, G_TYPE_FLOAT, G_TYPE_STRING);
  int active = -1;
  for (size_t i = 0; i < record->n_points; ++i) {
    const Lv2ScalePoint* point = &record->points[i];
    GtkTreeIter       iter;
    gtk_list_store_append(list_store, &iter);
    gtk_list_store_set(list_store, &iter,
                       0, point->value,
                       1, point->label,
                       -1);
    if (fabs(value - point->value) < FLT_EPSILON) {
      active = i;
    }
  }

  GtkWidget* combo = gtk_combo_box_new_with_model(GTK_TREE_MODEL(list_store));
  gtk_combo_box_set_active(GTK_COMBO_BOX(combo), active);
  g_object_unref(list_store);

  gtk_widget_set_sensitive(combo, record->is_writable);

  GtkCellRenderer* cell = gtk_cell_renderer_text_new();
  gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(combo), cell, TRUE);
  gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(combo), cell, "text", 1, NULL);

  if (record->is_writable) {
    g_signal_connect (
      G_OBJECT (combo), "changed",
      G_CALLBACK (combo_changed), record);
  }

  return new_controller(NULL, combo);
}

static PluginGtkController*
make_log_slider(Lv2Control* record, float value)
{
  const float min =
    lilv_get_float_from_node_with_fallback (
      record->min, 0.0f);
  const float max =
    lilv_get_float_from_node_with_fallback (
      record->max, 1.0f);
  const float lmin = logf (min);
  const float lmax = logf (max);
  const float ldft = logf (value);
  GtkWidget*  scale =
    gtk_scale_new_with_range (
      GTK_ORIENTATION_HORIZONTAL,
      lmin, lmax, 0.001);
  gtk_widget_set_size_request (
    scale, MIN_SCALE_WIDTH, -1);
  GtkWidget*  spin  =
    gtk_spin_button_new_with_range (
      min, max, 0.000001);

  gtk_widget_set_sensitive (
    scale, record->is_writable);
  gtk_widget_set_sensitive (
    spin, record->is_writable);

  gtk_widget_set_hexpand (scale, 1);

  gtk_scale_set_draw_value (
    GTK_SCALE (scale), FALSE);
  gtk_range_set_value (
    GTK_RANGE (scale), ldft);
  gtk_spin_button_set_value (
    GTK_SPIN_BUTTON (spin), value);

  if (record->is_writable)
    {
      g_signal_connect (
        G_OBJECT (scale), "value-changed",
        G_CALLBACK (log_scale_changed), record);
      g_signal_connect (
        G_OBJECT (spin), "value-changed",
        G_CALLBACK (log_spin_changed), record);
    }

  return
    new_controller (
      GTK_SPIN_BUTTON (spin), scale);
}

static PluginGtkController*
make_slider(Lv2Control* record, float value)
{
  const float min =
    lilv_get_float_from_node_with_fallback (
      record->min, 0.0f);
  const float max =
    lilv_get_float_from_node_with_fallback (
      record->max, 1.0f);
  const double step  = record->is_integer ? 1.0 : ((max - min) / 100.0);
  GtkWidget * scale =
    gtk_scale_new_with_range (
      GTK_ORIENTATION_HORIZONTAL, min, max, step);
  gtk_widget_set_size_request (
    scale, MIN_SCALE_WIDTH, -1);
  GtkWidget * spin =
    gtk_spin_button_new_with_range (min, max, step);

  gtk_widget_set_sensitive (
    scale, record->is_writable);
  gtk_widget_set_sensitive (
    spin, record->is_writable);

  gtk_widget_set_hexpand (scale, 1);

  if (record->is_integer) {
          gtk_spin_button_set_digits(GTK_SPIN_BUTTON(spin), 0);
  } else {
          gtk_spin_button_set_digits(GTK_SPIN_BUTTON(spin), 7);
  }

  gtk_scale_set_draw_value(GTK_SCALE(scale), FALSE);
  gtk_range_set_value(GTK_RANGE(scale), value);
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(spin), value);
  if (record->points) {
          for (size_t i = 0; i < record->n_points; ++i) {
                  const Lv2ScalePoint* point = &record->points[i];

                  gchar* str = g_markup_printf_escaped(
                          "<span font_size=\"small\">%s</span>", point->label);
                  gtk_scale_add_mark(
                          GTK_SCALE(scale), point->value, GTK_POS_TOP, str);
          }
  }

  if (record->is_writable) {
          g_signal_connect(G_OBJECT(scale), "value-changed",
                           G_CALLBACK(scale_changed), record);
          g_signal_connect(G_OBJECT(spin), "value-changed",
                           G_CALLBACK(spin_changed), record);
  }

  return new_controller(GTK_SPIN_BUTTON(spin), scale);
}

static PluginGtkController*
make_toggle(Lv2Control* record, float value)
{
  GtkWidget * check = gtk_switch_new ();
  gtk_widget_set_halign (check, GTK_ALIGN_START);

  gtk_widget_set_sensitive (
    check, record->is_writable);

  if (value)
    {
      gtk_switch_set_active (
        GTK_SWITCH (check), TRUE);
    }

  if (record->is_writable)
    {
      g_signal_connect (
        G_OBJECT (check), "state-set",
        G_CALLBACK (switch_state_set), record);
    }

  return new_controller (NULL, check);
}

static PluginGtkController*
make_entry(Lv2Control* control)
{
  GtkWidget* entry = gtk_entry_new();

  gtk_widget_set_sensitive(entry, control->is_writable);
  if (control->is_writable) {
    g_signal_connect(G_OBJECT(entry), "activate",
                     G_CALLBACK(string_changed), control);
  }

  return new_controller(NULL, entry);
}

static PluginGtkController*
make_file_chooser(Lv2Control* record)
{
  GtkWidget* button = gtk_file_chooser_button_new(
    "Open File", GTK_FILE_CHOOSER_ACTION_OPEN);

  gtk_widget_set_sensitive(button, record->is_writable);

  if (record->is_writable) {
    g_signal_connect(G_OBJECT(button), "file-set",
                     G_CALLBACK(file_changed), record);
  }

  return new_controller(NULL, button);
}

static PluginGtkController*
make_controller (
  Lv2Control * control, float value)
{
  PluginGtkController* controller = NULL;

  if (control->is_toggle) {
    controller = make_toggle (control, value);
  } else if (control->is_enumeration) {
    controller = make_combo (control, value);
  } else if (control->is_logarithmic) {
    controller = make_log_slider (control, value);
  } else {
    controller = make_slider (control, value);
  }

  if (controller)
    {
      g_return_val_if_fail (control->port, NULL);
      controller->port = control->port->port;
    }

  return controller;
}

static int
control_group_cmp(const void* p1, const void* p2, void* data)
{
  const Lv2Control* control1 = *(const Lv2Control**)p1;
  const Lv2Control* control2 = *(const Lv2Control**)p2;

  const int cmp = (control1->group && control2->group)
    ? strcmp(lilv_node_as_string(control1->group),
             lilv_node_as_string(control2->group))
    : ((intptr_t)control1->group - (intptr_t)control2->group);

  return cmp;
}

GtkWidget*
lv2_gtk_build_control_widget (
  Lv2Plugin* plugin, GtkWindow* window)
{
  GtkWidget* port_table =
    gtk_grid_new ();

  /* Make an array of controls sorted by group */
  GArray* controls =
    g_array_new (FALSE, TRUE, sizeof(Lv2Control*));
  int i;
  for (i = 0;
       i < plugin->controls.n_controls; ++i)
    {
      g_array_append_vals (
        controls, &plugin->controls.controls[i], 1);
    }
  g_array_sort_with_data (
    controls, control_group_cmp, plugin);

  /* Add controls in group order */
  LilvNode* last_group = NULL;
  int n_rows = 0;
  int num_ctrls = controls->len;
  for (i = 0; i < num_ctrls; ++i)
    {
      Lv2Control * record =
        g_array_index (controls, Lv2Control*, i);
      PluginGtkController * controller = NULL;
      LilvNode * group = record->group;

      /* Check group and add new heading if
       * necessary */
      if (group &&
          !lilv_node_equals (group, last_group))
        {
          LilvNode* group_name =
            lilv_world_get (
              LILV_WORLD, group,
              PM_LILV_NODES.core_name, NULL);
          GtkWidget* group_label =
            plugin_gtk_new_label (
              lilv_node_as_string (group_name),
              true, false, 0.0f, 1.0f);
          gtk_grid_attach (
            GTK_GRID (port_table), group_label,
            0, n_rows, 2, 1);
          ++n_rows;
        }
      last_group = group;

      /* Make control widget */
      if (record->value_type ==
            plugin->forge.String)
        {
          controller = make_entry (record);
        }
      else if (record->value_type ==
               plugin->forge.Path)
        {
          controller = make_file_chooser (record);
        }
      else
        {
          const float val =
            lilv_get_float_from_node_with_fallback (
              record->def, 0.0f);
          controller =
            make_controller (record, val);
        }

      record->widget = controller;
      if (record->type == PORT)
        plugin->ports[record->index].widget =
          controller;
      if (controller)
        {
          /* Add row to table for this controller */
          plugin_gtk_add_control_row (
            port_table, n_rows++,
            (record->label
             ? lilv_node_as_string (record->label)
             : lilv_node_as_uri (record->node)),
            controller);

          /* Set tooltip text from comment,
           * if available */
          LilvNode* comment =
            lilv_world_get (
              LILV_WORLD, record->node,
              PM_LILV_NODES.rdfs_comment, NULL);
          if (comment)
            {
              gtk_widget_set_tooltip_text (
                controller->control,
                lilv_node_as_string(comment));
            }
          lilv_node_free(comment);
        }
    }

  if (n_rows > 0)
    {
      gtk_window_set_resizable (
        GTK_WINDOW(window), TRUE);
      GtkWidget* box =
        gtk_box_new (
          GTK_ORIENTATION_HORIZONTAL, 0.0);
      /*gtk_alignment_set_padding(GTK_ALIGNMENT(alignment), 0, 0, 8, 8);*/
      gtk_widget_set_margin_start (
        GTK_WIDGET (port_table), 8);
      gtk_widget_set_margin_end (
        GTK_WIDGET (port_table), 8);
      gtk_container_add (
        GTK_CONTAINER (box), port_table);
      return box;
    }
  else
    {
      gtk_widget_destroy (port_table);
      GtkWidget* button =
        gtk_button_new_with_label (_("Close"));
      g_signal_connect_swapped (
        button, "clicked",
        G_CALLBACK (gtk_widget_destroy), window);
      gtk_window_set_resizable (
        GTK_WINDOW(window), FALSE);
      return button;
    }
}

void
on_external_ui_closed(void* controller)
{
  g_message ("External LV2 UI closed");
  Lv2Plugin* self = (Lv2Plugin *) controller;
  self->plugin->visible = 0;
  lv2_gtk_close_ui (self);
}

/**
 * Called on each GUI frame to update the GTK UI.
 *
 * @note This is a GSourceFunc.
 */
static int
update_plugin_ui (
  Lv2Plugin * plugin)
{
  /* Check quit flag and close if set. */
  if (zix_sem_try_wait (&plugin->exit_sem))
    {
      lv2_gtk_close_ui (plugin);
      return G_SOURCE_REMOVE;
    }

  /* Emit UI events. */
  if (plugin->plugin->visible &&
       (plugin->plugin->window ||
        plugin->external_ui_widget))
    {
      Lv2ControlChange ev;
      const size_t  space =
        zix_ring_read_space (
          plugin->plugin_to_ui_events);
      for (size_t i = 0;
           i + sizeof(ev) < space;
           i += sizeof(ev) + ev.size)
        {
          /* Read event header to get the size */
          zix_ring_read (
            plugin->plugin_to_ui_events,
            (char*)&ev, sizeof(ev));

          /* Resize read buffer if necessary */
          plugin->ui_event_buf =
            realloc (plugin->ui_event_buf, ev.size);
          void* const buf = plugin->ui_event_buf;

          /* Read event body */
          zix_ring_read (
            plugin->plugin_to_ui_events,
            (char*)buf, ev.size);

#if 0
          if (ev.protocol ==
                PM_URIDS.atom_eventTransfer)
            {
              /* Dump event in Turtle to the
               * console */
              LV2_Atom * atom = (LV2_Atom *) buf;
              char * str =
                sratom_to_turtle (
                  plugin->ui_sratom,
                  &plugin->unmap,
                  "plugin:", NULL, NULL,
                  atom->type, atom->size,
                  LV2_ATOM_BODY(atom));
              g_message (
                "Event from plugin to its UI "
                "(%u bytes): %s",
                atom->size, str);
              free(str);
            }
#endif

          lv2_gtk_ui_port_event (
            plugin, ev.index,
            ev.size, ev.protocol,
            ev.protocol == 0 ?
            (void *)
              &plugin->ports[ev.index].
                port->control :
            buf);

#if 0
          if (ev.protocol == 0)
            {
              float val = * (float *) buf;
              g_message (
                "%s = %f",
                lv2_port_get_symbol_as_string (
                  plugin,
                  &plugin->ports[ev.index]),
                (double) val);
            }
#endif
      }

      if (plugin->has_external_ui &&
          plugin->external_ui_widget)
        {
          plugin->external_ui_widget->run (
            plugin->external_ui_widget);
        }
    }

  return G_SOURCE_CONTINUE;
}

int
lv2_gtk_open_ui (
  Lv2Plugin* plugin)
{
  /* create a window if necessary */
  if (plugin->has_external_ui)
    {
      plugin->plugin->window = NULL;
      plugin->plugin->ev_box = NULL;
      plugin->plugin->vbox = NULL;
    }
  else
    {
      plugin_gtk_create_window (plugin->plugin);
    }

  /* Attempt to instantiate custom UI if
   * necessary */
  if (plugin->ui &&
      !g_settings_get_boolean (
        S_P_PLUGINS_UIS, "generic"))
    {
      if (plugin->has_external_ui)
        {
          g_message (
            "Instantiating external UI...");

          LV2_External_UI_Host extui;
          extui.ui_closed = on_external_ui_closed;
          LilvNode* name =
            lilv_plugin_get_name (
              plugin->lilv_plugin);
          extui.plugin_human_id =
            lilv_node_as_string (name);
          lilv_node_free (name);
          lv2_ui_instantiate (
            plugin,
            lilv_node_as_uri (plugin->ui_type),
            &extui);
        }
      else
        {
          g_message ("Instantiating UI...");
          lv2_ui_instantiate (
            plugin, LV2_UI__Gtk3UI,
            plugin->plugin->ev_box);
        }
    }

  /* present the window */
  if (plugin->has_external_ui &&
      plugin->external_ui_widget)
    {
      plugin->external_ui_widget->show (
        plugin->external_ui_widget);
    }
  else if (plugin->ui_instance)
    {
      g_message ("Creating suil window for UI...");
      GtkWidget* widget =
        GTK_WIDGET (
          suil_instance_get_widget (
            plugin->ui_instance));

      /* suil already adds the widget to the
       * container in win_in_gtk3 but it doesn't
       * in x11_in_gtk3 */
#ifndef _WOE32
      gtk_container_add (
        GTK_CONTAINER (plugin->plugin->ev_box),
        widget);
#endif
      gtk_window_set_resizable (
        GTK_WINDOW (plugin->plugin->window),
        lv2_ui_is_resizable (plugin));
      gtk_widget_show_all (
        GTK_WIDGET (plugin->plugin->ev_box));
      gtk_widget_grab_focus (widget);
      gtk_window_present (
        GTK_WINDOW (plugin->plugin->window));
    }
  else
    {
      g_message (
        "No UI found, creating generic GTK "
        "window..");
      GtkWidget* controls =
        lv2_gtk_build_control_widget (
          plugin, plugin->plugin->window);
      GtkWidget* scroll_win =
        gtk_scrolled_window_new (NULL, NULL);
      gtk_container_add (
        GTK_CONTAINER (scroll_win), controls);
      gtk_scrolled_window_set_policy (
        GTK_SCROLLED_WINDOW (scroll_win),
        GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
      gtk_container_add (
        GTK_CONTAINER (plugin->plugin->ev_box),
        scroll_win);
      gtk_widget_show_all (
        GTK_WIDGET (plugin->plugin->vbox));

      GtkRequisition controls_size, box_size;
      gtk_widget_get_preferred_size (
        GTK_WIDGET (controls), NULL,
        &controls_size);
      gtk_widget_get_preferred_size (
        GTK_WIDGET (plugin->plugin->vbox), NULL,
        &box_size);

      gtk_window_set_default_size (
        GTK_WINDOW (plugin->plugin->window),
        MAX (
          MAX (
            box_size.width,
            controls_size.width) + 24,
          640),
        box_size.height + controls_size.height);
      gtk_window_present (
        GTK_WINDOW (plugin->plugin->window));
  }

  if (!plugin->has_external_ui)
    {
      g_return_val_if_fail (
        plugin->lilv_plugin, -1);
    }

  lv2_ui_init (plugin);

  plugin->plugin->ui_instantiated = 1;
  EVENTS_PUSH (
    ET_PLUGIN_VISIBILITY_CHANGED,
    plugin->plugin);

  g_message (
    "plugin window shown, adding idle timeout. "
    "Update frequency (Hz): %.01f",
    (double) plugin->plugin->ui_update_hz);
  g_return_val_if_fail (
    plugin->plugin->ui_update_hz >=
      PLUGIN_MIN_REFRESH_RATE, -1);

  g_timeout_add (
    (int) (1000.f / plugin->plugin->ui_update_hz),
    (GSourceFunc) update_plugin_ui, plugin);

  return 0;
}

int
lv2_gtk_close_ui (
  Lv2Plugin* plugin)
{
  g_message ("%s called", __func__);
  if (plugin->plugin->window)
    {
      plugin_gtk_close_window (plugin->plugin);
    }

  if (plugin->has_external_ui &&
      plugin->external_ui_widget)
    {
      g_message ("hiding external UI");
      plugin->external_ui_widget->hide (
        plugin->external_ui_widget);
    }

  return TRUE;
}
