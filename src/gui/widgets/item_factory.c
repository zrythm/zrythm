/*
 * Copyright (C) 2021 Alexandros Theodotou <alex at zrythm dot org>
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
#include "project.h"
#include "utils/error.h"
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
  WrappedObjectWithChangeSignal * obj =
    Z_WRAPPED_OBJECT_WITH_CHANGE_SIGNAL (user_data);

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

static void
item_factory_setup_cb (
  GtkSignalListItemFactory * factory,
  GtkListItem *              listitem,
  gpointer                   user_data)
{
  ItemFactory * self = (ItemFactory *) user_data;

  /*WrappedObjectWithChangeSignal * obj =*/
    /*Z_WRAPPED_OBJECT_WITH_CHANGE_SIGNAL (*/
      /*gtk_list_item_get_item (listitem));*/

  switch (self->type)
    {
    case ITEM_FACTORY_TOGGLE:
      {
        GtkWidget * check_btn =
          gtk_check_button_new ();
        gtk_list_item_set_child (listitem, check_btn);
      }
      break;
    case ITEM_FACTORY_TEXT:
      {
        GtkWidget * label = gtk_label_new ("");
        gtk_list_item_set_child (listitem, label);
      }
      break;
    default:
      break;
    }
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
        GtkWidget * check_btn =
          gtk_list_item_get_child (listitem);

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
        GtkLabel * label =
          GTK_LABEL (
            gtk_list_item_get_child (listitem));

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

  /*WrappedObjectWithChangeSignal * obj =*/
    /*Z_WRAPPED_OBJECT_WITH_CHANGE_SIGNAL (*/
      /*gtk_list_item_get_item (listitem));*/

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

ItemFactory *
item_factory_new (
  ItemFactoryType type,
  bool            editable,
  const char *    column_name)
{
  ItemFactory * self = object_new (ItemFactory);

  self->editable = editable;
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
      ITEM_FACTORY_TOGGLE, editable, column_name);
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
