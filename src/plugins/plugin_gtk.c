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
 *
 * This file incorporates work covered by the following copyright and
 * permission notice:
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
 */

#include "zrythm-config.h"

#include <math.h>

#include "audio/engine.h"
#include "audio/track.h"
#include "gui/backend/event.h"
#include "gui/backend/event_manager.h"
#include "gui/widgets/main_window.h"
#include "plugins/lv2/lv2_gtk.h"
#include "plugins/lv2/lv2_state.h"
#include "plugins/lv2/lv2_ui.h"
#include "plugins/lv2/lv2_urid.h"
#include "plugins/plugin.h"
#include "plugins/plugin_gtk.h"
#include "plugins/plugin_manager.h"
#include "settings/settings.h"
#include "project.h"
#include "utils/flags.h"
#include "utils/math.h"
#include "utils/objects.h"
#include "zrythm.h"
#include "zrythm_app.h"

#include <glib/gi18n.h>

#define MIN_SCALE_WIDTH 120

static void
on_quit_activate (
  GtkWidget* widget, gpointer data)
{
  GtkWidget* window = (GtkWidget*)data;
  gtk_widget_destroy (window);
}

static void
on_save_activate (
  GtkWidget* widget,
  Plugin * plugin)
{
  switch (plugin->setting->descr->protocol)
    {
    case PROT_LV2:
      g_return_if_fail (plugin->lv2);
      lv2_gtk_on_save_activate (plugin->lv2);
      break;
    case PROT_VST:
      break;
    default:
      break;
    }
#if 0
  GtkWidget* dialog = gtk_file_chooser_dialog_new(
    _("Save State"),
    (GtkWindow*)plugin->window,
    GTK_FILE_CHOOSER_ACTION_CREATE_FOLDER,
    _("_Cancel"), GTK_RESPONSE_CANCEL,
    _("_Save"), GTK_RESPONSE_ACCEPT,
    NULL);

  if (gtk_dialog_run (
        GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT)
    {
      char* path =
        gtk_file_chooser_get_filename (
          GTK_FILE_CHOOSER(dialog));
      char* base =
        g_build_filename (path, "/", NULL);

      switch (plugin->descr->protocol)
        {
        case PROT_LV2:
          lv2_state_save_to_file (plugin->lv2, base);
          break;
        case PROT_VST:
          break;
        default:
          break;
        }
      g_free(path);
      g_free(base);
    }
  gtk_widget_destroy(dialog);
#endif
}

void
plugin_gtk_set_window_title (
  Plugin *    plugin,
  GtkWindow * window)
{
  g_return_if_fail (
    plugin && plugin->setting->descr && window);

  char * title =
    plugin_generate_window_title (plugin);

  gtk_window_set_title (window, title);

  g_free (title);
}

void
plugin_gtk_on_preset_activate (
  GtkWidget* widget,
  PluginGtkPresetRecord * record)
{
  g_return_if_fail (record && record->plugin);
  Plugin * plugin = record->plugin;

  if (GTK_CHECK_MENU_ITEM (widget) !=
        plugin->active_preset_item)
    {
      switch (plugin->setting->descr->protocol)
        {
        case PROT_LV2:
          g_return_if_fail (plugin->lv2);
          lv2_state_apply_preset (
            plugin->lv2, (LilvNode *) record->preset);
          break;
        default:
          break;
        }
      if (plugin->active_preset_item)
        {
          gtk_check_menu_item_set_active (
            plugin->active_preset_item,
            FALSE);
        }

      plugin->active_preset_item =
        GTK_CHECK_MENU_ITEM (widget);
      gtk_check_menu_item_set_active (
        plugin->active_preset_item, TRUE);
      plugin_gtk_set_window_title (
        plugin, plugin->window);
    }
}

void
plugin_gtk_on_preset_destroy (
  PluginGtkPresetRecord * record,
  GClosure* closure)
{
  switch (record->plugin->setting->descr->protocol)
    {
    case PROT_LV2:
      lilv_node_free ((LilvNode *) record->preset);
      break;
    default:
      break;
    }
  free (record);
}

PluginGtkPresetMenu*
plugin_gtk_preset_menu_new (
  const char* label)
{
  PluginGtkPresetMenu* menu =
    object_new (PluginGtkPresetMenu);

  menu->label = g_strdup (label);
  menu->item =
    GTK_MENU_ITEM (
      gtk_menu_item_new_with_label(menu->label));
  menu->menu = GTK_MENU (gtk_menu_new());
  menu->banks = NULL;

  return menu;
}

static void
preset_menu_free (
  PluginGtkPresetMenu* menu)
{
  if (menu->banks)
    {
      for (GSequenceIter* i =
             g_sequence_get_begin_iter(menu->banks);
           !g_sequence_iter_is_end(i);
           i = g_sequence_iter_next(i))
        {
          PluginGtkPresetMenu* bank_menu =
            (PluginGtkPresetMenu*)g_sequence_get(i);
          preset_menu_free (bank_menu);
        }
      g_sequence_free(menu->banks);
    }

  free(menu->label);
  free(menu);
}

gint
plugin_gtk_menu_cmp (
  gconstpointer a, gconstpointer b, gpointer data)
{
  return strcmp(((PluginGtkPresetMenu*)a)->label,
                ((PluginGtkPresetMenu*)b)->label);
}

static void
finish_menu (PluginGtkPresetMenu* menu)
{
  for (GSequenceIter* i =
         g_sequence_get_begin_iter(menu->banks);
       !g_sequence_iter_is_end(i);
       i = g_sequence_iter_next(i))
    {
      PluginGtkPresetMenu* bank_menu =
        (PluginGtkPresetMenu*)g_sequence_get(i);
      gtk_menu_shell_append (
        GTK_MENU_SHELL(menu->menu),
        GTK_WIDGET(bank_menu->item));
    }

  g_sequence_free(menu->banks);
}

void
plugin_gtk_rebuild_preset_menu (
  Plugin* plugin,
  GtkContainer* pset_menu)
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
  PluginGtkPresetMenu menu =
    {
      NULL, NULL, GTK_MENU(pset_menu),
      g_sequence_new (
        (GDestroyNotify)preset_menu_free),
    };
  if (plugin->lv2)
    {
      lv2_state_load_presets (
        plugin->lv2, lv2_gtk_add_preset_to_menu,
        &menu);
    }
  finish_menu (&menu);
  gtk_widget_show_all (GTK_WIDGET(pset_menu));
}

