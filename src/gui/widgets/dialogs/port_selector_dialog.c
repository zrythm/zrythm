// SPDX-FileCopyrightText: © 2019-2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "actions/undo_manager.h"
#include "actions/undoable_action.h"
#include "dsp/port.h"
#include "dsp/router.h"
#include "dsp/track.h"
#include "dsp/tracklist.h"
#include "gui/widgets/dialogs/port_selector_dialog.h"
#include "gui/widgets/main_window.h"
#include "gui/widgets/popovers/port_connections_popover.h"
#include "plugins/plugin.h"
#include "project.h"
#include "utils/error.h"
#include "utils/resources.h"
#include "utils/ui.h"
#include "zrythm_app.h"

#include <glib/gi18n.h>
#include <gtk/gtk.h>

G_DEFINE_TYPE (
  PortSelectorDialogWidget,
  port_selector_dialog_widget,
  GTK_TYPE_DIALOG)

/** Used as the first row in the Plugin treeview to
 * indicate if "Track ports is selected or not. */
static const Plugin * dummy_plugin = (const Plugin *) 123;

static void
on_response (GtkDialog * dialog, gint response_id, PortSelectorDialogWidget * self)
{
  if (response_id == GTK_RESPONSE_OK)
    {
      if (!self->selected_port)
        {
          ui_show_error_message (false, _ ("No port selected"));
          return;
        }

      Port *src = NULL, *dest = NULL;
      if (self->port->id.flow == FLOW_INPUT)
        {
          src = self->selected_port;
          dest = self->port;
        }
      else if (self->port->id.flow == FLOW_OUTPUT)
        {
          src = self->port;
          dest = self->selected_port;
        }

      g_return_if_fail (src && dest);

      if (ports_can_be_connected (src, dest))
        {
          GError * err = NULL;
          bool     ret =
            port_connection_action_perform_connect (&src->id, &dest->id, &err);
          if (!ret)
            {
              HANDLE_ERROR (
                err, _ ("Failed to connect %s to %s"), src->id.label,
                dest->id.label);
            }

          port_connections_popover_widget_refresh (self->owner, self->port);
        }
      else
        {
          ui_show_error_message (false, _ ("These ports cannot be connected"));
        }
    }

  gtk_window_destroy (GTK_WINDOW (dialog));
}

/**
 * @param track Track, if track ports selected.
 * @param pl Plugin, if plugin selected.
 */
static GtkTreeModel *
create_model_for_ports (
  PortSelectorDialogWidget * self,
  Track *                    track,
  Plugin *                   pl)
{
  GtkListStore * list_store;
  GtkTreeIter    iter;

  /* icon, name, pointer to port */
  list_store =
    gtk_list_store_new (3, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_POINTER);

  PortType type = self->port->id.type;
  PortFlow flow = self->port->id.flow;

/* Add a new row to the model if not already
 * in the destinations */
#define ADD_ROW \
  if ( \
    (flow == FLOW_INPUT && !ports_connected (port, self->port)) \
    || (flow == FLOW_OUTPUT && !ports_connected (self->port, port))) \
    { \
      gtk_list_store_append (list_store, &iter); \
      gtk_list_store_set ( \
        list_store, &iter, 0, "node-type-cusp", 1, port->id.label, 2, port, \
        -1); \
    }

  /* if filtering to track ports */
  if (track)
    {
      Channel * ch = track->channel;
      if (flow == FLOW_INPUT)
        {
          if (
            (type == TYPE_AUDIO || type == TYPE_CV)
            && track->out_signal_type == TYPE_AUDIO)
            {
              Port * port;
              port = ch->prefader->stereo_out->l;
              ADD_ROW;
              port = ch->prefader->stereo_out->r;
              ADD_ROW;
              port = ch->fader->stereo_out->l;
              ADD_ROW;
              port = ch->fader->stereo_out->r;
              ADD_ROW;
            }
          else if (type == TYPE_EVENT && track->out_signal_type == TYPE_EVENT)
            {
              Port * port;
              port = ch->midi_out;
              ADD_ROW;
            }
        }
      else if (flow == FLOW_OUTPUT)
        {
          if (type == TYPE_AUDIO)
            {
              if (track->in_signal_type == TYPE_AUDIO)
                {
                  Port * port;
                  port = track->processor->stereo_in->l;
                  ADD_ROW;
                  port = track->processor->stereo_in->r;
                  ADD_ROW;
                }
            }
          else if (type == TYPE_CV)
            {
              if (track->channel)
                {
                  Port * port;
                  port = track->channel->fader->amp;
                  ADD_ROW;
                  port = track->channel->fader->balance;
                  ADD_ROW;
                }
              if (track->type == TRACK_TYPE_MODULATOR)
                {
                  for (int j = 0; j < track->num_modulator_macros; j++)
                    {
                      Port * port;
                      port = track->modulator_macros[j]->cv_in;
                      ADD_ROW;
                    }
                }
            }
          else if (type == TYPE_EVENT)
            {
              if (track->in_signal_type == TYPE_EVENT)
                {
                  Port * port;
                  port = track->processor->midi_in;
                  ADD_ROW;
                }
            }
        } /* endif output */
    }     /* endif filtering to track ports */
  /* else if filtering to plugin ports */
  else if (pl)
    {
      if (flow == FLOW_INPUT)
        {
          for (int i = 0; i < pl->num_out_ports; i++)
            {
              Port * port;
              port = pl->out_ports[i];

              if (
                (port->id.type != type)
                && !(port->id.type == TYPE_CV && type == TYPE_CONTROL))
                continue;

              ADD_ROW;
            }
        }
      else if (flow == FLOW_OUTPUT)
        {
          for (int i = 0; i < pl->num_in_ports; i++)
            {
              Port * port;
              port = pl->in_ports[i];

              if (
                (port->id.type != type)
                && !(type == TYPE_CV && port->id.type == TYPE_CONTROL))
                continue;

              ADD_ROW;
            }
        }
    }

#undef ADD_ROW

  return GTK_TREE_MODEL (list_store);
}

