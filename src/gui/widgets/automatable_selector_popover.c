/*
 * Copyright (C) 2019-2021 Alexandros Theodotou <alex at zrythm dot org>
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

#include "audio/automation_track.h"
#include "audio/channel_track.h"
#include "audio/engine.h"
#include "gui/backend/event.h"
#include "gui/backend/event_manager.h"
#include "gui/widgets/automatable_selector_popover.h"
#include "plugins/plugin.h"
#include "project.h"
#include "utils/flags.h"
#include "utils/gtk.h"
#include "utils/resources.h"
#include "zrythm_app.h"

#include <glib/gi18n.h>
#include <gtk/gtk.h>

G_DEFINE_TYPE (
  AutomatableSelectorPopoverWidget,
  automatable_selector_popover_widget,
  GTK_TYPE_POPOVER)

static void
on_closed (
  AutomatableSelectorPopoverWidget * self,
  gpointer                           user_data)
{
  /* if the selected automatable changed */
  Port * at_port = port_find_from_identifier (
    &self->owner->port_id);
  if (
    self->selected_port
    && at_port != self->selected_port)
    {
      /* set the previous automation track
       * invisible */
      self->owner->visible = 0;

      g_message (
        "selected port: %s",
        self->selected_port->id.label);

      /* swap indices */
      AutomationTracklist * atl =
        automation_track_get_automation_tracklist (
          self->owner);
      AutomationTrack * selected_at =
        automation_tracklist_get_at_from_port (
          atl, self->selected_port);
      g_return_if_fail (selected_at);
      automation_tracklist_set_at_index (
        atl, self->owner, selected_at->index,
        F_NO_PUSH_DOWN);

      selected_at->created = true;
      selected_at->visible = true;
      EVENTS_PUSH (
        ET_AUTOMATION_TRACK_ADDED, selected_at);
    }
  else
    {
      g_message (
        "same automatable selected, doing nothing");
    }

  gtk_widget_unparent (GTK_WIDGET (self));
}

/**
 * Updates the info label based on the currently
 * selected Automatable.
 */
static int
update_info_label (
  AutomatableSelectorPopoverWidget * self)
{
  if (self->selected_port)
    {
      Port * port = self->selected_port;

      char * label = g_strdup_printf (
        "%s\nMin: %f\nMax: %f", port->id.label,
        (double) port->minf, (double) port->maxf);

      gtk_label_set_text (self->info, label);
    }
  else
    {
      gtk_label_set_text (
        self->info, _ ("No control selected"));
    }

  return G_SOURCE_REMOVE;
}

/**
 * Selects the selected automatable on the UI.
 */
static void
select_automatable (
  AutomatableSelectorPopoverWidget * self)
{
  /* select current type */
  GtkTreeSelection * sel =
    gtk_tree_view_get_selection (
      GTK_TREE_VIEW (self->type_treeview));
  GtkTreeIter iter;
  int valid = gtk_tree_model_get_iter_first (
    self->type_model, &iter);
  g_return_if_fail (valid);
  while (valid)
    {
      int type;
      int slot;
      gtk_tree_model_get (
        self->type_model, &iter, 2, &type, 3,
        &slot, -1);
      if (
        (AutomatableSelectorType) type
          == self->selected_type
        && slot == self->selected_slot)
        {
          break;
        }

      valid = gtk_tree_model_iter_next (
        self->type_model, &iter);
    }
  g_return_if_fail (valid);
  gtk_tree_selection_select_iter (sel, &iter);

  /* select current automatable */
  if (self->selected_port)
    {
      sel = gtk_tree_view_get_selection (
        GTK_TREE_VIEW (self->port_treeview));

      /* find the iter corresponding to the
       * automatable */
      valid = gtk_tree_model_get_iter_first (
        self->port_model, &iter);
      while (valid)
        {
          Port * port;
          gtk_tree_model_get (
            self->port_model, &iter, 2, &port, -1);
          if (port == self->selected_port)
            {
              gtk_tree_selection_select_iter (
                sel, &iter);
              break;
            }
          valid = gtk_tree_model_iter_next (
            self->port_model, &iter);
        }
    }

  update_info_label (self);
}