static void
on_save_preset_activate (
  GtkWidget * widget,
  Plugin *    plugin)
{
  switch (plugin->setting->descr->protocol)
    {
    case PROT_LV2:
      lv2_gtk_on_save_preset_activate (
        widget, plugin->lv2);
      break;
    default:
      break;
    }
}

static void
on_delete_preset_activate (
  GtkWidget * widget,
  Plugin *    plugin)
{
  switch (plugin->setting->descr->protocol)
    {
    case PROT_LV2:
      lv2_gtk_on_delete_preset_activate (
        widget, plugin->lv2);
      break;
    default:
      break;
    }
}

/**
 * Creates a label for a control.
 *
 * @param title Whether this is a title text (makes
 *   it bold).
 * @param preformatted Whether the text is
 *   preformatted.
 */
GtkWidget*
plugin_gtk_new_label (
  const char * text,
  bool         title,
  bool         preformatted,
  float        xalign,
  float        yalign)
{
  GtkWidget * label = gtk_label_new(NULL);
  const char * fmt =
    title ?
      "<b>%s</b>" :
      "%s: ";
  gchar * str;
  if (preformatted)
    {
      str = g_strdup_printf (fmt, text);
    }
  else
    {
      str = g_markup_printf_escaped (fmt, text);
    }
  gtk_label_set_markup (GTK_LABEL(label), str);
  g_free (str);
  gtk_label_set_xalign (
    GTK_LABEL (label), xalign);
  gtk_label_set_yalign (
    GTK_LABEL (label), yalign);
  return label;
}

void
plugin_gtk_add_control_row (
  GtkWidget*  table,
  int         row,
  const char* _name,
  PluginGtkController* controller)
{
  char name[600];
  strcpy (name, _name);
  int preformatted = false;

  if (controller && controller->port)
    {
      PortIdentifier id = controller->port->id;

#define FORMAT_UNIT(caps) \
  case PORT_UNIT_##caps: \
    sprintf ( \
      name, "%s <small>(%s)</small>", \
      _name, port_unit_strings[id.unit].str); \
    break

      switch (id.unit)
        {
        FORMAT_UNIT (HZ);
        FORMAT_UNIT (DB);
        FORMAT_UNIT (DEGREES);
        FORMAT_UNIT (SECONDS);
        FORMAT_UNIT (MS);
        default:
          break;
        }

#undef FORMAT_UNIT

      preformatted = true;
    }

  GtkWidget* label =
    plugin_gtk_new_label (
      name, false, preformatted, 1.0, 0.5);
  gtk_grid_attach (
    GTK_GRID (table), label,
    0, row, 1, 1);
  int control_left_attach = 1;
  if (controller->spin)
    {
      control_left_attach = 2;
      gtk_grid_attach (
        GTK_GRID (table),
        GTK_WIDGET (controller->spin),
        1, row, 1, 1);
    }
  gtk_grid_attach (
    GTK_GRID (table), controller->control,
    control_left_attach, row, 3 - control_left_attach, 1);
}

