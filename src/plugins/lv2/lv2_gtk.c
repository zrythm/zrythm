/*
 * Copyright (C) 2018-2019 Alexandros Theodotou <alex at zrythm dot org>
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
#include "gui/widgets/instrument_track.h"
#include "gui/widgets/main_window.h"
#include "plugins/lv2_plugin.h"
#include "plugins/lv2/lv2_gtk.h"
#include "plugins/lv2/lv2_state.h"
#include "plugins/lv2/lv2_ui.h"
#include "plugins/plugin_manager.h"
#include "project.h"
#include "zrythm.h"

#include <gtk/gtk.h>

#include <glib/gi18n.h>
#include <lv2/lv2plug.in/ns/ext/patch/patch.h>
#include <lv2/lv2plug.in/ns/ext/port-props/port-props.h>
#include <suil/suil.h>

#if GTK_MAJOR_VERSION == 3
#if defined(__clang__)
#    pragma clang diagnostic push
#    pragma clang diagnostic ignored "-Wdeprecated-declarations"
#elif __GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 6)
#    pragma GCC diagnostic push
#    pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif
#endif

bool no_menu = false;
bool print_controls = 0;
static const int dump = 0;

/** Widget for a control. */
typedef struct {
  GtkSpinButton* spin;
  GtkWidget*     control;
} Controller;

static float
get_float (const LilvNode* node, float fallback)
{
  if (lilv_node_is_float(node) ||
      lilv_node_is_int(node))
    return lilv_node_as_float(node);

  return fallback;
}

static GtkWidget*
new_box (gboolean horizontal, gint spacing)
{
  return gtk_box_new (
    horizontal ?
    GTK_ORIENTATION_HORIZONTAL :
    GTK_ORIENTATION_VERTICAL,
    spacing);
}

static GtkWidget*
new_hscale (
  gdouble min, gdouble max, gdouble step)
{
  return gtk_scale_new_with_range (
    GTK_ORIENTATION_HORIZONTAL, min, max, step);
}

static void
size_request (
  GtkWidget* widget, GtkRequisition* req)
{
  gtk_widget_get_preferred_size (widget, NULL, req);
}

/**
 * Called both on generic UIs and normal UIs when
 * a plugin window is destroyed.
 */
static void
on_window_destroy(
  GtkWidget* widget,
  gpointer   data)
{
  Lv2Plugin * plugin = (Lv2Plugin *) data;
  plugin->window = NULL;
  g_message ("destroying");

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
}

static void
on_save_activate (
  GtkWidget* widget, void* ptr)
{
  Lv2Plugin* plugin = (Lv2Plugin*)ptr;
  GtkWidget* dialog = gtk_file_chooser_dialog_new(
    _("Save State"),
    (GtkWindow*)plugin->window,
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
    lv2_state_save (plugin, base);
    g_free(path);
    g_free(base);
  }

  gtk_widget_destroy(dialog);
}

static void
on_quit_activate (
  GtkWidget* widget, gpointer data)
{
  GtkWidget* window = (GtkWidget*)data;
  gtk_widget_destroy(window);
}

typedef struct {
  Lv2Plugin*     plugin;
  LilvNode* preset;
} PresetRecord;

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

static void
set_window_title (Lv2Plugin* plugin)
{
  g_return_if_fail (
    plugin && plugin->plugin &&
    plugin->plugin->track &&
    plugin->plugin->track->name);
  const char* track_name =
    plugin->plugin->track->name;
  const char* plugin_name =
    plugin->plugin->descr->name;
  g_return_if_fail (track_name && plugin_name);

  char title[500];
  sprintf (
    title,
    "%s (%s)",
    track_name, plugin_name);
  if (plugin->preset)
    {
      const char* preset_label =
        lilv_state_get_label(plugin->preset);
      g_return_if_fail (preset_label);
      char preset_part[500];
      sprintf (
        preset_part, " - %s",
        preset_label);
      strcat (
        title, preset_part);
    }

  gtk_window_set_title (
    GTK_WINDOW(plugin->window),
    title);
}

