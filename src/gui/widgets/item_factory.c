/*
 * Copyright (C) 2021-2022 Alexandros Theodotou <alex at zrythm dot org>
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

#include "audio/midi_mapping.h"
#include "gui/backend/wrapped_object_with_change_signal.h"
#include "gui/widgets/item_factory.h"
#include "gui/widgets/popover_menu_bin.h"
#include "plugins/collection.h"
#include "plugins/collections.h"
#include "plugins/plugin_manager.h"
#include "project.h"
#include "settings/chord_preset_pack.h"
#include "utils/error.h"
#include "utils/gtk.h"
#include "utils/objects.h"
#include "utils/string.h"

#include <glib/gi18n.h>

/**
 * Data for individual widget callbacks.
 */
typedef struct ItemFactoryData
{
  ItemFactory *                   factory;
  WrappedObjectWithChangeSignal * obj;
} ItemFactoryData;

static ItemFactoryData *
item_factory_data_new (
  ItemFactory *                   factory,
  WrappedObjectWithChangeSignal * obj)
{
  ItemFactoryData * self =
    object_new (ItemFactoryData);
  self->factory = factory;
  self->obj = obj;

  return self;
}

static void
item_factory_data_destroy_closure (
  gpointer   user_data,
  GClosure * closure)
{
  ItemFactoryData * data =
    (ItemFactoryData *) user_data;

  object_zero_and_free (data);
}

static void
on_toggled (
  GtkCheckButton * self,
  gpointer         user_data)
{
  ItemFactoryData * data =
    (ItemFactoryData *) user_data;

  WrappedObjectWithChangeSignal * obj =
    Z_WRAPPED_OBJECT_WITH_CHANGE_SIGNAL (data->obj);

  bool active = gtk_check_button_get_active (self);

  switch (obj->type)
    {
    case WRAPPED_OBJECT_TYPE_MIDI_MAPPING:
      {
        MidiMapping * mapping =
          (MidiMapping *) obj->obj;
        int mapping_idx =
          midi_mapping_get_index (
            MIDI_MAPPINGS, mapping);
        GError * err = NULL;
        bool ret =
          midi_mapping_action_perform_enable (
            mapping_idx, active, &err);
        if (!ret)
          {
            HANDLE_ERROR (
              err, "%s",
              _("Failed to enable binding"));
          }
      }
      break;
    default:
      break;
    }
}

#if 0
static void
on_list_item_selected (
  GObject    * gobject,
  GParamSpec * pspec,
  gpointer     user_data)
{
  GtkListItem * list_item = GTK_LIST_ITEM (gobject);
  GtkWidget * child =
    gtk_list_item_get_child (list_item);

  if (gtk_list_item_get_selected (list_item))
    gtk_widget_add_css_class (child, "selected");
  else
    gtk_widget_remove_css_class (child, "selected");
}
#endif

static GdkContentProvider *
on_dnd_drag_prepare (
  GtkDragSource * source,
  double          x,
  double          y,
  gpointer        user_data)
{
  ItemFactoryData * data =
    (ItemFactoryData *) user_data;

  g_debug ("PREPARE");
  GdkContentProvider * content_providers[] = {
    gdk_content_provider_new_typed (
      WRAPPED_OBJECT_WITH_CHANGE_SIGNAL_TYPE,
      data->obj),
  };

  return
    gdk_content_provider_new_union (
      content_providers,
      G_N_ELEMENTS (content_providers));
}

static void
item_factory_setup_cb (
  GtkSignalListItemFactory * factory,
  GtkListItem *              listitem,
  gpointer                   user_data)
{
  ItemFactory * self = (ItemFactory *) user_data;

  PopoverMenuBinWidget * bin =
    popover_menu_bin_widget_new ();
  gtk_widget_add_css_class (
    GTK_WIDGET (bin), "list-item-child");

  switch (self->type)
    {
    case ITEM_FACTORY_TOGGLE:
      {
        GtkWidget * check_btn =
          gtk_check_button_new ();
        popover_menu_bin_widget_set_child (
          bin, check_btn);
        gtk_list_item_set_child (
          listitem, GTK_WIDGET (bin));
      }
      break;
    case ITEM_FACTORY_TEXT:
      {
        GtkWidget * label = gtk_label_new ("");
        popover_menu_bin_widget_set_child (
          bin, label);
        gtk_list_item_set_child (
          listitem, GTK_WIDGET (bin));
      }
      break;
    case ITEM_FACTORY_ICON_AND_TEXT:
      {
        GtkBox * box =
          GTK_BOX (
            gtk_box_new (
              GTK_ORIENTATION_HORIZONTAL, 6));
        GtkImage * img =
          GTK_IMAGE (gtk_image_new ());
        GtkWidget * label = gtk_label_new ("");
        gtk_box_append (
          GTK_BOX (box), GTK_WIDGET (img));
        gtk_box_append (GTK_BOX (box), label);
        popover_menu_bin_widget_set_child (
          bin, GTK_WIDGET (box));
        gtk_list_item_set_child (
          listitem, GTK_WIDGET (bin));
      }
      break;
    default:
      break;
    }

#if 0
  g_signal_connect (
    G_OBJECT (listitem), "notify::selected",
    G_CALLBACK (on_list_item_selected), self);
#endif
}