static void
build_menu (
  Plugin* plugin,
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

  PluginGtkPresetMenu menu = {
    NULL, NULL, GTK_MENU (pset_menu),
    g_sequence_new (
      (GDestroyNotify) preset_menu_free)
  };
  if (plugin->lv2)
    {
      lv2_state_load_presets (
        plugin->lv2, lv2_gtk_add_preset_to_menu,
        &menu);
    }
  finish_menu (&menu);

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
    GTK_BOX (vbox), menu_bar, FALSE, FALSE, 0);
}

/**
 * Called when the plugin window is destroyed.
 */
static void
on_window_destroy (
  GtkWidget * widget,
  Plugin *    pl)
{
  g_return_if_fail (IS_PLUGIN_AND_NONNULL (pl));
  pl->window = NULL;
  g_message (
    "destroying window for %s",
    pl->setting->descr->name);

  /* reinit widget in plugin ports/parameters */
  for (int i = 0; i < pl->num_in_ports; i++)
    {
      Port * port = pl->in_ports[i];
      port->widget = NULL;
    }

  if (pl->lv2 &&
      pl->lv2->ui_instance)
    {
      suil_instance_free (pl->lv2->ui_instance);
      pl->lv2->ui_instance = NULL;
    }
}

static gboolean
on_delete_event (
  GtkWidget *widget,
  GdkEvent  *event,
  Plugin * plugin)
{
  plugin->visible = 0;
  plugin->window = NULL;
  EVENTS_PUSH (
    ET_PLUGIN_VISIBILITY_CHANGED, plugin);

  return FALSE;
}

/**
 * Creates a new GtkWindow that will be used to
 * either wrap plugin UIs or create generic UIs in.
 */
void
plugin_gtk_create_window (
  Plugin * plugin)
{
  g_message (
    "creating GTK window for %s",
    plugin->setting->descr->name);

  /* create window */
  plugin->window =
    GTK_WINDOW (
      gtk_window_new (GTK_WINDOW_TOPLEVEL));
  plugin_gtk_set_window_title (
    plugin, plugin->window);
  gtk_window_set_icon_name (
    plugin->window, "zrythm");
  gtk_window_set_role (
    plugin->window, "plugin_ui");

  if (g_settings_get_boolean (
        S_P_PLUGINS_UIS, "stay-on-top"))
    {
      gtk_window_set_transient_for (
        plugin->window,
        GTK_WINDOW (MAIN_WINDOW));
    }

  /* add vbox for stacking elements */
  plugin->vbox =
    GTK_BOX (
      gtk_box_new (GTK_ORIENTATION_VERTICAL, 0));
  gtk_container_add (
    GTK_CONTAINER (plugin->window),
    GTK_WIDGET (plugin->vbox));

  /* add menu bar */
  build_menu (
    plugin, GTK_WIDGET (plugin->window),
    GTK_WIDGET (plugin->vbox));

  /* Create/show alignment to contain UI (whether
   * custom or generic) */
  plugin->ev_box =
    GTK_EVENT_BOX (
      gtk_event_box_new ());
  gtk_box_pack_start (
    plugin->vbox, GTK_WIDGET (plugin->ev_box),
    TRUE, TRUE, 0);
  gtk_widget_show_all (GTK_WIDGET (plugin->vbox));

  /* connect signals */
  g_signal_connect (
    plugin->window, "destroy",
    G_CALLBACK (on_window_destroy), plugin);
  plugin->delete_event_id =
    g_signal_connect (
      G_OBJECT (plugin->window), "delete-event",
      G_CALLBACK (on_delete_event), plugin);
}

/**
 * Eventually called by the callbacks when the user
 * changes a widget value (e.g., the slider changed
 * callback) and updates the port values if plugin
 * is not updating.
 */