static void
on_preset_activate (
  GtkWidget* widget, gpointer data)
{
  PresetRecord* record = (PresetRecord*)data;
  if (GTK_CHECK_MENU_ITEM(widget) !=
      record->plugin->active_preset_item)
    {
      lv2_state_apply_preset (
        record->plugin, record->preset);
      if (record->plugin->active_preset_item)
        {
          gtk_check_menu_item_set_active (
            record->plugin->active_preset_item,
            FALSE);
        }

      record->plugin->active_preset_item =
        GTK_CHECK_MENU_ITEM(widget);
      gtk_check_menu_item_set_active (
        record->plugin->active_preset_item, TRUE);
      set_window_title(record->plugin);
    }
}

static void
on_preset_destroy (
  gpointer data, GClosure* closure)
{
  PresetRecord* record =
    (PresetRecord*)data;
  lilv_node_free(record->preset);
  free(record);
}

typedef struct {
  GtkMenuItem* item;
  char*        label;
  GtkMenu*     menu;
  GSequence*   banks;
} PresetMenu;

static PresetMenu*
pset_menu_new(const char* label)
{
  PresetMenu* menu =
    (PresetMenu*) calloc (1, sizeof(PresetMenu));

  menu->label = g_strdup(label);
  menu->item =
    GTK_MENU_ITEM (
      gtk_menu_item_new_with_label(menu->label));
  menu->menu = GTK_MENU (gtk_menu_new());
  menu->banks = NULL;

  return menu;
}

static void
pset_menu_free (
  PresetMenu* menu)
{
  if (menu->banks)
    {
      for (GSequenceIter* i =
             g_sequence_get_begin_iter(menu->banks);
           !g_sequence_iter_is_end(i);
           i = g_sequence_iter_next(i))
        {
          PresetMenu* bank_menu =
            (PresetMenu*)g_sequence_get(i);
          pset_menu_free(bank_menu);
        }
      g_sequence_free(menu->banks);
    }

  free(menu->label);
  free(menu);
}

static gint
menu_cmp (
  gconstpointer a, gconstpointer b, gpointer data)
{
  return strcmp(((PresetMenu*)a)->label,
                ((PresetMenu*)b)->label);
}

static PresetMenu*
get_bank_menu (
  Lv2Plugin* plugin,
  PresetMenu* menu,
  const LilvNode* bank)
{
  LilvNode* label =
    lilv_world_get (
      LILV_WORLD, bank,
      PM_LILV_NODES.rdfs_label, NULL);

  const char* uri = lilv_node_as_string(bank);
  const char* str =
    label ? lilv_node_as_string(label) : uri;
  PresetMenu key = { NULL, (char*)str, NULL, NULL };
  GSequenceIter* i =
    g_sequence_lookup (
      menu->banks, &key, menu_cmp, NULL);
  if (!i)
    {
      PresetMenu* bank_menu = pset_menu_new(str);
      gtk_menu_item_set_submenu(bank_menu->item, GTK_WIDGET(bank_menu->menu));
      g_sequence_insert_sorted(menu->banks, bank_menu, menu_cmp, NULL);
      return bank_menu;
    }

  return (PresetMenu*) g_sequence_get (i);
}

static int
add_preset_to_menu (
  Lv2Plugin*      plugin,
  const LilvNode* node,
  const LilvNode* title,
  void*           data)
{
  PresetMenu* menu  = (PresetMenu*)data;
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
      plugin->active_preset_item =
        GTK_CHECK_MENU_ITEM(item);
    }

  LilvNode* bank =
    lilv_world_get (
      LILV_WORLD, node,
      PM_LILV_NODES.pset_bank, NULL);

  if (bank)
    {
      PresetMenu* bank_menu =
        get_bank_menu(plugin, menu, bank);
      gtk_menu_shell_append (
        GTK_MENU_SHELL(bank_menu->menu), item);
    }
  else
    gtk_menu_shell_append (
      GTK_MENU_SHELL(menu->menu), item);

  PresetRecord* record =
    (PresetRecord*)calloc(1, sizeof(PresetRecord));
  record->plugin = plugin;
  record->preset = lilv_node_duplicate(node);

  g_signal_connect_data (
    G_OBJECT(item), "activate",
    G_CALLBACK(on_preset_activate),
    record, on_preset_destroy,
    (GConnectFlags)0);

  return 0;
}