static void
add_plugin_descr_context_menu (
  PopoverMenuBinWidget * bin,
  PluginDescriptor *     descr)
{
  GMenu * menu =
    G_MENU (
      popover_menu_bin_widget_get_menu_model (bin));
  if (menu)
    g_menu_remove_all (menu);
  else
    menu = g_menu_new ();

  GMenuItem * menuitem;
  char tmp[600];

  /* TODO */
#if 0
  /* add option for native generic LV2 UI */
  if (descr->protocol == PROT_LV2
      &&
      descr->min_bridge_mode == CARLA_BRIDGE_NONE)
    {
      menuitem =
        z_gtk_create_menu_item (
          _("Add to project (native generic UI)"),
          NULL,
          "app.plugin-browser-add-to-project");
      g_menu_append_item (menu, menuitem);
    }
#endif

#ifdef HAVE_CARLA
  sprintf (
    tmp,
    "app.plugin-browser-add-to-project-carla::%p",
    descr);
  menuitem =
    z_gtk_create_menu_item (
      _("Add to project"), NULL, tmp);
  g_menu_append_item (menu, menuitem);

  PluginSetting * new_setting =
    plugin_setting_new_default (descr);
  if (descr->has_custom_ui &&
      descr->min_bridge_mode ==
        CARLA_BRIDGE_NONE &&
      !new_setting->force_generic_ui)
    {
      sprintf (
        tmp,
        "app.plugin-browser-add-to-project-"
        "bridged-ui::%p",
        descr);
      menuitem =
        z_gtk_create_menu_item (
          _("Add to project (bridged UI)"), NULL,
          tmp);
      g_menu_append_item (menu, menuitem);
    }
  plugin_setting_free (new_setting);

  sprintf (
    tmp,
    "app.plugin-browser-add-to-project-bridged-"
    "full::%p",
    descr);
  menuitem =
    z_gtk_create_menu_item (
      _("Add to project (bridged full)"), NULL,
      tmp);
  g_menu_append_item (menu, menuitem);
#endif

#if 0
  menuitem =
    GTK_MENU_ITEM (
      gtk_check_menu_item_new_with_mnemonic (
        _("Use _Generic UI")));
  APPEND;
  gtk_check_menu_item_set_active (
    GTK_CHECK_MENU_ITEM (menuitem),
    new_setting->force_generic_ui);
  g_signal_connect (
    G_OBJECT (menuitem), "toggled",
    G_CALLBACK (on_use_generic_ui_toggled), descr);
#endif

  /* add to collection */
  GMenu * add_collections_submenu = g_menu_new ();
  int num_added = 0;
  for (int i = 0;
       i < PLUGIN_MANAGER->collections->
         num_collections;
       i++)
    {
      PluginCollection * coll =
        PLUGIN_MANAGER->collections->collections[i];
      if (plugin_collection_contains_descriptor (
            coll, descr, false))
        {
          continue;
        }

      sprintf (
        tmp,
        "app.plugin-browser-add-to-collection::%p",
        coll);
      menuitem =
        z_gtk_create_menu_item (
          coll->name, NULL, tmp);
      g_menu_append_item (
        add_collections_submenu, menuitem);
      num_added++;
    }
  if (num_added > 0)
    {
      g_menu_append_section (
        menu, _("Add to collection"),
        G_MENU_MODEL (
          add_collections_submenu));
    }
  else
    {
      g_object_unref (
        add_collections_submenu);
    }

  /* remove from collection */
  GMenu * remove_collections_submenu =
    g_menu_new ();
  num_added = 0;
  for (int i = 0;
       i < PLUGIN_MANAGER->collections->
         num_collections;
       i++)
    {
      PluginCollection * coll =
        PLUGIN_MANAGER->collections->collections[i];
      if (!plugin_collection_contains_descriptor (
            coll, descr, false))
        {
          continue;
        }

      sprintf (
        tmp,
        "app.plugin-browser-remove-from-collection::%p",
        coll);
      menuitem =
        z_gtk_create_menu_item (
          coll->name, NULL, tmp);
      g_menu_append_item (
        remove_collections_submenu, menuitem);
      num_added++;
    }
  if (num_added > 0)
    {
      g_menu_append_section (
        menu, _("Remove from collection"),
        G_MENU_MODEL (remove_collections_submenu));
    }
  else
    {
      g_object_unref (remove_collections_submenu);
    }
  popover_menu_bin_widget_set_menu_model (
    bin, G_MENU_MODEL (menu));
}