static void
set_lv2_control (
  Lv2Plugin *  lv2_plugin,
  Port *       port,
  uint32_t     size,
  LV2_URID     type,
  const void * body)
{
#if 0
  g_debug (
    "%s (%s) %s - updating %d",
    __FILE__, __func__,
    lilv_node_as_string (control->symbol),
    control->plugin->updating);
#endif

  if (lv2_plugin->updating)
    return;

  bool is_property =
    port->id.flags & PORT_FLAG_IS_PROPERTY;

  if (!is_property &&
      port->value_type == lv2_plugin->forge.Float)
    {
      g_debug (
        "setting float control '%s' to '%f'",
        port->id.sym,
        (double) * (float *) body);

      port->control = *(float*) body;
      port->unsnapped_control = *(float*)body;
    }
  else if (is_property)
    {
      g_debug (
        "setting property control '%s' to '%s'",
        port->id.sym,
        (char *) body);

      /* Copy forge since it is used by process
       * thread */
      LV2_Atom_Forge forge = lv2_plugin->forge;
      LV2_Atom_Forge_Frame frame;
      uint8_t buf[1024];

      /* forge patch set atom */
      lv2_atom_forge_set_buffer (
        &forge, buf, sizeof (buf));
      lv2_atom_forge_object (
        &forge, &frame, 0, PM_URIDS.patch_Set);
      lv2_atom_forge_key (
        &forge, PM_URIDS.patch_property);
      lv2_atom_forge_urid (
        &forge,
        lv2_urid_map_uri (
          lv2_plugin, port->id.uri));
      lv2_atom_forge_key (
        &forge, PM_URIDS.patch_value);
      lv2_atom_forge_atom (
        &forge, size, type);
      lv2_atom_forge_write (
        &forge, body, size);

      const LV2_Atom* atom =
        lv2_atom_forge_deref (
          &forge, frame.ref);
      g_return_if_fail (
        lv2_plugin->control_in >= 0 &&
        lv2_plugin->control_in < 400000);
      lv2_ui_send_event_from_ui_to_plugin (
        lv2_plugin,
        (uint32_t) lv2_plugin->control_in,
        lv2_atom_total_size (atom),
        PM_URIDS.atom_eventTransfer, atom);
    }
  else
    {
      g_warning ("control change not handled");
    }
}

/**
 * Called by generic UI callbacks when e.g. a slider
 * changes value.
 */
static void
set_float_control (
  Plugin * pl,
  Port *   port,
  float    value)
{
  if (pl->lv2)
    {
      Lv2Plugin * lv2_plugin = pl->lv2;

      LV2_URID type = port->value_type;

      if (type == lv2_plugin->forge.Int)
        {
          const int32_t ival = lrint (value);
          set_lv2_control (
            lv2_plugin, port, sizeof (ival), type,
            &ival);
        }
      else if (type == lv2_plugin->forge.Long)
        {
          const int64_t lval = lrint(value);
          set_lv2_control (
            lv2_plugin, port, sizeof (lval), type,
            &lval);
        }
      else if (type == lv2_plugin->forge.Float)
        {
          set_lv2_control (
            lv2_plugin, port, sizeof (value),
            type, &value);
        }
      else if (type == lv2_plugin->forge.Double)
        {
          const double dval = value;
          set_lv2_control (
            lv2_plugin, port, sizeof (dval), type,
            &dval);
        }
      else if (type == lv2_plugin->forge.Bool)
        {
          const int32_t ival = (int32_t) value;
          set_lv2_control (
            lv2_plugin, port, sizeof (ival), type,
            &ival);
        }
    }
  else if (pl->setting->open_with_carla)
    {
      port_set_control_value (
        port, value, F_NOT_NORMALIZED,
        F_PUBLISH_EVENTS);
    }

  PluginGtkController * controller =
    (PluginGtkController *) port->widget;
  if (controller && controller->spin &&
      !math_floats_equal (
        (float)
        gtk_spin_button_get_value (
          controller->spin),
        value))
    {
      gtk_spin_button_set_value (
        controller->spin, value);
    }
}

static gboolean
scale_changed (
  GtkRange * range,
  Port *     port)
{
  /*g_message ("scale changed");*/
  g_return_val_if_fail (
    IS_PORT_AND_NONNULL (port), false);
  Plugin * pl = port_get_plugin (port, true);
  g_return_val_if_fail (
    IS_PLUGIN_AND_NONNULL (pl), false);

  set_float_control (
    pl, port, (float) gtk_range_get_value (range));

  return FALSE;
}

static gboolean
spin_changed (
  GtkSpinButton * spin,
  Port *          port)
{
  PluginGtkController* controller =
    port->widget;
  GtkRange* range =
    GTK_RANGE (controller->control);
  const double value =
    gtk_spin_button_get_value (spin);
  if (!math_doubles_equal (
        gtk_range_get_value (range), value))
    {
      gtk_range_set_value (range, value);
    }

  return FALSE;
}