static void
finish_menu (PresetMenu* menu)
{
  for (GSequenceIter* i =
         g_sequence_get_begin_iter(menu->banks);
       !g_sequence_iter_is_end(i);
       i = g_sequence_iter_next(i))
    {
      PresetMenu* bank_menu =
        (PresetMenu*)g_sequence_get(i);
      gtk_menu_shell_append (
        GTK_MENU_SHELL(menu->menu),
        GTK_WIDGET(bank_menu->item));
    }

  g_sequence_free(menu->banks);
}

static void
rebuild_preset_menu (
  Lv2Plugin* plugin, GtkContainer* pset_menu)
{
  // Clear current menu
  plugin->active_preset_item = NULL;
  for (GList* items =
         g_list_nth (
           gtk_container_get_children (pset_menu),
           3);
       items;
       items = items->next)
    {
      gtk_container_remove (
        pset_menu, GTK_WIDGET(items->data));
    }

  // Load presets and build new menu
  PresetMenu menu =
    {
      NULL, NULL, GTK_MENU(pset_menu),
      g_sequence_new((GDestroyNotify)pset_menu_free)
    };
  lv2_state_load_presets (
    plugin, add_preset_to_menu, &menu);
  finish_menu (&menu);
  gtk_widget_show_all (GTK_WIDGET(pset_menu));
}