static void
add_supported_file_context_menu (
  PopoverMenuBinWidget * bin,
  SupportedFile *        descr)
{
  GMenu * menu = g_menu_new ();
  GMenuItem * menuitem;
  char tmp[600];

  if (descr->type == FILE_TYPE_DIR)
    {
      sprintf (
        tmp,
        "app.panel-file-browser-add-bookmark::%p",
        descr);
      menuitem =
        z_gtk_create_menu_item (
          _("Add Bookmark"), "favorite", tmp);
      g_menu_append_item (menu, menuitem);
    }

  popover_menu_bin_widget_set_menu_model (
    bin, G_MENU_MODEL (menu));
}

static void
add_chord_pset_pack_context_menu (
  PopoverMenuBinWidget * bin,
  ChordPresetPack *      pack)
{
  GMenu * menu = g_menu_new ();
  GMenuItem * menuitem;

  char tmp[600];
  sprintf (
    tmp,
    "app.chord-pack-browser-delete-pack::%p",
    pack);
  menuitem =
    z_gtk_create_menu_item (
      _("Delete"), "edit-delete", tmp);
  g_menu_append_item (menu, menuitem);

  popover_menu_bin_widget_set_menu_model (
    bin, G_MENU_MODEL (menu));
}

static void
add_chord_pset_context_menu (
  PopoverMenuBinWidget * bin,
  ChordPreset *          pack)
{
  GMenu * menu = g_menu_new ();

#if 0
  GMenuItem * menuitem;
  char tmp[600];
  sprintf (
    tmp,
    "app.chord-preset-pack-browser-delete::%p",
    pack);
  menuitem =
    z_gtk_create_menu_item (
      _("Delete"), "edit-delete", tmp);
  g_menu_append_item (menu, menuitem);
#endif

  popover_menu_bin_widget_set_menu_model (
    bin, G_MENU_MODEL (menu));
}