static gboolean
log_scale_changed (
  GtkRange* range,
  Port *     port)
{
  /*g_message ("log scale changed");*/
  g_return_val_if_fail (
    IS_PORT_AND_NONNULL (port), false);
  Plugin * pl = port_get_plugin (port, true);
  g_return_val_if_fail (
    IS_PLUGIN_AND_NONNULL (pl), false);

  set_float_control (
    pl, port,
    expf ((float) gtk_range_get_value (range)));

  return FALSE;
}

static gboolean
log_spin_changed (
  GtkSpinButton * spin,
  Port *          port)
{
  PluginGtkController * controller =
    port->widget;
  GtkRange * range =
    GTK_RANGE (controller->control);
  const double value =
    gtk_spin_button_get_value (spin);
  if (!math_floats_equal (
        (float) gtk_range_get_value (range),
        logf ((float) value)))
    {
      gtk_range_set_value (
        range, (double) logf ((float) value));
    }

  return FALSE;
}

static void
combo_changed (
  GtkComboBox * box,
  Port *        port)
{
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

      g_return_if_fail (
        IS_PORT_AND_NONNULL (port));
      Plugin * pl = port_get_plugin (port, true);
      g_return_if_fail (
        IS_PLUGIN_AND_NONNULL (pl));

      set_float_control (pl, port, (float) v);
    }
}

static gboolean
switch_state_set (
  GtkSwitch * button,
  gboolean    state,
  Port *      port)
{
  /*g_message ("toggle_changed");*/
  g_return_val_if_fail (
    IS_PORT_AND_NONNULL (port), false);
  Plugin * pl = port_get_plugin (port, true);
  g_return_val_if_fail (
    IS_PLUGIN_AND_NONNULL (pl), false);

  set_float_control (
    pl, port, state ? 1.0f : 0.0f);

  return FALSE;
}

static void
string_changed (
  GtkEntry * widget,
  Port *     port)
{
  const char* string =
    gtk_entry_get_text (widget);

  g_return_if_fail (
    IS_PORT_AND_NONNULL (port));
  Plugin * pl = port_get_plugin (port, true);
  g_return_if_fail (
    IS_PLUGIN_AND_NONNULL (pl));

  if (pl->lv2)
    {
      set_lv2_control (
        pl->lv2, port,
        strlen (string) + 1,
        pl->lv2->forge.String,
        string);
    }
  else
    {
      /* TODO carla */
    }
}

static void
file_changed (
  GtkFileChooserButton * widget,
  Port *                 port)
{
  const char* filename = gtk_file_chooser_get_filename(
    GTK_FILE_CHOOSER (widget));

  g_return_if_fail (
    IS_PORT_AND_NONNULL (port));
  Plugin * pl = port_get_plugin (port, true);
  g_return_if_fail (
    IS_PLUGIN_AND_NONNULL (pl));

  if (pl->lv2)
    {
      set_lv2_control (
        pl->lv2, port, strlen (filename),
        pl->lv2->forge.Path, filename);
    }
}

static PluginGtkController *
new_controller (
  GtkSpinButton * spin,
  GtkWidget *     control)
{
  PluginGtkController * controller =
    object_new (PluginGtkController);

  controller->spin = spin;
  controller->control = control;

  return controller;
}

static PluginGtkController *
make_combo (
  Port * port,
  float  value)
{
  GtkListStore* list_store =
    gtk_list_store_new (
      2, G_TYPE_FLOAT, G_TYPE_STRING);
  int active = -1;
  for (int i = 0; i < port->num_scale_points; i++)
    {
      PortScalePoint * point =
        &port->scale_points[i];

      GtkTreeIter iter;
      gtk_list_store_append (list_store, &iter);
      gtk_list_store_set (
        list_store, &iter,
        0, (double) point->val,
        1, point->label,
        -1);
      if (fabsf (value - point->val) < FLT_EPSILON)
        {
          active = i;
        }
    }

  GtkWidget* combo =
    gtk_combo_box_new_with_model (
      GTK_TREE_MODEL (list_store));
  gtk_combo_box_set_active (
    GTK_COMBO_BOX (combo), active);
  g_object_unref (list_store);

  bool is_input = port->id.flow == FLOW_INPUT;
  gtk_widget_set_sensitive (combo, is_input);

  GtkCellRenderer * cell =
    gtk_cell_renderer_text_new ();
  gtk_cell_layout_pack_start (
    GTK_CELL_LAYOUT (combo), cell, TRUE);
  gtk_cell_layout_set_attributes (
    GTK_CELL_LAYOUT (combo), cell, "text", 1, NULL);

  if (is_input)
    {
      g_signal_connect (
        G_OBJECT (combo), "changed",
        G_CALLBACK (combo_changed), port);
    }

  return new_controller (NULL, combo);
}