static void
on_save_preset_activate (
  GtkWidget* widget, void* ptr)
{
  Lv2Plugin* plugin = (Lv2Plugin*)ptr;

  GtkWidget* dialog = gtk_file_chooser_dialog_new(
    "Save Preset",
    (GtkWindow*)plugin->window,
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
  GtkBox* box = GTK_BOX(new_box(true, 8));
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
      rebuild_preset_menu (
        plugin,
        GTK_CONTAINER (
          gtk_widget_get_parent(widget)));
      set_window_title(plugin);

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

static void
on_delete_preset_activate (
  GtkWidget* widget, void* ptr)
{
  Lv2Plugin* plugin = (Lv2Plugin*)ptr;
  if (!plugin->preset)
    return;

  GtkWidget* dialog =
    gtk_dialog_new_with_buttons(
      _("Delete Preset?"),
      (GtkWindow*)plugin->window,
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
        GTK_RESPONSE_ACCEPT) {
    lv2_state_delete_current_preset (plugin);
    rebuild_preset_menu (
      plugin,
      GTK_CONTAINER (
        gtk_widget_get_parent(widget)));
  }

  lilv_state_free (plugin->preset);
  plugin->preset = NULL;
  set_window_title (plugin);

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

static bool
differ_enough (float a, float b)
{
  return fabsf(a - b) >= FLT_EPSILON;
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

  Controller* controller =
    (Controller*) control->widget;
  if (controller && controller->spin &&
      differ_enough (
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
  Controller* controller,
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
        (Controller*)control->widget,
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
           (Controller*)plugin->ports[
             port_index].widget)
    {
      control_changed (
        plugin,
        (Controller*)plugin->ports[port_index].widget,
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
      g_warning (
        "Unknown port event protocol");
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
  Controller* controller =
    (Controller*)control->widget;
  GtkRange* range =
    GTK_RANGE (controller->control);
  const double value =
    gtk_spin_button_get_value(spin);
  if (differ_enough (
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
  Controller* controller =
    (Controller*)control->widget;
  GtkRange* range =
    GTK_RANGE(controller->control);
  const double value =
    gtk_spin_button_get_value (spin);
  if (differ_enough (
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
toggle_changed (
  GtkToggleButton* button, gpointer data)
{
  /*g_message ("toggle_changed");*/
  lv2_gtk_set_float_control (
    (const Lv2Control*)data,
    gtk_toggle_button_get_active (
      button) ? 1.0f : 0.0f);
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

static Controller*
new_controller(GtkSpinButton* spin, GtkWidget* control)
{
  Controller* controller = (Controller*)calloc(1, sizeof(Controller));
  controller->spin    = spin;
  controller->control = control;
  return controller;
}

static Controller*
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

static Controller*
make_log_slider(Lv2Control* record, float value)
{
  const float min   = get_float(record->min, 0.0f);
  const float max   = get_float(record->max, 1.0f);
  const float lmin  = logf(min);
  const float lmax  = logf(max);
  const float ldft  = logf(value);
  GtkWidget*  scale = new_hscale(lmin, lmax, 0.001);
  GtkWidget*  spin  = gtk_spin_button_new_with_range(min, max, 0.000001);

  gtk_widget_set_sensitive(scale, record->is_writable);
  gtk_widget_set_sensitive(spin, record->is_writable);

  gtk_scale_set_draw_value(GTK_SCALE(scale), FALSE);
  gtk_range_set_value(GTK_RANGE(scale), ldft);
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(spin), value);

  if (record->is_writable) {
    g_signal_connect(G_OBJECT(scale), "value-changed",
                     G_CALLBACK(log_scale_changed), record);
    g_signal_connect(G_OBJECT(spin), "value-changed",
                     G_CALLBACK(log_spin_changed), record);
  }

  return new_controller(GTK_SPIN_BUTTON(spin), scale);
}

static Controller*
make_slider(Lv2Control* record, float value)
{
  const float  min   = get_float(record->min, 0.0f);
  const float  max   = get_float(record->max, 1.0f);
  const double step  = record->is_integer ? 1.0 : ((max - min) / 100.0);
  GtkWidget*   scale = new_hscale(min, max, step);
  GtkWidget*   spin  = gtk_spin_button_new_with_range(min, max, step);

  gtk_widget_set_sensitive(scale, record->is_writable);
  gtk_widget_set_sensitive(spin, record->is_writable);

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

static Controller*
make_toggle(Lv2Control* record, float value)
{
  GtkWidget* check = gtk_check_button_new();

  gtk_widget_set_sensitive(check, record->is_writable);

  if (value) {
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check), TRUE);
  }

  if (record->is_writable) {
    g_signal_connect(G_OBJECT(check), "toggled",
                     G_CALLBACK(toggle_changed), record);
  }

  return new_controller(NULL, check);
}

static Controller*
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

static Controller*
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

static Controller*
make_controller(Lv2Control* control, float value)
{
  Controller* controller = NULL;

  if (control->is_toggle) {
    controller = make_toggle(control, value);
  } else if (control->is_enumeration) {
    controller = make_combo(control, value);
  } else if (control->is_logarithmic) {
    controller = make_log_slider(control, value);
  } else {
    controller = make_slider(control, value);
  }

  return controller;
}

static GtkWidget*
new_label(const char* text, bool title, float xalign, float yalign)
{
  GtkWidget*  label = gtk_label_new(NULL);
  const char* fmt   = title ? "<span font_weight=\"bold\">%s</span>" : "%s:";
  gchar*      str   = g_markup_printf_escaped(fmt, text);
  gtk_label_set_markup(GTK_LABEL(label), str);
  g_free(str);
  gtk_misc_set_alignment(GTK_MISC(label), xalign, yalign);
  return label;
}

static void
add_control_row(GtkWidget*  table,
                int         row,
                const char* name,
                Controller* controller)
{
  GtkWidget* label = new_label(name, false, 1.0, 0.5);
  gtk_table_attach(GTK_TABLE(table),
                   label,
                   0, 1, row, row + 1,
                   GTK_FILL, (GtkAttachOptions)(GTK_FILL|GTK_EXPAND), 8, 1);
  int control_left_attach = 1;
  if (controller->spin) {
    control_left_attach = 2;
    gtk_table_attach(GTK_TABLE(table), GTK_WIDGET(controller->spin),
                     1, 2, row, row + 1,
                     GTK_FILL, GTK_FILL, 2, 1);
  }
  gtk_table_attach(GTK_TABLE(table), controller->control,
                   control_left_attach, 3, row, row + 1,
                   (GtkAttachOptions)(GTK_FILL|GTK_EXPAND), GTK_FILL, 2, 1);
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

static GtkWidget*
build_control_widget (
  Lv2Plugin* plugin, GtkWidget* window)
{
  GtkWidget* port_table =
    gtk_table_new (
      plugin->num_ports, 3, false);

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
      Lv2Control* record =
        g_array_index(controls, Lv2Control*, i);
      Controller* controller = NULL;
      LilvNode*   group      = record->group;

      /* Check group and add new heading if necessary */
      if (group &&
          !lilv_node_equals (group, last_group))
        {
          LilvNode* group_name =
            lilv_world_get (
              LILV_WORLD, group,
              PM_LILV_NODES.core_name, NULL);
          GtkWidget* group_label =
            new_label (
              lilv_node_as_string (group_name),
              true, 0.0f, 1.0f);
          gtk_table_attach (
            GTK_TABLE(port_table), group_label,
            0, 2, n_rows, n_rows + 1,
            GTK_FILL, GTK_FILL, 0, 6);
          ++n_rows;
        }
      last_group = group;

      /* Make control widget */
      if (record->value_type == plugin->forge.String)
        controller = make_entry(record);
      else if (record->value_type ==
               plugin->forge.Path)
        controller = make_file_chooser(record);
      else
        {
          const float val =
            get_float(record->def, 0.0f);
          controller = make_controller(record, val);
        }

      record->widget = controller;
      if (record->type == PORT)
        plugin->ports[record->index].widget =
          controller;
      if (controller)
        {
          /* Add row to table for this controller */
          add_control_row (
            port_table, n_rows++,
            (record->label
             ? lilv_node_as_string(record->label)
             : lilv_node_as_uri(record->node)),
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
      GtkWidget* alignment =
        gtk_alignment_new (0.5, 0.0, 1.0, 0.0);
      /*gtk_alignment_set_padding(GTK_ALIGNMENT(alignment), 0, 0, 8, 8);*/
      gtk_widget_set_margin_start (
        GTK_WIDGET (port_table),
        8);
      gtk_widget_set_margin_end (
        GTK_WIDGET (port_table),
        8);
      gtk_container_add (
        GTK_CONTAINER(alignment), port_table);
      return alignment;
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

static void
build_menu (
  Lv2Plugin* plugin,
  GtkWidget* window,
  GtkWidget* vbox)
{
  GtkWidget* menu_bar  = gtk_menu_bar_new();
  GtkWidget* file      = gtk_menu_item_new_with_mnemonic("_File");
  GtkWidget* file_menu = gtk_menu_new();

  GtkAccelGroup* ag = gtk_accel_group_new();
  gtk_window_add_accel_group(GTK_WINDOW(window), ag);

  GtkWidget* save =
    gtk_menu_item_new_with_mnemonic ("_Save");
  GtkWidget* quit =
    gtk_menu_item_new_with_mnemonic ("_Quit");

  gtk_menu_item_set_submenu(GTK_MENU_ITEM(file), file_menu);
  gtk_menu_shell_append(GTK_MENU_SHELL(file_menu), save);
  gtk_menu_shell_append(GTK_MENU_SHELL(file_menu), quit);
  gtk_menu_shell_append(GTK_MENU_SHELL(menu_bar), file);

  GtkWidget* pset_item   = gtk_menu_item_new_with_mnemonic("_Presets");
  GtkWidget* pset_menu   = gtk_menu_new();
  GtkWidget* save_preset = gtk_menu_item_new_with_mnemonic(
          "_Save Preset...");
  GtkWidget* delete_preset = gtk_menu_item_new_with_mnemonic(
          "_Delete Current Preset...");
  gtk_menu_item_set_submenu(GTK_MENU_ITEM(pset_item), pset_menu);
  gtk_menu_shell_append(GTK_MENU_SHELL(pset_menu), save_preset);
  gtk_menu_shell_append(GTK_MENU_SHELL(pset_menu), delete_preset);
  gtk_menu_shell_append(GTK_MENU_SHELL(pset_menu),
                        gtk_separator_menu_item_new());
  gtk_menu_shell_append(GTK_MENU_SHELL(menu_bar), pset_item);

  PresetMenu menu = {
          NULL, NULL, GTK_MENU(pset_menu),
          g_sequence_new((GDestroyNotify)pset_menu_free)
  };
  lv2_state_load_presets (
    plugin, add_preset_to_menu, &menu);
  finish_menu(&menu);

  /* connect signals */
  g_signal_connect (
    G_OBJECT(quit), "activate",
    G_CALLBACK(on_quit_activate), window);
  g_signal_connect (
    G_OBJECT(save), "activate",
    G_CALLBACK(on_save_activate), plugin);
  g_signal_connect (
    G_OBJECT(save_preset), "activate",
    G_CALLBACK(on_save_preset_activate), plugin);
  g_signal_connect (
    G_OBJECT(delete_preset), "activate",
    G_CALLBACK(on_delete_preset_activate), plugin);

  gtk_box_pack_start (
    GTK_BOX(vbox), menu_bar, FALSE, FALSE, 0);
}

void
on_external_ui_closed(void* controller)
{
  Lv2Plugin* jalv = (Lv2Plugin *) controller;
  lv2_gtk_close_ui (jalv);
}

gboolean
on_delete_event (GtkWidget *widget,
               GdkEvent  *event,
               Lv2Plugin * plugin)
{
  plugin->plugin->visible = 0;
  plugin->window = NULL;
  EVENTS_PUSH (ET_PLUGIN_VISIBILITY_CHANGED,
               plugin->plugin);

  return FALSE;
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
  if (zix_sem_try_wait(&plugin->exit_sem))
    {
      lv2_gtk_close_ui(plugin);
      return G_SOURCE_REMOVE;
    }

  /* Emit UI events. */
  if (plugin->window)
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

          if (dump && ev.protocol ==
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

          /*if (ev.index == 2)*/
            /*g_message ("lv2_gtk_ui_port_event from "*/
                       /*"lv2 update");*/
          lv2_gtk_ui_port_event (
            plugin, ev.index,
            ev.size, ev.protocol,
            ev.protocol == 0 ?
            (void *)
              &plugin->ports[ev.index].
                port->control :
            buf);

          if (ev.protocol == 0 && print_controls)
            {
              float val = * (float *) buf;
              g_message (
                "%s = %f",
                lv2_port_get_symbol_as_string (
                  plugin,
                  &plugin->ports[ev.index]),
                (double) val);
            }
      }

      if (plugin->externalui && plugin->extuiptr) {
              LV2_EXTERNAL_UI_RUN(plugin->extuiptr);
      }
    }

  return G_SOURCE_CONTINUE;
}

int
lv2_gtk_open_ui (
  Lv2Plugin* plugin)
{
  GtkWidget* window =
    gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_icon_name (
    GTK_WINDOW (window),
    "zrythm");

  if (g_settings_get_int (
        S_PREFERENCES, "plugin-uis-stay-on-top"))
    gtk_window_set_transient_for (
      GTK_WINDOW (window),
      GTK_WINDOW (MAIN_WINDOW));

  plugin->window = window;
  plugin->delete_event_id =
    g_signal_connect (
      G_OBJECT (window), "delete-event",
      G_CALLBACK (on_delete_event), plugin);
  g_return_val_if_fail (plugin->lilv_plugin, -1);
  LilvNode* name =
    lilv_plugin_get_name (plugin->lilv_plugin);
  lilv_node_free (name);

  /* connect destroy signal */
  g_signal_connect (
    window, "destroy",
    G_CALLBACK (on_window_destroy), plugin);

  set_window_title(plugin);

  GtkWidget* vbox = new_box(false, 0);
  gtk_window_set_role (
    GTK_WINDOW (window), "plugin_ui");
  gtk_container_add (
    GTK_CONTAINER (window), vbox);

  if (!no_menu)
    build_menu (plugin, window, vbox);

  /* Create/show alignment to contain UI (whether
   * custom or generic) */
  GtkWidget* alignment =
    gtk_alignment_new (0.5, 0.5, 1.0, 1.0);
  gtk_box_pack_start (
    GTK_BOX (vbox), alignment, TRUE, TRUE, 0);
  gtk_widget_show (alignment);

  /* Attempt to instantiate custom UI if
   * necessary */
  if (plugin->ui && !LV2_NODES.opts.generic_ui)
    {
      if (plugin->externalui)
        {
          g_message ("Instantiating external UI...");

          LV2_External_UI_Host extui;
          extui.ui_closed = on_external_ui_closed;
          extui.plugin_human_id =
            lilv_node_as_string (name);
          lv2_ui_instantiate (
            plugin,
            lilv_node_as_uri (plugin->ui_type),
            &extui);
        }
      else
        {
          g_message ("Instantiating UI...");
          lv2_ui_instantiate (
            plugin,
            LV2_UI__Gtk3UI,
            alignment);
        }
    }

  if (plugin->externalui && plugin->extuiptr)
    {
      LV2_EXTERNAL_UI_SHOW(plugin->extuiptr);
    }
  else if (plugin->ui_instance)
    {
      g_message ("Creating window for UI...");
      GtkWidget* widget =
        GTK_WIDGET (
          suil_instance_get_widget (
            plugin->ui_instance));

      gtk_container_add (
        GTK_CONTAINER (alignment), widget);
      gtk_window_set_resizable (
        GTK_WINDOW (window),
        lv2_ui_is_resizable(plugin));
      gtk_widget_show_all(vbox);
      gtk_widget_grab_focus(widget);
      gtk_window_present(GTK_WINDOW(window));
    }
  else
    {
      g_message ("No UI found, building native..");
      GtkWidget* controls =
        build_control_widget (plugin, window);
      GtkWidget* scroll_win =
        gtk_scrolled_window_new (NULL, NULL);
      gtk_scrolled_window_add_with_viewport (
        GTK_SCROLLED_WINDOW (scroll_win), controls);
      gtk_scrolled_window_set_policy (
        GTK_SCROLLED_WINDOW (scroll_win),
        GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
      gtk_container_add (
        GTK_CONTAINER (alignment), scroll_win);
      gtk_widget_show_all(vbox);

      GtkRequisition controls_size, box_size;
      size_request (GTK_WIDGET(controls), &controls_size);
      size_request (GTK_WIDGET(vbox), &box_size);

      gtk_window_set_default_size (
        GTK_WINDOW(window),
        MAX (
          MAX (
            box_size.width,
            controls_size.width) + 24,
          640),
        box_size.height + controls_size.height);
      gtk_window_present (GTK_WINDOW(window));
  }

  lv2_ui_init (plugin);

  plugin->plugin->ui_instantiated = 1;

  g_message (
    "plugin window shown, adding idle timeout. "
    "Update frequency (Hz): %.01f",
    (double) plugin->ui_update_hz);

  g_timeout_add (
    (int) (1000.f / plugin->ui_update_hz),
    (GSourceFunc) update_plugin_ui, plugin);

  return 0;
}

int
lv2_gtk_close_ui (
  Lv2Plugin* plugin)
{
  if (plugin->window)
    {
      gtk_widget_set_sensitive (
        GTK_WIDGET (plugin->window),
        0);
      /*GtkWidget * widget =*/
        /*GTK_WIDGET (*/
          /*suil_instance_get_widget (*/
            /*(SuilInstance *) plugin->ui_instance));*/
      /*if (widget)*/
        /*gtk_widget_destroy (widget);*/
      gtk_window_close (
        GTK_WINDOW (plugin->window));
      /*gtk_widget_destroy (*/
        /*GTK_WIDGET (plugin->window));*/
    }
  return TRUE;
}

#if GTK_MAJOR_VERSION == 3
#if defined(__clang__)
#    pragma clang diagnostic pop
#elif __GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 6)
#    pragma GCC diagnostic pop
#endif
#endif