static GtkTreeModel *
create_model_for_tracks (PortSelectorDialogWidget * self)
{
  GtkListStore * list_store;
  GtkTreeIter    iter;

  /* icon, name, pointer to channel */
  list_store =
    gtk_list_store_new (3, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_POINTER);

  Track * track;
  for (int i = 0; i < TRACKLIST->num_tracks; i++)
    {
      track = TRACKLIST->tracks[i];

      if (track->type != TRACK_TYPE_MODULATOR && !track->channel)
        continue;

      // Add a new row to the model
      gtk_list_store_append (list_store, &iter);
      gtk_list_store_set (
        list_store, &iter, 0, "track-inspector", 1, track->name, 2, track, -1);
    }

  return GTK_TREE_MODEL (list_store);
}

static void
add_plugin (
  PortSelectorDialogWidget * self,
  Plugin *                   pl,
  GtkListStore *             list_store,
  GtkTreeIter *              iter)
{
  Port *           port = self->port;
  PortIdentifier * id = &port->id;

  /* skip if no plugin or the plugin is the
   * port's plugin */
  if (
    !pl
    || (id->owner_type == PORT_OWNER_TYPE_PLUGIN && pl == port_get_plugin (self->port, true)))
    {
      return;
    }

  // Add a new row to the model
  gtk_list_store_append (list_store, iter);
  gtk_list_store_set (
    list_store, iter, 0, "plugins", 1, pl->setting->descr->name, 2, pl, -1);
}

/* FIXME leaking if model already exists */
static GtkTreeModel *
create_model_for_plugins (PortSelectorDialogWidget * self, Track * track)
{
  GtkListStore * list_store;
  GtkTreeIter    iter;

  /* icon, name, pointer to plugin */
  list_store =
    gtk_list_store_new (3, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_POINTER);

  Port *           port = self->port;
  PortIdentifier * id = &port->id;
  if (track)
    {
      /* skip track ports if the owner port is
       * a track port of the same track */
      Track * port_track = port_get_track (port, 0);
      if (
        !((id->owner_type == PORT_OWNER_TYPE_TRACK && port_track == track)
          || (id->owner_type == PORT_OWNER_TYPE_FADER && port_track == track)
          || (id->owner_type == PORT_OWNER_TYPE_TRACK_PROCESSOR && port_track == track)))
        {
          // Add a new row to the model
          gtk_list_store_append (list_store, &iter);
          gtk_list_store_set (
            list_store, &iter, 0, "folder", 1, _ ("Track Ports"), 2,
            dummy_plugin, -1);
        }

      Channel * ch = track->channel;
      Plugin *  pl;

      if (ch)
        {
          for (int i = 0; i < STRIP_SIZE; i++)
            {
              pl = ch->midi_fx[i];
              add_plugin (self, pl, list_store, &iter);
            }
          for (int i = 0; i < STRIP_SIZE; i++)
            {
              pl = ch->inserts[i];
              add_plugin (self, pl, list_store, &iter);
            }

          add_plugin (self, ch->instrument, list_store, &iter);
        }
      for (int i = 0; i < track->num_modulators; i++)
        {
          pl = track->modulators[i];
          add_plugin (self, pl, list_store, &iter);
        }
    }

  return GTK_TREE_MODEL (list_store);
}

static void
tree_view_setup (PortSelectorDialogWidget * self, GtkTreeModel * model, bool init);

