/*
 * Copyright (C) 2020-2022 Alexandros Theodotou <alex at zrythm dot org>
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
#include "gui/widgets/file_chooser_button.h"
#include "gui/widgets/main_window.h"
#include "plugins/carla_native_plugin.h"
#include "plugins/lv2_plugin.h"
#include "plugins/lv2/lv2_gtk.h"
#include "plugins/lv2/lv2_state.h"
#include "plugins/lv2/lv2_ui.h"
#include "plugins/lv2/lv2_urid.h"
#include "plugins/plugin.h"
#include "plugins/plugin_gtk.h"
#include "plugins/plugin_manager.h"
#include "settings/settings.h"
#include "project.h"
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

#if 0
static void
on_quit_activate (
  GtkWidget * widget,
  gpointer    data)
{
  GtkWindow * window = GTK_WINDOW (data);
  gtk_window_destroy (window);
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
#endif

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

#if 0
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
      if (plugin->setting->descr->protocol ==
                 PROT_LV2)
        {
          g_return_if_fail (plugin->lv2);
          GError * err = NULL;
          bool applied =
            lv2_state_apply_preset (
              plugin->lv2,
              (LilvNode *) record->preset, NULL,
              &err);
          if (!applied)
            {
              HANDLE_ERROR (
                err, "%s",
                _("Failed to apply preset"));
              return;
            }
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

      EVENTS_PUSH (ET_PLUGIN_PRESET_LOADED, plugin);
    }
}

void
plugin_gtk_on_preset_destroy (
  PluginGtkPresetRecord * record,
  GClosure* closure)
{
  free (record);
}
#endif

#if 0
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
  Plugin *    plugin,
  GtkWidget * pset_menu)
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
#endif

void
plugin_gtk_on_save_preset_activate (
  GtkWidget * widget,
  Plugin *    plugin)
{
  const PluginSetting * setting = plugin->setting;
  const PluginDescriptor * descr = setting->descr;
  bool open_with_carla = setting->open_with_carla;
  bool is_lv2 = descr->protocol == PROT_LV2;

  GtkWidget* dialog =
    gtk_file_chooser_dialog_new (
    _("Save Preset"),
    plugin->window ?
      plugin->window : GTK_WINDOW (MAIN_WINDOW),
    GTK_FILE_CHOOSER_ACTION_SAVE,
    _("_Cancel"), GTK_RESPONSE_REJECT,
    _("_Save"), GTK_RESPONSE_ACCEPT,
    NULL);

  if (open_with_carla)
    {
#ifdef HAVE_CARLA
      char * homedir =
        g_build_filename (
          g_get_home_dir(), NULL);
      GFile * homedir_file =
        g_file_new_for_path (homedir);
      gtk_file_chooser_set_current_folder (
        GTK_FILE_CHOOSER (dialog), homedir_file,
        NULL);
      g_object_unref (homedir_file);
      g_free (homedir);
#else
      g_return_if_reached ();
#endif
    }
  else if (is_lv2)
    {
      char * dot_lv2 =
        g_build_filename (
          g_get_home_dir(), ".lv2", NULL);
      GFile * dot_lv2_file =
        g_file_new_for_path (dot_lv2);
      gtk_file_chooser_set_current_folder (
        GTK_FILE_CHOOSER (dialog), dot_lv2_file,
        NULL);
      g_object_unref (dot_lv2_file);
      g_free (dot_lv2);
    }

  /* add additional inputs */
  GtkWidget* content =
    gtk_dialog_get_content_area (
      GTK_DIALOG (dialog));
  GtkWidget * uri_entry = NULL;
  GtkWidget * add_prefix =
    add_prefix =
      gtk_check_button_new_with_mnemonic (
        _("_Prefix plugin name"));
  gtk_toggle_button_set_active (
    GTK_TOGGLE_BUTTON (add_prefix), TRUE);
  GtkBox* box =
    GTK_BOX (
      gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 8));
  if (open_with_carla)
    {
    }
  else if (is_lv2)
    {
      GtkWidget* uri_label =
        gtk_label_new (_("URI (Optional):"));
      uri_entry = gtk_entry_new();

      gtk_box_append (
        box, uri_label);
      gtk_box_append (
        box, uri_entry);
      gtk_box_append (
        GTK_BOX (content), GTK_WIDGET (box));
      gtk_entry_set_activates_default (
        GTK_ENTRY (uri_entry), true);
    }
  gtk_box_append (
    GTK_BOX (content), add_prefix);

  /*gtk_window_present (GTK_WINDOW (dialog));*/
  gtk_dialog_set_default_response (
    GTK_DIALOG(dialog), GTK_RESPONSE_ACCEPT);
  int ret =
    z_gtk_dialog_run (
      GTK_DIALOG (dialog), false);
  if (ret == GTK_RESPONSE_ACCEPT)
    {
      char * path =
        z_gtk_file_chooser_get_filename (
          GTK_FILE_CHOOSER (dialog));
      bool add_prefix_active =
        gtk_toggle_button_get_active (
          GTK_TOGGLE_BUTTON (add_prefix));
      if (open_with_carla)
        {
#ifdef HAVE_CARLA
          const char * prefix = "";
          const char * sep = "";
          char * dirname =
            g_path_get_dirname(path);
          char * basename =
            g_path_get_basename(path);
          char * sym =
            string_symbolify (basename);
          if (add_prefix_active)
            {
              prefix = descr->name;
              sep = "_";
            }
          char * sprefix =
            string_symbolify (prefix);
          char * bundle =
            g_strjoin (
              NULL, sprefix, sep, sym,
              ".preset.carla", NULL);
          char * dir =
            g_build_filename (
              dirname, bundle, NULL);
          carla_native_plugin_save_state (
            plugin->carla, false, dir);
          g_free (dirname);
          g_free (basename);
          g_free (sym);
          g_free (sprefix);
          g_free (bundle);
          g_free (dir);
#endif
        }
      else if (is_lv2)
        {
          const char * uri =
            gtk_editable_get_text (
              GTK_EDITABLE (uri_entry));
          lv2_gtk_on_save_preset_activate (
            widget, plugin->lv2, path, uri,
            add_prefix_active);
        }
      g_free (path);
    }

  gtk_window_destroy (GTK_WINDOW (dialog));

  EVENTS_PUSH (ET_PLUGIN_PRESET_SAVED, plugin);
}

