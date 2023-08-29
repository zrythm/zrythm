// SPDX-FileCopyrightText: © 2018-2023 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

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

#include "dsp/engine.h"
#include "dsp/track.h"
#include "gui/backend/event.h"
#include "gui/backend/event_manager.h"
#include "gui/widgets/main_window.h"
#include "plugins/lv2/lv2_gtk.h"
#include "plugins/lv2/lv2_state.h"
#include "plugins/lv2/lv2_ui.h"
#include "plugins/lv2_plugin.h"
#include "plugins/plugin_gtk.h"
#include "plugins/plugin_manager.h"
#include "project.h"
#include "settings/settings.h"
#include "utils/lilv.h"
#include "utils/math.h"
#include "utils/objects.h"
#include "utils/string.h"
#include "utils/ui.h"
#include "zrythm.h"
#include "zrythm_app.h"

#include <glib/gi18n.h>
#include <gtk/gtk.h>

#include <lv2/patch/patch.h>
#include <lv2/port-props/port-props.h>
#include <lv2/presets/presets.h>
#include <suil/suil.h>

void
lv2_gtk_on_save_activate (Lv2Plugin * plugin)
{
  return;

#if 0
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

#if 0
PluginGtkPresetMenu*
lv2_gtk_get_bank_menu (
  Lv2Plugin* plugin,
  PluginGtkPresetMenu* menu,
  const LilvNode* bank)
{
  LilvNode* label =
    lilv_world_get (
      LILV_WORLD, bank,
      PM_GET_NODE (LILV_NS_RDFS "label"), NULL);

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
      PM_GET_NODE (LV2_PRESETS__bank), NULL);

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
#endif

/**
 * Called by plugin_gtk_on_save_preset_activate()
 * on accept.
 */
void
lv2_gtk_on_save_preset_activate (
  GtkWidget *  widget,
  Lv2Plugin *  plugin,
  const char * path,
  const char * uri,
  bool         add_prefix)
{
  LilvNode * plug_name =
    lilv_plugin_get_name (plugin->lilv_plugin);
  const char * prefix = "";
  const char * sep = "";
  if (add_prefix)
    {
      prefix = lilv_node_as_string (plug_name);
      sep = "_";
    }

  char * dirname = g_path_get_dirname (path);
  char * basename = g_path_get_basename (path);
  char * sym = string_symbolify (basename);
  char * sprefix = string_symbolify (prefix);
  char * bundle =
    g_strjoin (NULL, sprefix, sep, sym, ".preset.lv2/", NULL);
  char * file = g_strjoin (NULL, sym, ".ttl", NULL);
  char * dir = g_build_filename (dirname, bundle, NULL);

  lv2_state_save_preset (
    plugin, dir, (strlen (uri) ? uri : NULL), basename, file);

  // Reload bundle into the world
  LilvNode * ldir = lilv_new_file_uri (LILV_WORLD, NULL, dir);
  lilv_world_unload_bundle (LILV_WORLD, ldir);
  lilv_world_load_bundle (LILV_WORLD, ldir);
  lilv_node_free (ldir);

#if 0
  // Rebuild preset menu and update window title
  if (GTK_IS_MENU_ITEM (widget))
    {
      plugin_gtk_rebuild_preset_menu (
        plugin->plugin,
        GTK_CONTAINER (
          gtk_widget_get_parent (widget)));
    }
#endif

  g_free (dir);
  g_free (file);
  g_free (bundle);
  free (sprefix);
  free (sym);
  g_free (basename);
  g_free (dirname);
  lilv_node_free (plug_name);
}

#if 0
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

  gtk_window_present (GTK_WINDOW (dialog));
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
#endif

static int
patch_set_get (
  Lv2Plugin *             plugin,
  const LV2_Atom_Object * obj,
  const LV2_Atom_URID **  property,
  const LV2_Atom **       value)
{
  lv2_atom_object_get (
    obj, PM_URIDS.patch_property, (const LV2_Atom *) property,
    PM_URIDS.patch_value, value, 0);
  if (!*property)
    {
      g_warning ("patch:Set message with no property");
      return 1;
    }
  else if ((*property)->atom.type != plugin->main_forge.URID)
    {
      g_warning ("patch:Set property is not a URID");
      return 1;
    }

  return 0;
}

static int
patch_put_get (
  Lv2Plugin *              plugin,
  const LV2_Atom_Object *  obj,
  const LV2_Atom_Object ** body)
{
  lv2_atom_object_get (
    obj, PM_URIDS.patch_body, (const LV2_Atom *) body, 0);
  if (!*body)
    {
      g_warning ("patch:Put message with no body");
      return 1;
    }
  else if (!lv2_atom_forge_is_object_type (
             &plugin->main_forge, (*body)->atom.type))
    {
      g_warning ("patch:Put body is not an object");
      return 1;
    }

  return 0;
}

/**
 * Called when a property changed when delivering a
 * port event to the plugin UI.
 *
 * This applies to both generic and custom UIs.
 */
static void
property_changed (
  Lv2Plugin *      plugin,
  LV2_URID         key,
  const LV2_Atom * value)
{
  Port * port = lv2_plugin_get_property_port (plugin, key);
  if (port)
    {
      g_message (
        "LV2 plugin property for %s changed", port->id.sym);
      plugin_gtk_generic_set_widget_value (
        plugin->plugin, port->widget, value->size,
        value->type, value + 1);
    }
  else
    {
      g_message ("Unknown LV2 plugin property changed");
    }
}

/**
 * Called to deliver a port event to the plugin
 * UI.
 *
 * This applies to both generic and custom UIs.
 *
 * Called on the main thread.
 */
void
lv2_gtk_ui_port_event (
  Lv2Plugin *  lv2_plugin,
  uint32_t     port_index,
  uint32_t     buffer_size,
  uint32_t     protocol,
  const void * buffer)
{
  if (lv2_plugin->suil_instance)
    {
      suil_instance_port_event (
        lv2_plugin->suil_instance, port_index, buffer_size,
        protocol, buffer);
      return;
    }

  if (protocol == 0)
    {
      Plugin * pl = lv2_plugin->plugin;
      g_return_if_fail ((int) port_index < pl->num_lilv_ports);
      Port * port = pl->lilv_ports[port_index];

      if (port->widget)
        {
          plugin_gtk_generic_set_widget_value (
            pl, port->widget, buffer_size,
            lv2_plugin->main_forge.Float, buffer);
          return;
        }
      else
        {
          /* no widget (probably notOnGUI) */
          return;
        }
    }
  else if (protocol != PM_URIDS.atom_eventTransfer)
    {
      g_warning ("Unknown port event protocol");
      return;
    }

  const LV2_Atom * atom = (const LV2_Atom *) buffer;
  if (lv2_atom_forge_is_object_type (
        &lv2_plugin->main_forge, atom->type))
    {
      lv2_plugin->updating = true;
      const LV2_Atom_Object * obj =
        (const LV2_Atom_Object *) buffer;
      if (obj->body.otype == PM_URIDS.patch_Set)
        {
          const LV2_Atom_URID * property = NULL;
          const LV2_Atom *      value = NULL;
          if (!patch_set_get (
                lv2_plugin, obj, &property, &value))
            {
              property_changed (
                lv2_plugin, property->body, value);
            }
        }
      else if (obj->body.otype == PM_URIDS.patch_Put)
        {
          const LV2_Atom_Object * body = NULL;
          if (!patch_put_get (lv2_plugin, obj, &body))
            {
              LV2_ATOM_OBJECT_FOREACH (body, prop)
              {
                property_changed (
                  lv2_plugin, prop->key, &prop->value);
              }
            }
        }
      else
        g_warning ("Unknown object type?");
      lv2_plugin->updating = false;
    }
}

void
on_external_ui_closed (void * controller)
{
  g_message ("External LV2 UI closed");
  Lv2Plugin * self = (Lv2Plugin *) controller;
  plugin_gtk_close_ui (self->plugin);
  self->plugin->visible = 0;
  EVENTS_PUSH (ET_PLUGIN_VISIBILITY_CHANGED, self->plugin);
}

/**
 * Opens the LV2 plugin's UI (either wrapped with
 * suil or external).
 *
 * Use plugin_gtk_*() for generic UIs.
 */
int
lv2_gtk_open_ui (Lv2Plugin * plugin)
{
  if (plugin->has_external_ui)
    {
      plugin->plugin->window = NULL;
      plugin->plugin->ev_box = NULL;
      plugin->plugin->vbox = NULL;
    }

  /* Attempt to instantiate custom UI if
   * necessary */
  if (!plugin->plugin->setting->force_generic_ui)
    {
      if (plugin->has_external_ui)
        {
          g_message ("Instantiating external UI...");

          plugin->extui.ui_closed = on_external_ui_closed;
          plugin->extui.plugin_human_id =
            plugin_generate_window_title (plugin->plugin);
        }
      else
        {
          g_message ("Instantiating UI...");
        }
      lv2_ui_instantiate (plugin);
    }

  /* present the window */
  if (plugin->has_external_ui && plugin->external_ui_widget)
    {
      g_message ("showing external LV2 UI");
      plugin->external_ui_widget->show (
        plugin->external_ui_widget);
      plugin->plugin->external_ui_visible = true;
    }
  else if (plugin->suil_instance)
    {
      g_message ("Creating suil window for UI...");
      GtkWidget * widget = GTK_WIDGET (
        suil_instance_get_widget (plugin->suil_instance));

      /* suil already adds the widget to the
       * container in win_in_gtk3 but it doesn't
       * in x11_in_gtk3 */
#ifndef _WOE32
      gtk_box_append (
        GTK_BOX (plugin->plugin->ev_box), widget);
#endif
      gtk_window_set_resizable (
        GTK_WINDOW (plugin->plugin->window),
        lv2_ui_is_resizable (plugin));
      gtk_widget_grab_focus (widget);
      gtk_window_present (GTK_WINDOW (plugin->plugin->window));
    }
  else
    {
      ui_show_message_printf (
        _ ("Failed to Open UI"),
        _ ("Failed to open LV2 UI for %s"),
        plugin->plugin->setting->descr->name);
      return -1;
    }

  if (!plugin->has_external_ui)
    {
      g_return_val_if_fail (plugin->lilv_plugin, -1);
    }

  lv2_ui_init (plugin);

  plugin->plugin->ui_instantiated = 1;
#if 0
  EVENTS_PUSH (
    ET_PLUGIN_VISIBILITY_CHANGED,
    plugin->plugin);
#endif

  g_message (
    "plugin window shown, adding idle timeout. "
    "Update frequency (Hz): %.01f",
    (double) plugin->plugin->ui_update_hz);
  g_return_val_if_fail (
    plugin->plugin->ui_update_hz >= PLUGIN_MIN_REFRESH_RATE,
    -1);

  plugin->plugin->update_ui_source_id = g_timeout_add (
    (int) (1000.f / plugin->plugin->ui_update_hz),
    (GSourceFunc) plugin_gtk_update_plugin_ui, plugin->plugin);

  return 0;
}
