// SPDX-FileCopyrightText: © 2019-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "actions/port_connection_action.h"
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

#include "gtk_wrapper.h"

/* FIXME this should re-use the same logic from automatable selector popover */

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
          ui_show_error_message (
            _ ("Connection Failed"), _ ("No port selected"));
          return;
        }

      Port *src = nullptr, *dest = NULL;
      if (self->port->id_.flow_ == PortFlow::Input)
        {
          src = self->selected_port;
          dest = self->port;
        }
      else if (self->port->id_.flow_ == PortFlow::Output)
        {
          src = self->port;
          dest = self->selected_port;
        }

      z_return_if_fail (src && dest);

      if (src->can_be_connected_to (*dest))
        {
          try
            {
              UNDO_MANAGER->perform (
                std::make_unique<PortConnectionConnectAction> (
                  src->id_, dest->id_));
            }
          catch (const ZrythmException &e)
            {
              e.handle (format_str (
                _ ("Failed to connect %s to %s"), src->get_label (),
                dest->get_label ()));
            }

          port_connections_popover_widget_refresh (self->owner, self->port);
        }
      else
        {
          ui_show_error_message (
            _ ("Connection Failed"), _ ("These ports cannot be connected"));
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
  /* icon, name, pointer to port */
  GtkListStore * list_store =
    gtk_list_store_new (3, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_POINTER);

  PortType type = self->port->id_.type_;
  PortFlow flow = self->port->id_.flow_;

  /* Add a new row to the model if not already in the destinations */
  auto add_row = [flow, self, list_store] (Port &port) {
    if (
      (flow == PortFlow::Input && !port.is_connected_to (*self->port))
      || (flow == PortFlow::Output && !self->port->is_connected_to (port)))
      {
        GtkTreeIter iter;
        gtk_list_store_append (list_store, &iter);
        gtk_list_store_set (
          list_store, &iter, 0, "automation-4p", 1, port.get_label ().c_str (),
          2, &port, -1);
      }
  };

  /* if filtering to track ports */
  if (track)
    {
      auto ch_track = dynamic_cast<ChannelTrack *> (track);
      auto processable_track = dynamic_cast<ProcessableTrack *> (track);
      if (flow == PortFlow::Input)
        {
          if (
            (type == PortType::Audio || type == PortType::CV)
            && track->out_signal_type_ == PortType::Audio && ch_track)
            {
              add_row (ch_track->channel_->prefader_->stereo_out_->get_l ());
              add_row (ch_track->channel_->prefader_->stereo_out_->get_r ());
              add_row (ch_track->channel_->fader_->stereo_out_->get_l ());
              add_row (ch_track->channel_->fader_->stereo_out_->get_r ());
            }
          else if (
            type == PortType::Event
            && track->out_signal_type_ == PortType::Event && ch_track)
            {
              add_row (*ch_track->channel_->midi_out_);
            }
        }
      else if (flow == PortFlow::Output)
        {
          if (type == PortType::Audio)
            {
              if (track->in_signal_type_ == PortType::Audio && processable_track)
                {
                  add_row (processable_track->processor_->stereo_in_->get_l ());
                  add_row (processable_track->processor_->stereo_in_->get_r ());
                }
            }
          else if (type == PortType::CV)
            {
              if (ch_track)
                {
                  add_row (*ch_track->channel_->fader_->amp_);
                  add_row (*ch_track->channel_->fader_->balance_);
                  for (int j = 0; j < STRIP_SIZE; j++)
                    {
                      auto &send = ch_track->channel_->sends_[j];
                      add_row (*send->amount_);
                    }
                }
              if (track->is_modulator ())
                {
                  auto modulator_track = dynamic_cast<ModulatorTrack *> (track);
                  for (
                    auto &macro : modulator_track->modulator_macro_processors_)
                    {
                      add_row (*macro->cv_in_);
                    }
                }
            }
          else if (type == PortType::Event)
            {
              if (track->in_signal_type_ == PortType::Event && processable_track)
                {
                  add_row (*processable_track->processor_->midi_in_);
                }
            }
        } /* endif output */
    } /* endif filtering to track ports */
  /* else if filtering to plugin ports */
  else if (pl)
    {
      if (flow == PortFlow::Input)
        {
          for (auto &port : pl->out_ports_)
            {
              if (
                (port->id_.type_ != type)
                && !(
                  port->id_.type_ == PortType::CV && type == PortType::Control))
                continue;

              add_row (*port);
            }
        }
      else if (flow == PortFlow::Output)
        {
          for (auto &port : pl->in_ports_)
            {
              if (
                (port->id_.type_ != type)
                && !(
                  type == PortType::CV && port->id_.type_ == PortType::Control))
                continue;

              add_row (*port);
            }
        }
    }

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

  for (auto &track : TRACKLIST->tracks_)
    {
      if (!track->is_modulator () && !track->has_channel ())
        continue;

      // Add a new row to the model
      gtk_list_store_append (list_store, &iter);
      gtk_list_store_set (
        list_store, &iter, 0, "track-inspector", 1, track->name_.c_str (), 2,
        track.get (), -1);
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
  PortIdentifier * id = &port->id_;

  /* skip if no plugin or the plugin is the
   * port's plugin */
  if (
    !pl
    || (id->owner_type_ == PortIdentifier::OwnerType::Plugin && pl == self->port->get_plugin (true)))
    {
      return;
    }

  // Add a new row to the model
  gtk_list_store_append (list_store, iter);
  gtk_list_store_set (
    list_store, iter, 0, "plugins", 1, pl->get_name ().c_str (), 2, pl, -1);
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
  PortIdentifier * id = &port->id_;
  if (track)
    {
      /* skip track ports if the owner port is a track port of the same track */
      Track * port_track = port->get_track (0);
      if (
        !((id->owner_type_ == PortIdentifier::OwnerType::Track
           && port_track == track)
          || (id->owner_type_ == PortIdentifier::OwnerType::Fader && port_track == track)
          || (id->owner_type_ == PortIdentifier::OwnerType::TrackProcessor && port_track == track)))
        {
          // Add a new row to the model
          gtk_list_store_append (list_store, &iter);
          gtk_list_store_set (
            list_store, &iter, 0, "folder", 1, _ ("Track Ports"), 2,
            dummy_plugin, -1);
        }

      auto ch_track = dynamic_cast<ChannelTrack *> (track);
      if (ch_track)
        {
          for (auto &pl : ch_track->channel_->midi_fx_)
            {
              add_plugin (self, pl.get (), list_store, &iter);
            }
          for (auto &pl : ch_track->channel_->inserts_)
            {
              add_plugin (self, pl.get (), list_store, &iter);
            }

          add_plugin (
            self, ch_track->channel_->instrument_.get (), list_store, &iter);
        }
      if (auto mod_track = dynamic_cast<ModulatorTrack *> (track))
        {
          for (auto &pl : mod_track->modulators_)
            {
              add_plugin (self, pl.get (), list_store, &iter);
            }
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
  GList * selected_rows = gtk_tree_selection_get_selected_rows (ts, nullptr);
  if (selected_rows)
    {
      GtkTreePath * tp = (GtkTreePath *) g_list_first (selected_rows)->data;
      GtkTreeIter   iter;
      gtk_tree_model_get_iter (model, &iter, tp);
      GValue value = G_VALUE_INIT;

      if (model == self->track_model)
        {
          gtk_tree_model_get_value (model, &iter, 2, &value);
          self->selected_track =
            static_cast<Track *> (g_value_get_pointer (&value));
          self->plugin_model =
            create_model_for_plugins (self, self->selected_track);
          tree_view_setup (self, self->plugin_model, false);
          self->port_model = create_model_for_ports (self, nullptr, nullptr);
          tree_view_setup (self, self->port_model, false);
        }
      else if (model == self->plugin_model)
        {
          gtk_tree_model_get_value (model, &iter, 2, &value);
          Plugin * selected_pl =
            static_cast<Plugin *> (g_value_get_pointer (&value));
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
              create_model_for_ports (self, self->selected_track, nullptr);
          else
            self->port_model =
              create_model_for_ports (self, nullptr, self->selected_plugin);
          tree_view_setup (self, self->port_model, false);
        }
      else if (model == self->port_model)
        {
          gtk_tree_model_get_value (model, &iter, 2, &value);
          self->selected_port =
            static_cast<Port *> (g_value_get_pointer (&value));
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
        "icon", renderer, "icon-name", 0, nullptr);
      gtk_tree_view_append_column (GTK_TREE_VIEW (tree_view), column);

      /* column for name */
      renderer = gtk_cell_renderer_text_new ();
      column = gtk_tree_view_column_new_with_attributes (
        "name", renderer, "text", 1, nullptr);
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

  self->plugin_model = create_model_for_plugins (self, nullptr);
  tree_view_setup (self, self->plugin_model, !self->setup);

  self->port_model = create_model_for_ports (self, nullptr, nullptr);
  tree_view_setup (self, self->port_model, !self->setup);

  self->setup = true;
}

/**
 * Creates the popover.
 */
PortSelectorDialogWidget *
port_selector_dialog_widget_new (PortConnectionsPopoverWidget * owner)
{
  PortSelectorDialogWidget * self = Z_PORT_SELECTOR_DIALOG_WIDGET (
    g_object_new (PORT_SELECTOR_DIALOG_WIDGET_TYPE, "modal", true, nullptr));

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