static PluginGtkController *
make_log_slider (
  Port * port,
  float  value)
{
  const float min = port->minf;
  const float max = port->maxf;
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

  bool is_input = port->id.flow == FLOW_INPUT;
  gtk_widget_set_sensitive (scale, is_input);
  gtk_widget_set_sensitive (spin, is_input);

  gtk_widget_set_hexpand (scale, 1);

  gtk_scale_set_draw_value (
    GTK_SCALE (scale), FALSE);
  gtk_range_set_value (
    GTK_RANGE (scale), ldft);
  gtk_spin_button_set_value (
    GTK_SPIN_BUTTON (spin), value);

  if (is_input)
    {
      g_signal_connect (
        G_OBJECT (scale), "value-changed",
        G_CALLBACK (log_scale_changed), port);
      g_signal_connect (
        G_OBJECT (spin), "value-changed",
        G_CALLBACK (log_spin_changed), port);
    }

  return
    new_controller (
      GTK_SPIN_BUTTON (spin), scale);
}

static PluginGtkController*
make_slider (
  Port * port,
  float value)
{
  const float min = port->minf;
  const float max = port->maxf;
  bool is_integer =
    port->id.flags & PORT_FLAG_INTEGER;
  const double step  =
    is_integer ?
      1.0 : ((double) (max - min) / 100.0);
  GtkWidget * scale =
    gtk_scale_new_with_range (
      GTK_ORIENTATION_HORIZONTAL, min, max, step);
  gtk_widget_set_size_request (
    scale, MIN_SCALE_WIDTH, -1);
  GtkWidget * spin =
    gtk_spin_button_new_with_range (min, max, step);

  bool is_input = port->id.flow == FLOW_INPUT;
  gtk_widget_set_sensitive (scale, is_input);
  gtk_widget_set_sensitive (spin, is_input);

  gtk_widget_set_hexpand (scale, 1);

  if (is_integer)
    {
      gtk_spin_button_set_digits (
        GTK_SPIN_BUTTON (spin), 0);
    }
  else
    {
      gtk_spin_button_set_digits (
        GTK_SPIN_BUTTON (spin), 7);
    }

  gtk_scale_set_draw_value (
    GTK_SCALE (scale), FALSE);
  gtk_range_set_value (GTK_RANGE(scale), value);
  gtk_spin_button_set_value (
    GTK_SPIN_BUTTON (spin), value);
  if (port->num_scale_points > 0)
    {
      for (int i = 0; i < port->num_scale_points;
           i++)
        {
          const PortScalePoint * point =
            &port->scale_points[i];

          char * str =
            g_markup_printf_escaped (
              "<span font_size=\"small\">"
              "%s</span>", point->label);
          gtk_scale_add_mark (
            GTK_SCALE (scale), point->val,
            GTK_POS_TOP, str);
        }
    }

  if (is_input)
    {
      g_signal_connect (
        G_OBJECT (scale), "value-changed",
        G_CALLBACK(scale_changed), port);
      g_signal_connect (
        G_OBJECT (spin), "value-changed",
        G_CALLBACK (spin_changed), port);
    }

  return
    new_controller (GTK_SPIN_BUTTON (spin), scale);
}

static PluginGtkController*
make_toggle (
  Port * port,
  float  value)
{
  GtkWidget * check = gtk_switch_new ();
  gtk_widget_set_halign (check, GTK_ALIGN_START);

  bool is_input = port->id.flow == FLOW_INPUT;
  gtk_widget_set_sensitive (check, is_input);

  if (!math_floats_equal (value, 0))
    {
      gtk_switch_set_active (
        GTK_SWITCH (check), TRUE);
    }

  if (is_input)
    {
      g_signal_connect (
        G_OBJECT (check), "state-set",
        G_CALLBACK (switch_state_set), port);
    }

  return new_controller (NULL, check);
}

static PluginGtkController*
make_entry (
  Port * port)
{
  GtkWidget* entry = gtk_entry_new();

  bool is_input = port->id.flow == FLOW_INPUT;
  gtk_widget_set_sensitive (entry, is_input);
  if (is_input)
    {
      g_signal_connect (
        G_OBJECT (entry), "activate",
        G_CALLBACK (string_changed), port);
    }

  return new_controller (NULL, entry);
}

static PluginGtkController*
make_file_chooser (
  Port * port)
{
  GtkWidget * button =
    gtk_file_chooser_button_new (
    _("Open File"), GTK_FILE_CHOOSER_ACTION_OPEN);

  bool is_input = port->id.flow == FLOW_INPUT;
  gtk_widget_set_sensitive (button, is_input);

  if (is_input)
    {
      g_signal_connect (
        G_OBJECT (button), "file-set",
        G_CALLBACK (file_changed), port);
    }

  return new_controller (NULL, button);
}