static void
on_selection_changed (GtkTreeSelection * ts, PortSelectorDialogWidget * self)
{
  GtkTreeView *  tv = gtk_tree_selection_get_tree_view (ts);
  GtkTreeModel * model = gtk_tree_view_get_model (tv);
  GList * selected_rows = gtk_tree_selection_get_selected_rows (ts, NULL);
  if (selected_rows)
    {
      GtkTreePath * tp = (GtkTreePath *) g_list_first (selected_rows)->data;
      GtkTreeIter   iter;
      gtk_tree_model_get_iter (model, &iter, tp);
      GValue value = G_VALUE_INIT;

      if (model == self->track_model)
        {
          gtk_tree_model_get_value (model, &iter, 2, &value);
          self->selected_track = g_value_get_pointer (&value);
          self->plugin_model =
            create_model_for_plugins (self, self->selected_track);
          tree_view_setup (self, self->plugin_model, false);
          self->port_model = create_model_for_ports (self, NULL, NULL);
          tree_view_setup (self, self->port_model, false);
        }
      else if (model == self->plugin_model)
        {
          gtk_tree_model_get_value (model, &iter, 2, &value);
          Plugin * selected_pl = g_value_get_pointer (&value);
          if (selected_pl == dummy_plugin)
            {
              self->selected_plugin = NULL;
              self->track_ports_selected = 1;
            }
          else
            {
              self->selected_plugin = selected_pl;
              self->track_ports_selected = 0;
            }

          if (self->track_ports_selected)
            self->port_model =
              create_model_for_ports (self, self->selected_track, NULL);
          else
            self->port_model =
              create_model_for_ports (self, NULL, self->selected_plugin);
          tree_view_setup (self, self->port_model, false);
        }
      else if (model == self->port_model)
        {
          gtk_tree_model_get_value (model, &iter, 2, &value);
          self->selected_port = g_value_get_pointer (&value);
        }
    }
}

/**
 * @param init Initialize columns, only the first time.
 */
static void
tree_view_setup (PortSelectorDialogWidget * self, GtkTreeModel * model, bool init)
{
  GtkTreeView * tree_view = NULL;

  if (model == self->track_model)
    tree_view = self->track_treeview;
  else if (model == self->plugin_model)
    tree_view = self->plugin_treeview;
  else if (model == self->port_model)
    tree_view = self->port_treeview;

  /* instantiate tree view using model */
  gtk_tree_view_set_model (tree_view, model);

  if (init)
    {
      /* init tree view */
      GtkCellRenderer *   renderer;
      GtkTreeViewColumn * column;

      /* column for icon */
      renderer = gtk_cell_renderer_pixbuf_new ();
      column = gtk_tree_view_column_new_with_attributes (
        "icon", renderer, "icon-name", 0, NULL);
      gtk_tree_view_append_column (GTK_TREE_VIEW (tree_view), column);

      /* column for name */
      renderer = gtk_cell_renderer_text_new ();
      column = gtk_tree_view_column_new_with_attributes (
        "name", renderer, "text", 1, NULL);
      gtk_tree_view_append_column (GTK_TREE_VIEW (tree_view), column);

      /* set search column */
      gtk_tree_view_set_search_column (GTK_TREE_VIEW (tree_view), 1);

      /* set headers invisible */
      gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (tree_view), false);

      g_signal_connect (
        G_OBJECT (gtk_tree_view_get_selection (GTK_TREE_VIEW (tree_view))),
        "changed", G_CALLBACK (on_selection_changed), self);
    }
}

void
port_selector_dialog_widget_refresh (PortSelectorDialogWidget * self, Port * port)
{
  self->port = port;
  self->track_model = create_model_for_tracks (self);
  tree_view_setup (self, self->track_model, !self->setup);

  self->plugin_model = create_model_for_plugins (self, NULL);
  tree_view_setup (self, self->plugin_model, !self->setup);

  self->port_model = create_model_for_ports (self, NULL, NULL);
  tree_view_setup (self, self->port_model, !self->setup);

  self->setup = true;
}

/**
 * Creates the popover.
 */
PortSelectorDialogWidget *
port_selector_dialog_widget_new (PortConnectionsPopoverWidget * owner)
{
  PortSelectorDialogWidget * self =
    g_object_new (PORT_SELECTOR_DIALOG_WIDGET_TYPE, "modal", true, NULL);

  GtkRoot * root = gtk_widget_get_root (GTK_WIDGET (owner));
  gtk_window_set_transient_for (GTK_WINDOW (self), GTK_WINDOW (root));

  self->owner = owner;

  return self;
}

static void
port_selector_dialog_widget_class_init (PortSelectorDialogWidgetClass * _klass)
{
  GtkWidgetClass * klass = GTK_WIDGET_CLASS (_klass);
  resources_set_class_template (klass, "port_selector_dialog.ui");

#define BIND_CHILD(x) \
  gtk_widget_class_bind_template_child (klass, PortSelectorDialogWidget, x)

  BIND_CHILD (track_scroll);
  BIND_CHILD (track_treeview);
  BIND_CHILD (plugin_scroll);
  BIND_CHILD (plugin_treeview);
  BIND_CHILD (plugin_separator);
  BIND_CHILD (port_scroll);
  BIND_CHILD (port_treeview);
  BIND_CHILD (ok_btn);
  BIND_CHILD (cancel_btn);

#undef BIND_CHILD
}

static void
port_selector_dialog_widget_init (PortSelectorDialogWidget * self)
{
  gtk_widget_init_template (GTK_WIDGET (self));

  /* set max height */
  int max_height = 380;
  gtk_scrolled_window_set_max_content_height (self->track_scroll, max_height);
  gtk_scrolled_window_set_max_content_height (self->plugin_scroll, max_height);
  gtk_scrolled_window_set_max_content_height (self->port_scroll, max_height);

  g_signal_connect (G_OBJECT (self), "response", G_CALLBACK (on_response), self);
}