static void
item_factory_bind_cb (
  GtkSignalListItemFactory * factory,
  GtkListItem *              listitem,
  gpointer                   user_data)
{
  ItemFactory * self = (ItemFactory *) user_data;

  WrappedObjectWithChangeSignal * obj =
    Z_WRAPPED_OBJECT_WITH_CHANGE_SIGNAL (
      gtk_list_item_get_item (listitem));

  switch (self->type)
    {
    case ITEM_FACTORY_TOGGLE:
      {
        PopoverMenuBinWidget * bin =
          Z_POPOVER_MENU_BIN_WIDGET (
            gtk_list_item_get_child (listitem));
        GtkWidget * check_btn =
          popover_menu_bin_widget_get_child (bin);

        switch (obj->type)
          {
          case WRAPPED_OBJECT_TYPE_MIDI_MAPPING:
            {
              if (string_is_equal (
                    self->column_name, _("On")))
                {
                  MidiMapping * mapping =
                    (MidiMapping *) obj->obj;
                  gtk_check_button_set_active (
                    GTK_CHECK_BUTTON (check_btn),
                    mapping->enabled);
                }
            }
            break;
          default:
            break;
          }

        ItemFactoryData * data =
          item_factory_data_new (self, obj);
        g_object_set_data (
          G_OBJECT (check_btn), "item-factory-data",
          data);
        g_signal_connect_data (
          G_OBJECT (check_btn), "toggled",
          G_CALLBACK (on_toggled), data,
          item_factory_data_destroy_closure,
          G_CONNECT_AFTER);
      }
      break;
    case ITEM_FACTORY_TEXT:
      {
        PopoverMenuBinWidget * bin =
          Z_POPOVER_MENU_BIN_WIDGET (
            gtk_list_item_get_child (listitem));
        GtkLabel * label =
          GTK_LABEL (
            popover_menu_bin_widget_get_child (
              bin));

        switch (obj->type)
          {
          case WRAPPED_OBJECT_TYPE_MIDI_MAPPING:
            {
              MidiMapping * mm =
                (MidiMapping *) obj->obj;
              if (string_is_equal (
                    self->column_name,
                    _("Note/Control")))
                {
                  char ctrl_str[80];
                  char ctrl[60];
                  int ctrl_change_ch =
                    midi_ctrl_change_get_ch_and_description (
                      mm->key, ctrl);
                  if (ctrl_change_ch > 0)
                    {
                      sprintf (
                        ctrl_str, "%02X-%02X (%s)",
                        mm->key[0], mm->key[1], ctrl);
                    }
                  else
                    {
                      sprintf (
                        ctrl_str, "%02X-%02X",
                        mm->key[0], mm->key[1]);
                    }

                  gtk_label_set_text (
                    GTK_LABEL (label), ctrl_str);
                }
              if (string_is_equal (
                    self->column_name,
                    _("Destination")))
                {
                  char path[600];
                  Port * port =
                    port_find_from_identifier (
                      &mm->dest_id);
                  port_get_full_designation (
                    port, path);

                  char min_str[40], max_str[40];
                  sprintf (
                    min_str, "%.4f",
                    (double) port->minf);
                  sprintf (
                    max_str, "%.4f",
                    (double) port->maxf);

                  gtk_label_set_text (
                    GTK_LABEL (label), path);
                }
            }
            break;
          default:
            break;
          }
      }
      break;
    case ITEM_FACTORY_ICON_AND_TEXT:
      {
        PopoverMenuBinWidget * bin =
          Z_POPOVER_MENU_BIN_WIDGET (
            gtk_list_item_get_child (listitem));
        GtkBox * box =
          GTK_BOX (
            popover_menu_bin_widget_get_child (bin));
        GtkImage * img =
          GTK_IMAGE (
            gtk_widget_get_first_child (
              GTK_WIDGET (box)));
        GtkLabel * lbl =
          GTK_LABEL (
            gtk_widget_get_last_child (
              GTK_WIDGET (box)));

        switch (obj->type)
          {
          case WRAPPED_OBJECT_TYPE_PLUGIN_DESCR:
            {
              PluginDescriptor * descr =
                (PluginDescriptor *) obj->obj;
              gtk_image_set_from_icon_name (
                img,
                plugin_descriptor_get_icon_name (
                  descr));
              gtk_label_set_text (lbl, descr->name);

              /* set as drag source */
              GtkDragSource * drag_source =
                gtk_drag_source_new ();
              ItemFactoryData * data =
                item_factory_data_new (self, obj);
              g_object_set_data (
                G_OBJECT (bin), "item-factory-data",
                data);
              g_signal_connect_data (
                G_OBJECT (drag_source),
                "prepare",
                G_CALLBACK (on_dnd_drag_prepare),
                data,
                item_factory_data_destroy_closure,
                0);
              g_object_set_data (
                G_OBJECT (bin), "drag-source",
                drag_source);
              gtk_widget_add_controller (
                GTK_WIDGET (bin),
                GTK_EVENT_CONTROLLER (
                  drag_source));

              add_plugin_descr_context_menu (
                bin, descr);
            }
            break;
          case WRAPPED_OBJECT_TYPE_SUPPORTED_FILE:
            {
              SupportedFile * descr =
                (SupportedFile *) obj->obj;

              gtk_image_set_from_icon_name (
                img,
                supported_file_get_icon_name (
                  descr));
              gtk_label_set_text (lbl, descr->label);

              /* set as drag source */
              GtkDragSource * drag_source =
                gtk_drag_source_new ();
              ItemFactoryData * data =
                item_factory_data_new (self, obj);
              g_object_set_data (
                G_OBJECT (bin), "item-factory-data",
                data);
              g_signal_connect_data (
                G_OBJECT (drag_source),
                "prepare",
                G_CALLBACK (on_dnd_drag_prepare),
                data,
                item_factory_data_destroy_closure,
                0);
              g_object_set_data (
                G_OBJECT (bin), "drag-source",
                drag_source);
              gtk_widget_add_controller (
                GTK_WIDGET (bin),
                GTK_EVENT_CONTROLLER (
                  drag_source));

              add_supported_file_context_menu (
                bin, descr);
            }
            break;
          case WRAPPED_OBJECT_TYPE_CHORD_PSET_PACK:
            {
              ChordPresetPack * pack =
                (ChordPresetPack *) obj->obj;

              gtk_image_set_from_icon_name (
                img, "minuet-chords");
              gtk_label_set_text (lbl, pack->name);

              add_chord_pset_pack_context_menu (
                bin, pack);
            }
            break;
          case WRAPPED_OBJECT_TYPE_CHORD_PSET:
            {
              ChordPreset * pset =
                (ChordPreset *) obj->obj;

              gtk_image_set_from_icon_name (
                img, "minuet-chords");
              gtk_label_set_text (lbl, pset->name);

              add_chord_pset_context_menu (
                bin, pset);
            }
            break;
          default:
            break;
          }
      }
      break;
    default:
      break;
    }
}