static PluginGtkController *
make_controller (
  Port * port,
  float  value)
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
    }

  return controller;
}

static int
control_group_cmp (
  const void* p1, const void* p2, void* data)
{
  const Port * control1 = *(const Port **)p1;
  const Port * control2 = *(const Port **)p2;

  g_return_val_if_fail (IS_PORT (control1), -1);
  g_return_val_if_fail (IS_PORT (control2), -1);

  return
    (control1->id.port_group &&
     control2->id.port_group) ?
      strcmp (
        control1->id.port_group,
        control2->id.port_group) : 0;
}

static GtkWidget*
build_control_widget (
  Plugin *    pl,
  GtkWindow * window)
{
  GtkWidget * port_table = gtk_grid_new ();

  /* Make an array of ports sorted by group */
  GArray * controls =
    g_array_new (false, true, sizeof (Port *));
  for (int i = 0; i < pl->num_in_ports; i++)
    {
      Port * port = pl->in_ports[i];
      if (port->id.type != TYPE_CONTROL)
        continue;

      g_array_append_vals (controls, &port, 1);
    }
  g_array_sort_with_data (
    controls, control_group_cmp, pl);

  /* Add controls in group order */
  const char * last_group = NULL;
  int n_rows = 0;
  int num_ctrls = (int) controls->len;
  for (int i = 0; i < num_ctrls; ++i)
    {
      Port * port =
        g_array_index (controls, Port *, i);
      PluginGtkController  * controller = NULL;
      const char * group = port->id.port_group;

      /* Check group and add new heading if
       * necessary */
      if (group && group != last_group)
        {
          const char * group_name = group;
          GtkWidget * group_label =
            plugin_gtk_new_label (
              group_name, true, false, 0.0f, 1.0f);
          gtk_grid_attach (
            GTK_GRID (port_table), group_label,
            0, n_rows, 2, 1);
          ++n_rows;
        }
      last_group = group;

      /* Make control widget */
      if (pl->lv2)
        {
          if (port->value_type ==
                pl->lv2->forge.String)
            {
              controller = make_entry (port);
            }
          else if (port->value_type ==
                     pl->lv2->forge.Path)
            {
              controller = make_file_chooser (port);
            }
          else
            {
              controller =
                make_controller (port, port->deff);
            }
        }
      else
        {
          /* TODO handle non-float carla params */
          controller =
            make_controller (port, port->deff);
        }

      port->widget = controller;
      if (controller)
        {
          /* Add row to table for this controller */
          plugin_gtk_add_control_row (
            port_table, n_rows++,
            (port->id.label ?
               port->id.label : port->id.sym),
            controller);

          /* Set tooltip text from comment,
           * if available */
          if (port->id.comment)
            {
              gtk_widget_set_tooltip_text (
                controller->control,
                port->id.comment);
            }
        }
    }

  if (n_rows > 0)
    {
      gtk_window_set_resizable (
        GTK_WINDOW (window), TRUE);
      GtkWidget * box =
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
        GTK_WINDOW (window), FALSE);
      return button;
    }
}

/**
 * Called on each GUI frame to update the GTK UI.
 *
 * @note This is a GSourceFunc.
 */