#if 0
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
#endif

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

#if 0
void
plugin_gtk_build_menu (
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
    G_CALLBACK(plugin_gtk_on_save_preset_activate), plugin);
  g_signal_connect (
    G_OBJECT(delete_preset), "activate",
    G_CALLBACK(on_delete_preset_activate), plugin);

  gtk_box_pack_start (
    GTK_BOX (vbox), menu_bar, FALSE, FALSE, 0);
}
#endif

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

  if (pl->lv2)
    {
      object_free_w_func_and_null (
        suil_instance_free,
        pl->lv2->suil_instance);
    }
}

static gboolean
on_close_request (
  GtkWindow * window,
  Plugin *    plugin)
{
  plugin->visible = 0;
  plugin->window = NULL;
  EVENTS_PUSH (
    ET_PLUGIN_VISIBILITY_CHANGED, plugin);

  char pl_str[700];
  plugin_print (plugin, pl_str, 700);
  g_message (
    "%s: deleted plugin [%s] window",
    __func__, pl_str);

  return false;
}

/**
 * Creates a new GtkWindow that will be used to
 * either wrap plugin UIs or create generic UIs in.
 */
void
plugin_gtk_create_window (
  Plugin * plugin)
{
  if (plugin->lv2 &&
      plugin->lv2->has_external_ui &&
      plugin->lv2->external_ui_widget)
    {
      g_message (
        "plugin has external UI, skipping window "
        "creation");
      return;
    }

  g_message (
    "creating GTK window for %s",
    plugin->setting->descr->name);

  /* create window */
  plugin->window =
    GTK_WINDOW (gtk_window_new ());
  plugin_gtk_set_window_title (
    plugin, plugin->window);
  gtk_window_set_icon_name (
    plugin->window, "zrythm");
  /*gtk_window_set_role (*/
    /*plugin->window, "plugin_ui");*/

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
  gtk_window_set_child (
    GTK_WINDOW (plugin->window),
    GTK_WIDGET (plugin->vbox));

#if 0
  /* add menu bar */
  plugin_gtk_build_menu (
    plugin, GTK_WIDGET (plugin->window),
    GTK_WIDGET (plugin->vbox));
#endif

  /* Create/show alignment to contain UI (whether
   * custom or generic) */
  plugin->ev_box =
    GTK_BOX (
      gtk_box_new (GTK_ORIENTATION_VERTICAL, 0));
  gtk_box_append (
    plugin->vbox, GTK_WIDGET (plugin->ev_box));

  /* connect signals */
  plugin->destroy_window_id =
    g_signal_connect (
      plugin->window, "destroy",
      G_CALLBACK (on_window_destroy), plugin);
  plugin->close_request_id =
    g_signal_connect (
      G_OBJECT (plugin->window), "close-request",
      G_CALLBACK (on_close_request), plugin);
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

  LV2_Atom_Forge * forge =
    &lv2_plugin->main_forge;
  LV2_Atom_Forge_Frame frame;

  if (is_property)
    {
      g_debug (
        "setting property control '%s' to '%s'",
        port->id.sym,
        (char *) body);

      uint8_t buf[1024];

      /* forge patch set atom */
      lv2_atom_forge_set_buffer (
        forge, buf, sizeof (buf));
      lv2_atom_forge_object (
        forge, &frame, 0, PM_URIDS.patch_Set);
      lv2_atom_forge_key (
        forge, PM_URIDS.patch_property);
      lv2_atom_forge_urid (
        forge,
        lv2_urid_map_uri (
          lv2_plugin, port->id.uri));
      lv2_atom_forge_key (
        forge, PM_URIDS.patch_value);
      lv2_atom_forge_atom (
        forge, size, type);
      lv2_atom_forge_write (
        forge, body, size);

      const LV2_Atom* atom =
        lv2_atom_forge_deref (
          forge, frame.ref);
      g_return_if_fail (
        lv2_plugin->control_in >= 0 &&
        lv2_plugin->control_in < 400000);
      lv2_ui_send_event_from_ui_to_plugin (
        lv2_plugin,
        (uint32_t) lv2_plugin->control_in,
        lv2_atom_total_size (atom),
        PM_URIDS.atom_eventTransfer, atom);
    }
  else if (port->value_type == forge->Float)
    {
      g_debug (
        "setting float control '%s' to '%f'",
        port->id.sym,
        (double) * (float *) body);

      port->control = *(float*) body;
      port->unsnapped_control = *(float*)body;
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
      LV2_Atom_Forge * forge =
        &lv2_plugin->main_forge;

      if (type == forge->Int)
        {
          const int32_t ival = lrint (value);
          set_lv2_control (
            lv2_plugin, port, sizeof (ival), type,
            &ival);
        }
      else if (type == forge->Long)
        {
          const int64_t lval = lrint(value);
          set_lv2_control (
            lv2_plugin, port, sizeof (lval), type,
            &lval);
        }
      else if (type == forge->Float)
        {
          set_lv2_control (
            lv2_plugin, port, sizeof (value),
            type, &value);
        }
      else if (type == forge->Double)
        {
          const double dval = value;
          set_lv2_control (
            lv2_plugin, port, sizeof (dval), type,
            &dval);
        }
      else if (type == forge->Bool)
        {
          const int32_t ival = (int32_t) value;
          set_lv2_control (
            lv2_plugin, port, sizeof (ival), type,
            &ival);
        }
      else if (port->id.flags &
                 PORT_FLAG_GENERIC_PLUGIN_PORT)
        {
          port_set_control_value (
            port, value, F_NOT_NORMALIZED,
            F_PUBLISH_EVENTS);
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
  PluginGtkController * controller = port->widget;
  Plugin * pl = controller->plugin;
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
  PluginGtkController * controller = port->widget;
  Plugin * pl = controller->plugin;
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
  PluginGtkController * controller = port->widget;
  Plugin * pl = controller->plugin;
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
    gtk_editable_get_text (GTK_EDITABLE (widget));

  g_return_if_fail (
    IS_PORT_AND_NONNULL (port));
  PluginGtkController * controller = port->widget;
  Plugin * pl = controller->plugin;
  g_return_if_fail (
    IS_PLUGIN_AND_NONNULL (pl));

  if (pl->lv2)
    {
      set_lv2_control (
        pl->lv2, port,
        strlen (string) + 1,
        pl->lv2->main_forge.String,
        string);
    }
  else
    {
      /* TODO carla */
    }
}

static void
file_changed (
  GtkNativeDialog *      dialog,
  gint                   response_id,
  Port *                 port)
{
  GtkFileChooserNative * file_chooser_native =
    GTK_FILE_CHOOSER_NATIVE (dialog);
  GtkFileChooser * file_chooser =
    GTK_FILE_CHOOSER (file_chooser_native);
  char * filename =
    z_gtk_file_chooser_get_filename (file_chooser);

  g_return_if_fail (
    IS_PORT_AND_NONNULL (port));
  PluginGtkController * controller = port->widget;
  Plugin * pl = controller->plugin;
  g_return_if_fail (
    IS_PLUGIN_AND_NONNULL (pl));

  if (pl->lv2)
    {
      set_lv2_control (
        pl->lv2, port, strlen (filename),
        pl->lv2->main_forge.Path, filename);
    }
  g_free (filename);
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
      const PortScalePoint * point =
        port->scale_points[i];

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
            port->scale_points[i];

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
    GTK_WIDGET (
      file_chooser_button_widget_new (
        GTK_WINDOW (MAIN_WINDOW),
        _("Open File"),
        GTK_FILE_CHOOSER_ACTION_OPEN));

  bool is_input = port->id.flow == FLOW_INPUT;
  gtk_widget_set_sensitive (button, is_input);

  if (is_input)
    {
      file_chooser_button_widget_set_response_callback (
        Z_FILE_CHOOSER_BUTTON_WIDGET (button),
        G_CALLBACK (file_changed), port, NULL);
    }

  return new_controller (NULL, button);
}

static PluginGtkController *
make_controller (
  Plugin * pl,
  Port *   port,
  float    value)
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

      g_array_append_val (controls, port);
    }
  g_array_sort (
    controls, port_identifier_port_group_cmp);

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
      if (group &&
          !string_is_equal (group, last_group))
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
          LV2_Atom_Forge * forge =
            &pl->lv2->main_forge;
          if (port->value_type == forge->String)
            {
              controller = make_entry (port);
            }
          else if (port->value_type == forge->Path)
            {
              controller = make_file_chooser (port);
            }
          else
            {
              controller =
                make_controller (
                  pl, port, port->deff);
            }
        }
      else
        {
          /* TODO handle non-float carla params */
          controller =
            make_controller (
              pl, port, port->deff);
        }

      port->widget = controller;
      if (controller)
        {
          controller->port = port;
          controller->plugin = pl;

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
      gtk_box_append (
        GTK_BOX (box), port_table);
      return box;
    }
  else
    {
      g_object_unref (port_table);
      GtkWidget* button =
        gtk_button_new_with_label (_("Close"));
      g_signal_connect_swapped (
        button, "clicked",
        G_CALLBACK (gtk_window_destroy), window);
      gtk_window_set_resizable (
        GTK_WINDOW (window), FALSE);
      return button;
    }
}