static GtkTreeModel *
create_model_for_ports (
  AutomatableSelectorPopoverWidget * self)
{
  g_message (
    "creating model for ports for type %d and slot "
    "%d",
    self->selected_type, self->selected_slot);
  GtkListStore * list_store;
  GtkTreeIter    iter;

  /* file name, index */
  list_store = gtk_list_store_new (
    3, G_TYPE_STRING, G_TYPE_STRING,
    G_TYPE_POINTER);

  Track * track =
    automation_track_get_track (self->owner);
  AutomationTracklist * atl =
    track_get_automation_tracklist (track);
  for (int i = 0; i < atl->num_ats; i++)
    {
      AutomationTrack * at = atl->ats[i];

      char icon_name[256];

      Port *   port = NULL;
      Plugin * plugin = NULL;
      switch (self->selected_type)
        {
        case AS_TYPE_MIDI_CH1:
        case AS_TYPE_MIDI_CH2:
        case AS_TYPE_MIDI_CH3:
        case AS_TYPE_MIDI_CH4:
        case AS_TYPE_MIDI_CH5:
        case AS_TYPE_MIDI_CH6:
        case AS_TYPE_MIDI_CH7:
        case AS_TYPE_MIDI_CH8:
        case AS_TYPE_MIDI_CH9:
        case AS_TYPE_MIDI_CH10:
        case AS_TYPE_MIDI_CH11:
        case AS_TYPE_MIDI_CH12:
        case AS_TYPE_MIDI_CH13:
        case AS_TYPE_MIDI_CH14:
        case AS_TYPE_MIDI_CH15:
        case AS_TYPE_MIDI_CH16:
          /* skip non-channel automation tracks */
          port = port_find_from_identifier (
            &at->port_id);
          if (!(port->id.flags
                & PORT_FLAG_MIDI_AUTOMATABLE))
            continue;
          /*g_message (*/
          /*"port %s is a midi automatable",*/
          /*port->id.label);*/

          if (
            port->id.flags2
              & PORT_FLAG2_MIDI_PITCH_BEND
            || port->id.flags2
                 & PORT_FLAG2_MIDI_POLY_KEY_PRESSURE
            || port->id.flags2
                 & PORT_FLAG2_MIDI_CHANNEL_PRESSURE)
            {
              if (
                (int) self->selected_type
                != (AS_TYPE_MIDI_CH1 + port->id.port_index))
                continue;
            }
          else
            {
              if (
                (int) self->selected_type
                != (AS_TYPE_MIDI_CH1 + port->id.port_index / 128))
                continue;
            }

          strcpy (icon_name, "signal-midi");
          break;
        case AS_TYPE_MACRO:
          /* skip non-channel automation tracks */
          port = port_find_from_identifier (
            &at->port_id);
          if (!(port->id.flags
                & PORT_FLAG_MODULATOR_MACRO))
            continue;

          strcpy (icon_name, "code-function");
          break;
        case AS_TYPE_CHANNEL:
          /* skip non-channel automation tracks */
          port = port_find_from_identifier (
            &at->port_id);
          if (
            !(port->id.flags & PORT_FLAG_FADER_MUTE
              || port->id.flags & PORT_FLAG_CHANNEL_FADER
              || port->id.flags & PORT_FLAG_STEREO_BALANCE
              || port->id.flags2
                   & PORT_FLAG2_CHANNEL_SEND_ENABLED
              || port->id.flags2
                   & PORT_FLAG2_CHANNEL_SEND_AMOUNT))
            continue;

          strcpy (icon_name, "node-type-cusp");
          break;
        case AS_TYPE_MIDI_FX:
          plugin =
            track->channel
              ->midi_fx[self->selected_slot];
          break;
        case AS_TYPE_INSERT:
          plugin =
            track->channel
              ->inserts[self->selected_slot];
          break;
        case AS_TYPE_INSTRUMENT:
          plugin = track->channel->instrument;
          break;
        case AS_TYPE_MODULATOR:
          plugin =
            track->modulators[self->selected_slot];
          break;
        case AS_TYPE_TEMPO:
          port = port_find_from_identifier (
            &at->port_id);

          /* skip non-tempo automation tracks */
          if (!(port->id.flags & PORT_FLAG_BPM
                || port->id.flags2
                     & PORT_FLAG2_BEATS_PER_BAR
                || port->id.flags2
                     & PORT_FLAG2_BEAT_UNIT))
            continue;
          break;
        }

      if (plugin)
        {
          /* skip non-plugin automation tracks */
          port = port_find_from_identifier (
            &at->port_id);
          if (
            port->id.owner_type
            != PORT_OWNER_TYPE_PLUGIN)
            continue;

          Plugin * port_pl =
            port_get_plugin (port, true);
          if (port_pl != plugin)
            continue;

          strcpy (icon_name, "plugins");
        }

      if (!port)
        continue;

      /* if this automation track is not
       * already in a visible lane */
      if (
        !at->created || !at->visible
        || at == self->owner)
        {
          /*g_message ("adding %s", port->id.label);*/
          /* add a new row to the model */
          gtk_list_store_append (list_store, &iter);
          gtk_list_store_set (
            list_store, &iter, 0, icon_name, 1,
            port->id.label, 2, port, -1);
        }
    }

  return GTK_TREE_MODEL (list_store);
}