int
plugin_gtk_update_plugin_ui (
  Plugin * pl)
{
  /* Emit UI events (for LV2 plugins). */
  if (!pl->setting->open_with_carla &&
       pl->lv2 && pl->visible &&
       (pl->window ||
        pl->lv2->external_ui_widget))
    {
      Lv2Plugin * lv2_plugin = pl->lv2;

      Lv2ControlChange ev;
      const size_t  space =
        zix_ring_read_space (
          lv2_plugin->plugin_to_ui_events);
      for (size_t i = 0;
           i + sizeof(ev) < space;
           i += sizeof(ev) + ev.size)
        {
          /* Read event header to get the size */
          zix_ring_read (
            lv2_plugin->plugin_to_ui_events,
            (char*)&ev, sizeof(ev));

          /* Resize read buffer if necessary */
          lv2_plugin->ui_event_buf =
            g_realloc (
              lv2_plugin->ui_event_buf, ev.size);
          void* const buf =
            lv2_plugin->ui_event_buf;

          /* Read event body */
          zix_ring_read (
            lv2_plugin->plugin_to_ui_events,
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
            lv2_plugin, ev.index,
            ev.size, ev.protocol,
            ev.protocol == 0 ?
              (void *)
              &pl->lilv_ports[ev.index]->control :
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

      if (lv2_plugin->has_external_ui &&
          lv2_plugin->external_ui_widget)
        {
          lv2_plugin->external_ui_widget->run (
            lv2_plugin->external_ui_widget);
        }
    }

  if (pl->setting->open_with_carla)
    {
      /* fetch port values */
      for (int i = 0; i < pl->num_in_ports; i++)
        {
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
plugin_gtk_open_generic_ui (
  Plugin * plugin)
{
  g_message (
    "creating generic GTK window..");
  GtkWidget* controls =
    build_control_widget (
      plugin, plugin->window);
  GtkWidget* scroll_win =
    gtk_scrolled_window_new (NULL, NULL);
  gtk_container_add (
    GTK_CONTAINER (scroll_win), controls);
  gtk_scrolled_window_set_policy (
    GTK_SCROLLED_WINDOW (scroll_win),
    GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
  gtk_container_add (
    GTK_CONTAINER (plugin->ev_box),
    scroll_win);
  gtk_widget_show_all (
    GTK_WIDGET (plugin->vbox));

  GtkRequisition controls_size, box_size;
  gtk_widget_get_preferred_size (
    GTK_WIDGET (controls), NULL,
    &controls_size);
  gtk_widget_get_preferred_size (
    GTK_WIDGET (plugin->vbox), NULL,
    &box_size);

  gtk_window_set_default_size (
    GTK_WINDOW (plugin->window),
    MAX (
      MAX (
        box_size.width,
        controls_size.width) + 24,
      640),
    box_size.height + controls_size.height);
  gtk_window_present (GTK_WINDOW (plugin->window));

  if (plugin->setting->descr->protocol ==
        PROT_LV2 &&
      !plugin->setting->open_with_carla)
    {
      lv2_ui_init (plugin->lv2);
    }

  plugin->ui_instantiated = true;
  EVENTS_PUSH (
    ET_PLUGIN_VISIBILITY_CHANGED, plugin);

  g_message (
    "plugin window shown, adding idle timeout. "
    "Update frequency (Hz): %.01f",
    (double) plugin->ui_update_hz);
  g_return_if_fail (
    plugin->ui_update_hz >=
      PLUGIN_MIN_REFRESH_RATE);

  plugin->update_ui_source_id =
    g_timeout_add (
      (guint)
      (1000.f / plugin->ui_update_hz),
      (GSourceFunc) plugin_gtk_update_plugin_ui,
      plugin);
}

/**
 * Closes the plugin's UI (either LV2 wrapped with
 * suil, generic or LV2 external).
 */
int
plugin_gtk_close_ui (
  Plugin * pl)
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
      g_signal_handler_disconnect (
        pl->window, pl->delete_event_id);
      gtk_widget_set_sensitive (
        GTK_WIDGET (pl->window), 0);
      gtk_window_close (
        GTK_WINDOW (pl->window));
    }

  if (pl->lv2 &&
      pl->lv2->has_external_ui &&
      pl->lv2->external_ui_widget)
    {
      g_message ("hiding external LV2 UI");
      pl->lv2->external_ui_widget->hide (
        pl->lv2->external_ui_widget);
    }

  return 0;
}

typedef struct PresetInfo
{
  char *            name;
  GtkComboBoxText * cb;
} PresetInfo;

/**
 * Sets up the combo box with all the banks the
 * plugin has.
 *
 * @return Whether any banks were added.
 */
bool
plugin_gtk_setup_plugin_banks_combo_box (
  GtkComboBoxText * cb,
  Plugin *          plugin)
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
      gtk_combo_box_text_append (
        cb, bank->uri, bank->name);
      ret = true;
    }

  gtk_combo_box_set_active (
    GTK_COMBO_BOX (cb),
    plugin->selected_bank.bank_idx);

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
plugin_gtk_setup_plugin_presets_combo_box (
  GtkComboBoxText * cb,
  Plugin *          plugin)
{
  bool ret = false;

  gtk_combo_box_text_remove_all (cb);

  if (!plugin ||
      plugin->selected_bank.bank_idx == -1)
    {
      return false;
    }

  PluginBank * bank =
    plugin->banks[
      plugin->selected_bank.bank_idx];
  for (int j = 0; j < bank->num_presets;
       j++)
    {
      PluginPreset * preset =
        bank->presets[j];
      gtk_combo_box_text_append (
        cb, preset->uri, preset->name);
      ret = true;
    }

  gtk_combo_box_set_active (
    GTK_COMBO_BOX (cb),
    plugin->selected_preset.idx);

  return ret;
}