static double
get_atom_double (
  Lv2Plugin *  plugin,
  uint32_t     size,
  LV2_URID     type,
  const void * body,
  bool *       is_nan)
{
  *is_nan = false;

  LV2_Atom_Forge * forge = &plugin->main_forge;
  if (type == forge->Int || type == forge->Bool)
    return *(const int32_t*)body;
  else if (type == forge->Long)
    return *(const int64_t*)body;
  else if (type == forge->Float)
    return *(const float*)body;
  else if (type == forge->Double)
    return *(const double*)body;

  *is_nan = true;

  return NAN;
}

/**
 * Called when a property changed or when there is a
 * UI port event to set (update) the widget's value.
 */
void
plugin_gtk_generic_set_widget_value (
  Plugin *              pl,
  PluginGtkController * controller,
  uint32_t              size,
  LV2_URID              type,
  const void *          body)
{
  GtkWidget * widget = controller->control;
  bool is_nan = false;
  double fvalue;
  if (pl->lv2)
    {
      fvalue =
        get_atom_double (
          pl->lv2, size, type, body, &is_nan);
    }
  else
    {
      fvalue = (double) *(const float*)body;
    }

  if (!is_nan)
    {
      if (GTK_IS_COMBO_BOX (widget))
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
                g_value_get_float (&value);
              g_value_unset (&value);
              if (fabs (v - fvalue) < DBL_EPSILON)
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
            fvalue > 0.0);
        }
      else if (GTK_IS_RANGE (widget))
        {
          gtk_range_set_value (
            GTK_RANGE (widget), fvalue);
        }
      else if (GTK_IS_SWITCH (widget))
        {
          gtk_switch_set_active (
            GTK_SWITCH (widget), fvalue > 0.0);
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
  else if (GTK_IS_ENTRY (widget) &&
           type == PM_URIDS.atom_String)
    {
      gtk_editable_set_text (
        GTK_EDITABLE (widget), (const char*)body);
    }
  else if (GTK_IS_FILE_CHOOSER (widget) &&
           type == PM_URIDS.atom_Path)
    {
      z_gtk_file_chooser_set_file_from_path (
        GTK_FILE_CHOOSER (widget),
        (const char*)body);
    }
  else
    g_warning (
      _("Unknown widget type for value\n"));
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
          Port * port = pl->in_ports[i];
          if (port->id.type != TYPE_CONTROL ||
              !port->widget)
            {
              continue;
            }

          plugin_gtk_generic_set_widget_value (
            pl, port->widget, 0, 0, &port->control);
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
  Plugin * plugin,
  bool     fire_events)
{
  g_message (
    "opening generic GTK window..");
  GtkWidget* controls =
    build_control_widget (
      plugin, plugin->window);
  GtkWidget* scroll_win =
    gtk_scrolled_window_new ();
  gtk_scrolled_window_set_child (
    GTK_SCROLLED_WINDOW (scroll_win), controls);
  gtk_scrolled_window_set_policy (
    GTK_SCROLLED_WINDOW (scroll_win),
    GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
  gtk_box_append (
    GTK_BOX (plugin->ev_box), scroll_win);
  gtk_widget_set_vexpand (
    GTK_WIDGET (scroll_win), true);

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
  if (fire_events)
    {
      EVENTS_PUSH (
        ET_PLUGIN_VISIBILITY_CHANGED, plugin);
    }

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
      if (pl->destroy_window_id)
        {
          g_signal_handler_disconnect (
            pl->window, pl->destroy_window_id);
          pl->destroy_window_id = 0;
        }
      if (pl->close_request_id)
        {
          g_signal_handler_disconnect (
            pl->window, pl->close_request_id);
          pl->close_request_id = 0;
        }
      gtk_widget_set_sensitive (
        GTK_WIDGET (pl->window), 0);
      /*gtk_window_close (*/
        /*GTK_WINDOW (pl->window));*/
      gtk_window_destroy (GTK_WINDOW (pl->window));
      pl->window = NULL;
    }

  if (pl->lv2 &&
      pl->lv2->has_external_ui &&
      pl->lv2->external_ui_widget &&
      pl->external_ui_visible)
    {
      g_message ("hiding external LV2 UI");
      pl->lv2->external_ui_widget->hide (
        pl->lv2->external_ui_widget);
      pl->external_ui_visible = false;
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
plugin_gtk_setup_plugin_presets_list_box (
  GtkListBox * box,
  Plugin *     plugin)
{
  g_debug ("%s: setting up...", __func__);

  z_gtk_list_box_remove_all_children (box);

  if (!plugin ||
      plugin->selected_bank.bank_idx == -1)
    {
      g_debug (
        "%s: no plugin (%p) or selected bank (%d)",
        __func__, plugin,
        plugin ?
          plugin->selected_bank.bank_idx : -100);
      return false;
    }

  char pl_str[800];
  plugin_print (plugin, pl_str, 800);
  if (!plugin->instantiated)
    {
      g_message (
        "plugin %s not instantiated", pl_str);
      return false;
    }

  bool ret = false;
  PluginBank * bank =
    plugin->banks[
      plugin->selected_bank.bank_idx];
  for (int j = 0; j < bank->num_presets;
       j++)
    {
      PluginPreset * preset =
        bank->presets[j];
      GtkWidget * label =
        gtk_label_new (preset->name);
      gtk_list_box_insert (
        box, label, -1);
      ret = true;
    }

  GtkListBoxRow * row =
    gtk_list_box_get_row_at_index (
      box, plugin->selected_preset.idx);
  gtk_list_box_select_row (box, row);

  g_debug ("%s: done", __func__);

  return ret;
}