static void
item_factory_unbind_cb (
  GtkSignalListItemFactory * factory,
  GtkListItem *              listitem,
  gpointer                   user_data)
{
  ItemFactory * self = (ItemFactory *) user_data;

  WrappedObjectWithChangeSignal * obj =
    Z_WRAPPED_OBJECT_WITH_CHANGE_SIGNAL (
      gtk_list_item_get_item (listitem));

  switch (self->type)
    {
    case ITEM_FACTORY_TOGGLE:
      {
        GtkWidget * child =
          gtk_list_item_get_child (listitem);
        gpointer data =
          g_object_get_data (
            G_OBJECT (child), "item-factory-data");
        g_signal_handlers_disconnect_by_func (
          child, on_toggled, data);
      }
      break;
    case ITEM_FACTORY_ICON_AND_TEXT:
      {
        PopoverMenuBinWidget * bin =
          Z_POPOVER_MENU_BIN_WIDGET (
            gtk_list_item_get_child (listitem));
        GtkBox * box =
          GTK_BOX (
            popover_menu_bin_widget_get_child (bin));
        (void) box;

        switch (obj->type)
          {
          case WRAPPED_OBJECT_TYPE_PLUGIN_DESCR:
          case WRAPPED_OBJECT_TYPE_SUPPORTED_FILE:
            {
              /* set as drag source */
              GtkDragSource * drag_source =
                g_object_get_data (
                  G_OBJECT (bin), "drag-source");
              gtk_widget_remove_controller (
                GTK_WIDGET (bin),
                GTK_EVENT_CONTROLLER (drag_source));
            }
            break;
          default:
            break;
          }
      }
      break;
    default:
      break;
    }
}

static void
item_factory_teardown_cb (
  GtkSignalListItemFactory * factory,
  GtkListItem *              listitem,
  gpointer                   user_data)
{
  /*WrappedObjectWithChangeSignal * obj =*/
    /*Z_WRAPPED_OBJECT_WITH_CHANGE_SIGNAL (*/
      /*gtk_list_item_get_item (listitem));*/

  gtk_list_item_set_child (listitem, NULL);
}

/**
 * Creates a new item factory.
 *
 * @param editable Whether the item should be
 *   editable.
 * @param column_name Column name, if column view,
 *   otherwise NULL.
 */
ItemFactory *
item_factory_new (
  ItemFactoryType type,
  bool            editable,
  const char *    column_name)
{
  ItemFactory * self = object_new (ItemFactory);

  self->type = type;

  self->editable = editable;
  if (column_name)
    self->column_name = g_strdup (column_name);

  GtkListItemFactory * list_item_factory =
    gtk_signal_list_item_factory_new ();

  g_signal_connect (
    G_OBJECT (list_item_factory), "setup",
    G_CALLBACK (item_factory_setup_cb), self);
  g_signal_connect (
    G_OBJECT (list_item_factory), "bind",
    G_CALLBACK (item_factory_bind_cb), self);
  g_signal_connect (
    G_OBJECT (list_item_factory), "unbind",
    G_CALLBACK (item_factory_unbind_cb), self);
  g_signal_connect (
    G_OBJECT (list_item_factory), "teardown",
    G_CALLBACK (item_factory_teardown_cb), self);

  self->list_item_factory =
    GTK_LIST_ITEM_FACTORY (list_item_factory);

  return self;
}

/**
 * Shorthand to generate and append a column to
 * a column view.
 */
void
item_factory_generate_and_append_column (
  GtkColumnView * column_view,
  GPtrArray *     item_factories,
  ItemFactoryType type,
  bool            editable,
  const char *    column_name)
{
  ItemFactory * item_factory =
    item_factory_new (
      type, editable, column_name);
  GtkListItemFactory * list_item_factory =
    item_factory->list_item_factory;
  GtkColumnViewColumn * column =
    gtk_column_view_column_new (
      column_name, list_item_factory);
  gtk_column_view_append_column (
    column_view, column);
  g_ptr_array_add (item_factories, item_factory);
}

void
item_factory_free (
  ItemFactory * self)
{
  g_free_and_null (self->column_name);

  object_zero_and_free (self);
}

void
item_factory_free_func (
  void * self)
{
  item_factory_free ((ItemFactory *) self);
}