static GtkTreeModel *
create_model_for_types (
  AutomatableSelectorPopoverWidget * self)
{
  GtkListStore * list_store;
  GtkTreeIter    iter;

  /* icon, type name, type enum, slot */
  list_store = gtk_list_store_new (
    4, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_INT,
    G_TYPE_INT);

  Track * track =
    automation_track_get_track (self->owner);

  if (track->type == TRACK_TYPE_TEMPO)
    {
      gtk_list_store_append (list_store, &iter);
      gtk_list_store_set (
        list_store, &iter, 0, "filename-bpm-amarok",
        1, _ ("Tempo"), 2, AS_TYPE_TEMPO, 3, 0, -1);
    }
  else if (track->type == TRACK_TYPE_MODULATOR)
    {
      gtk_list_store_append (list_store, &iter);
      gtk_list_store_set (
        list_store, &iter, 0, "code-function", 1,
        _ ("Macros"), 2, AS_TYPE_MACRO, 3, 0, -1);
    }

  if (track_type_has_piano_roll (track->type))
    {
      for (int i = 0; i < 16; i++)
        {
          char name[300];
          sprintf (name, _ ("MIDI Ch%d"), i + 1);
          gtk_list_store_append (list_store, &iter);
          gtk_list_store_set (
            list_store, &iter, 0, "signal-midi", 1,
            name, 2, AS_TYPE_MIDI_CH1 + i, 3, 0,
            -1);
        }
    }

  if (track_type_has_channel (track->type))
    {
      gtk_list_store_append (list_store, &iter);
      gtk_list_store_set (
        list_store, &iter, 0, "track-inspector", 1,
        _ ("Channel"), 2, AS_TYPE_CHANNEL, 3, 0,
        -1);

      if (track->channel->instrument)
        {
          Plugin * plugin =
            track->channel->instrument;
          char label[600];
          sprintf (
            label, _ ("[Instrument] %s"),
            plugin->setting->descr->name);
          gtk_list_store_append (list_store, &iter);
          gtk_list_store_set (
            list_store, &iter, 0, "instrument", 1,
            label, 2, AS_TYPE_INSTRUMENT, 3, 0, -1);
        }

      for (int i = 0; i < STRIP_SIZE; i++)
        {
          Plugin * plugin =
            track->channel->midi_fx[i];
          if (plugin)
            {
              char label[600];
              sprintf (
                label, _ ("[MIDI FX %d] %s"), i + 1,
                plugin->setting->descr->name);
              gtk_list_store_append (
                list_store, &iter);
              gtk_list_store_set (
                list_store, &iter, 0, "plugins", 1,
                label, 2, AS_TYPE_MIDI_FX, 3, i,
                -1);
            }

          plugin = track->channel->inserts[i];
          if (plugin)
            {
              char label[600];
              sprintf (
                label, _ ("[Insert %d] %s"), i + 1,
                plugin->setting->descr->name);
              gtk_list_store_append (
                list_store, &iter);
              gtk_list_store_set (
                list_store, &iter, 0, "plugins", 1,
                label, 2, AS_TYPE_INSERT, 3, i, -1);
            }
        }
    }
  for (int i = 0; i < track->num_modulators; i++)
    {
      Plugin * plugin = track->modulators[i];
      if (plugin)
        {
          char label[600];
          sprintf (
            label, _ ("[Modulator %d] %s"), i + 1,
            plugin->setting->descr->name);
          gtk_list_store_append (list_store, &iter);
          gtk_list_store_set (
            list_store, &iter, 0, "plugins", 1,
            label, 2, AS_TYPE_MODULATOR, 3, i, -1);
        }
    }

  return GTK_TREE_MODEL (list_store);
}

static GtkTreeView *
tree_view_create (
  AutomatableSelectorPopoverWidget * self,
  GtkTreeModel *                     model);

static void
on_selection_changed (
  GtkTreeSelection *                 ts,
  AutomatableSelectorPopoverWidget * self)
{
  if (self->selecting_manually)
    {
      self->selecting_manually = 0;
      return;
    }

  GtkTreeView * tv =
    gtk_tree_selection_get_tree_view (ts);
  GtkTreeModel * model =
    gtk_tree_view_get_model (tv);
  GList * selected_rows =
    gtk_tree_selection_get_selected_rows (ts, NULL);
  if (selected_rows)
    {
      GtkTreePath * tp =
        (GtkTreePath *) g_list_first (selected_rows)
          ->data;
      GtkTreeIter iter;
      gtk_tree_model_get_iter (model, &iter, tp);
      GValue value = G_VALUE_INIT;

      if (model == self->type_model)
        {
          gtk_tree_model_get (
            model, &iter, 2, &self->selected_type,
            3, &self->selected_slot, -1);
          g_message (
            "selected type %d slot %d",
            self->selected_type,
            self->selected_slot);
          self->port_model =
            create_model_for_ports (self);
          self->port_treeview = tree_view_create (
            self, self->port_model);
          z_gtk_widget_destroy_all_children (
            GTK_WIDGET (self->port_treeview_box));
          gtk_box_append (
            self->port_treeview_box,
            GTK_WIDGET (self->port_treeview));

          self->selected_port = NULL;
          update_info_label (self);
        }
      else if (model == self->port_model)
        {
          gtk_tree_model_get_value (
            model, &iter, 2, &value);
          Port * port =
            g_value_get_pointer (&value);

          self->selected_port = port;
          update_info_label (self);
        }
    }
}

static GtkTreeView *
tree_view_create (
  AutomatableSelectorPopoverWidget * self,
  GtkTreeModel *                     model)
{
  /* instantiate tree view using model */
  GtkWidget * tree_view =
    gtk_tree_view_new_with_model (
      GTK_TREE_MODEL (model));

  /* init tree view */
  GtkCellRenderer *   renderer;
  GtkTreeViewColumn * column;

  /* column for icon */
  renderer = gtk_cell_renderer_pixbuf_new ();
  column = gtk_tree_view_column_new_with_attributes (
    "icon", renderer, "icon-name", 0, NULL);
  gtk_tree_view_append_column (
    GTK_TREE_VIEW (tree_view), column);

  /* column for name */
  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes (
    "name", renderer, "text", 1, NULL);
  gtk_tree_view_append_column (
    GTK_TREE_VIEW (tree_view), column);

  /* set search column */
  gtk_tree_view_set_search_column (
    GTK_TREE_VIEW (tree_view), 1);

  /* set headers invisible */
  gtk_tree_view_set_headers_visible (
    GTK_TREE_VIEW (tree_view), 0);

  gtk_widget_set_visible (tree_view, 1);

  g_signal_connect (
    G_OBJECT (gtk_tree_view_get_selection (
      GTK_TREE_VIEW (tree_view))),
    "changed", G_CALLBACK (on_selection_changed),
    self);

  return GTK_TREE_VIEW (tree_view);
}

/**
 * Creates a digital meter with the given type (bpm or position).
 */
AutomatableSelectorPopoverWidget *
automatable_selector_popover_widget_new (
  AutomationTrack * owner)
{
  AutomatableSelectorPopoverWidget * self =
    g_object_new (
      AUTOMATABLE_SELECTOR_POPOVER_WIDGET_TYPE,
      NULL);

  self->owner = owner;

  /* set selected type */
  self->selected_type = AS_TYPE_CHANNEL;
  Port * port = port_find_from_identifier (
    &self->owner->port_id);
  PortIdentifier * id = &port->id;
  if (
    id->flags & PORT_FLAG_BPM
    || id->flags2 & PORT_FLAG2_BEATS_PER_BAR
    || id->flags2 & PORT_FLAG2_BEAT_UNIT)
    {
      self->selected_type = AS_TYPE_TEMPO;
    }
  else if (
    id->flags & PORT_FLAG_FADER_MUTE
    || id->flags & PORT_FLAG_CHANNEL_FADER
    || id->flags & PORT_FLAG_STEREO_BALANCE
    || id->flags2 & PORT_FLAG2_CHANNEL_SEND_ENABLED
    || id->flags2 & PORT_FLAG2_CHANNEL_SEND_AMOUNT)
    {
      self->selected_type = AS_TYPE_CHANNEL;
    }
  else if (id->flags & PORT_FLAG_MIDI_AUTOMATABLE)
    {
      if (
        id->flags2 & PORT_FLAG2_MIDI_PITCH_BEND
        || id->flags2 & PORT_FLAG2_MIDI_POLY_KEY_PRESSURE
        || id->flags2
             & PORT_FLAG2_MIDI_CHANNEL_PRESSURE)
        {
          self->selected_type =
            (AutomatableSelectorType) (AS_TYPE_MIDI_CH1 + id->port_index);
        }
      else
        {
          self->selected_type =
            (AutomatableSelectorType) (AS_TYPE_MIDI_CH1 + id->port_index / 128);
        }
    }
  else if (id->flags & PORT_FLAG_MODULATOR_MACRO)
    {
      self->selected_type = AS_TYPE_MACRO;
    }
  else if (
    id->flags & PORT_FLAG_PLUGIN_CONTROL
    || id->flags & PORT_FLAG_GENERIC_PLUGIN_PORT)
    {
      PluginIdentifier * pl_id = &id->plugin_id;
      switch (pl_id->slot_type)
        {
        case PLUGIN_SLOT_MIDI_FX:
          self->selected_type = AS_TYPE_MIDI_FX;
          self->selected_slot = pl_id->slot;
          break;
        case PLUGIN_SLOT_INSTRUMENT:
          self->selected_type = AS_TYPE_INSTRUMENT;
          break;
        case PLUGIN_SLOT_INSERT:
          self->selected_type = AS_TYPE_INSERT;
          self->selected_slot = pl_id->slot;
          break;
        case PLUGIN_SLOT_MODULATOR:
          self->selected_type = AS_TYPE_MODULATOR;
          self->selected_slot = pl_id->slot;
          break;
        default:
          g_return_val_if_reached (NULL);
          break;
        }
    }

  /* set selected automatable */
  self->selected_port = port;

  /* create model/treeview for types */
  self->type_model = create_model_for_types (self);
  self->type_treeview =
    tree_view_create (self, self->type_model);
  gtk_box_append (
    self->type_treeview_box,
    GTK_WIDGET (self->type_treeview));

  /* create model/treeview for ports */
  self->port_model = create_model_for_ports (self);
  self->port_treeview =
    tree_view_create (self, self->port_model);
  gtk_box_append (
    self->port_treeview_box,
    GTK_WIDGET (self->port_treeview));

  /* select the automatable */
  self->selecting_manually = 1;
  select_automatable (self);

  return self;
}

static void
automatable_selector_popover_widget_class_init (
  AutomatableSelectorPopoverWidgetClass * _klass)
{
  GtkWidgetClass * klass =
    GTK_WIDGET_CLASS (_klass);
  resources_set_class_template (
    klass, "automatable_selector.ui");

#define BIND_CHILD(x) \
  gtk_widget_class_bind_template_child ( \
    klass, AutomatableSelectorPopoverWidget, x)

  BIND_CHILD (type_treeview_box);
  BIND_CHILD (port_treeview_box);
  BIND_CHILD (info);
  gtk_widget_class_bind_template_callback (
    klass, on_closed);

#undef BIND_CHILD
}

static void
automatable_selector_popover_widget_init (
  AutomatableSelectorPopoverWidget * self)
{
  gtk_widget_init_template (GTK_WIDGET (self));
}
