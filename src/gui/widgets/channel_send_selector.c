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
 */

#include "actions/channel_send_action.h"
#include "audio/channel_send.h"
#include "audio/track.h"
#include "audio/tracklist.h"
#include "gui/widgets/channel_send.h"
#include "gui/widgets/channel_send_selector.h"
#include "plugins/plugin_identifier.h"
#include "project.h"
#include "utils/error.h"
#include "utils/objects.h"
#include "zrythm_app.h"

#include <glib/gi18n.h>

G_DEFINE_TYPE (
  ChannelSendSelectorWidget,
  channel_send_selector_widget,
  GTK_TYPE_POPOVER)

/* FIXME lots of memory leaks */

/**
 * Target type.
 */
typedef enum ChannelSendTargetType
{
  /** Remove send. */
  TARGET_TYPE_NONE,

  /** Send to track inputs. */
  TARGET_TYPE_TRACK,

  /** Send to plugin sidechain inputs. */
  TARGET_TYPE_PLUGIN_SIDECHAIN,
} ChannelSendTargetType;

typedef struct ChannelSendTarget
{
  ChannelSendTargetType type;

  int track_pos;

  PluginIdentifier pl_id;

  char * port_group;
} ChannelSendTarget;

static Track *
get_track_from_target (ChannelSendTarget * target)
{
  if (target->type == TARGET_TYPE_NONE)
    return NULL;

  return TRACKLIST->tracks[target->track_pos];
}

static StereoPorts *
get_sidechain_from_target (
  ChannelSendTarget * target)
{
  if (target->type != TARGET_TYPE_PLUGIN_SIDECHAIN)
    {
      return NULL;
    }

  Plugin * pl = plugin_find (&target->pl_id);
  g_return_val_if_fail (pl, NULL);
  Port * l = plugin_get_port_in_group (
    pl, target->port_group, true);
  Port * r = plugin_get_port_in_group (
    pl, target->port_group, false);
  return stereo_ports_new_from_existing (l, r);
}

#if 0
static void
on_ok_clicked (
  GtkButton * btn,
  ChannelSendSelectorWidget * self)
{
  gtk_widget_destroy (GTK_WIDGET (self));
}
#endif

static void
on_selection_changed (
  GtkTreeSelection *          ts,
  ChannelSendSelectorWidget * self)
{
  GList * selected_rows =
    gtk_tree_selection_get_selected_rows (ts, NULL);
  if (!selected_rows)
    return;

  GtkTreePath * tp =
    (GtkTreePath *) g_list_first (selected_rows)
      ->data;
  GtkTreeIter iter;
  gtk_tree_model_get_iter (self->model, &iter, tp);
  GValue value = G_VALUE_INIT;
  gtk_tree_model_get_value (
    self->model, &iter, 2, &value);
  ChannelSendTarget * target =
    g_value_get_pointer (&value);

  ChannelSend * send = self->send_widget->send;
  bool is_empty = channel_send_is_empty (send);

  Track * src_track = channel_send_get_track (send);
  Track *          dest_track = NULL;
  StereoPorts *    dest_sidechain = NULL;
  PortConnection * conn = NULL;
  ;
  switch (target->type)
    {
    case TARGET_TYPE_NONE:
      if (channel_send_is_enabled (send))
        {
          GError * err = NULL;
          bool     ret =
            channel_send_action_perform_disconnect (
              self->send_widget->send, &err);
          if (!ret)
            {
              HANDLE_ERROR (
                err, "%s",
                _ ("Failed to disconnect send"));
            }
        }
      break;
    case TARGET_TYPE_TRACK:
      dest_track = get_track_from_target (target);
      switch (src_track->out_signal_type)
        {
        case TYPE_EVENT:
          if (
            port_connections_manager_get_sources_or_dests (
              PORT_CONNECTIONS_MGR, NULL,
              &send->midi_out->id, false)
            == 1)
            {
              conn =
                port_connections_manager_get_source_or_dest (
                  PORT_CONNECTIONS_MGR,
                  &send->midi_out->id, false);
            }
          if (is_empty
              ||
              (conn
               &&
               !port_identifier_is_equal (
                 conn->dest_id,
                 &dest_track->processor->midi_in->
                   id)))
            {
              GError * err = NULL;
              bool     ret =
                channel_send_action_perform_connect_midi (
                  send,
                  dest_track->processor->midi_in,
                  &err);
              if (!ret)
                {
                  HANDLE_ERROR (
                    err, "%s",
                    _ ("Failed to connect send"));
                }
            }
          break;
        case TYPE_AUDIO:
          if (
            port_connections_manager_get_sources_or_dests (
              PORT_CONNECTIONS_MGR, NULL,
              &send->stereo_out->l->id, false)
            == 1)
            {
              conn =
                port_connections_manager_get_source_or_dest (
                  PORT_CONNECTIONS_MGR,
                  &send->stereo_out->l->id, false);
            }
          if (is_empty
              ||
              (conn
               &&
               !port_identifier_is_equal (
                  conn->dest_id,
                  &dest_track->processor->stereo_in->
                    l->id)))
            {
              GError * err = NULL;
              bool     ret =
                channel_send_action_perform_connect_audio (
                  send,
                  dest_track->processor->stereo_in,
                  &err);
              if (!ret)
                {
                  HANDLE_ERROR (
                    err, "%s",
                    _ ("Failed to connect send"));
                }
            }
          break;
        default:
          break;
        }
      break;
    case TARGET_TYPE_PLUGIN_SIDECHAIN:
      {
        dest_sidechain =
          get_sidechain_from_target (target);
        if (
          port_connections_manager_get_sources_or_dests (
            PORT_CONNECTIONS_MGR, NULL,
            &send->stereo_out->l->id, false)
          == 1)
          {
            conn =
              port_connections_manager_get_source_or_dest (
                PORT_CONNECTIONS_MGR,
                &send->stereo_out->l->id, false);
          }
        if (dest_sidechain &&
            (is_empty
             || !send->is_sidechain
             ||
             (conn
              &&
              !port_identifier_is_equal (
                conn->dest_id,
                &dest_sidechain->l->id))))
          {
            GError * err = NULL;
            bool     ret =
              channel_send_action_perform_connect_sidechain (
                send, dest_sidechain, &err);
            if (!ret)
              {
                HANDLE_ERROR (
                  err, "%s",
                  _ ("Failed to connect send"));
              }
          }
        object_zero_and_free (dest_sidechain);
      }
      break;
    }

  gtk_widget_queue_draw (
    GTK_WIDGET (self->send_widget));
}

static void
setup_treeview (ChannelSendSelectorWidget * self)
{
  /* icon, name, pointer to data */
  GtkListStore * list_store = gtk_list_store_new (
    3, G_TYPE_STRING, G_TYPE_STRING,
    G_TYPE_POINTER);

  GtkTreeSelection * sel =
    gtk_tree_view_get_selection (
      GTK_TREE_VIEW (self->treeview));

  ChannelSendTarget * target =
    object_new (ChannelSendTarget);
  target->type = TARGET_TYPE_NONE;
  GtkTreeIter iter;
  gtk_list_store_append (list_store, &iter);
  gtk_list_store_set (
    list_store, &iter, 0, "edit-none", 1,
    _ ("None"), 2, target, -1);
  int select_idx = 0;
  int count = 1;

  /* setup tracks */
  ChannelSend * send = self->send_widget->send;
  Track * track = channel_send_get_track (send);
  for (int i = 0; i < TRACKLIST->num_tracks; i++)
    {
      Track * target_track = TRACKLIST->tracks[i];

      g_debug ("target %s", target_track->name);

      /* skip tracks with non-matching signal types */
      if (
        target_track == track
        || track->out_signal_type
             != target_track->in_signal_type
        || !track_type_is_fx (target_track->type))
        continue;
      g_message ("adding %s", target_track->name);

      /* create target */
      target = object_new (ChannelSendTarget);
      target->type = TARGET_TYPE_TRACK;
      target->track_pos = i;

      /* add it to list */
      gtk_list_store_append (list_store, &iter);
      gtk_list_store_set (
        list_store, &iter, 0, "media-album-track",
        1, target_track->name, 2, target, -1);

      if (
        channel_send_is_enabled (send)
        && target_track
             == channel_send_get_target_track (
               send, NULL))
        {
          select_idx = count;
        }
      count++;
    }

  /* setup plugin sidechain inputs */
  g_debug ("setting up sidechains");
  for (int i = 0; i < TRACKLIST->num_tracks; i++)
    {
      Track * target_track = TRACKLIST->tracks[i];

      if (
        target_track == track
        || !track_type_has_channel (
          target_track->type))
        {
          continue;
        }

      Channel * ch = target_track->channel;

      Plugin * plugins[300];
      int      num_plugins =
        channel_get_plugins (ch, plugins);

      for (int j = 0; j < num_plugins; j++)
        {
          Plugin * pl = plugins[j];
          g_debug (
            "plugin %s", pl->setting->descr->name);

          for (int k = 0; k < pl->num_in_ports; k++)
            {
              Port * port = pl->in_ports[k];
              g_debug ("port %s", port->id.label);

              if (
                !(port->id.flags
                  & PORT_FLAG_SIDECHAIN)
                || port->id.type != TYPE_AUDIO
                || !port->id.port_group
                || !(
                  port->id.flags
                  & PORT_FLAG_STEREO_L))
                {
                  continue;
                }

              /* find corresponding port in the same
               * port group (e.g., if
               * this is left, find right and vice
               * versa) */
              Port * other_channel =
                plugin_get_port_in_same_group (
                  pl, port);
              if (!other_channel)
                {
                  continue;
                }
              g_debug (
                "other channel %s",
                other_channel->id.label);

              Port *l = NULL, *r = NULL;
              if (
                port->id.flags & PORT_FLAG_STEREO_L
                && other_channel->id.flags
                     & PORT_FLAG_STEREO_R)
                {
                  l = port;
                  r = other_channel;
                }
              if (!l || !r)
                {
                  continue;
                }

              /* create target */
              target =
                object_new (ChannelSendTarget);
              target->type =
                TARGET_TYPE_PLUGIN_SIDECHAIN;
              target->track_pos = target_track->pos;
              target->pl_id = pl->id;
              target->port_group =
                g_strdup (l->id.port_group);

              char designation[1200];
              plugin_get_full_port_group_designation (
                pl, port->id.port_group,
                designation);

              /* add it to list */
              gtk_list_store_append (
                list_store, &iter);
              gtk_list_store_set (
                list_store, &iter, 0,
                "media-album-track", 1,
                designation, 2, target, -1);

              if (channel_send_is_target_sidechain (
                    send))
                {
                  StereoPorts * sp =
                    channel_send_get_target_sidechain (
                      send);
                  if (sp->l == l && sp->r == r)
                    {
                      select_idx = count;
                    }
                  object_zero_and_free (sp);
                }
              count++;
            }
        }
    }

  self->model = GTK_TREE_MODEL (list_store);
  gtk_tree_view_set_model (
    self->treeview, self->model);

  GtkTreePath * path =
    gtk_tree_path_new_from_indices (select_idx, -1);
  gtk_tree_selection_select_path (sel, path);
  gtk_tree_path_free (path);

  g_signal_connect (
    G_OBJECT (gtk_tree_view_get_selection (
      GTK_TREE_VIEW (self->treeview))),
    "changed", G_CALLBACK (on_selection_changed),
    self);
}

void
channel_send_selector_widget_setup (
  ChannelSendSelectorWidget * self)
{
  setup_treeview (self);
}

ChannelSendSelectorWidget *
channel_send_selector_widget_new (
  ChannelSendWidget * send)
{
  ChannelSendSelectorWidget * self = g_object_new (
    CHANNEL_SEND_SELECTOR_WIDGET_TYPE, NULL);
  self->send_widget = send;

  return self;
}

static void
channel_send_selector_widget_class_init (
  ChannelSendSelectorWidgetClass * klass)
{
}

static void
channel_send_selector_widget_init (
  ChannelSendSelectorWidget * self)
{
  /* create box */
  self->vbox = GTK_BOX (
    gtk_box_new (GTK_ORIENTATION_VERTICAL, 0));
  gtk_popover_set_child (
    GTK_POPOVER (self), GTK_WIDGET (self->vbox));

  /* add scroll */
  GtkWidget * scroll = gtk_scrolled_window_new ();
  gtk_scrolled_window_set_policy (
    GTK_SCROLLED_WINDOW (scroll), GTK_POLICY_NEVER,
    GTK_POLICY_AUTOMATIC);
  gtk_scrolled_window_set_propagate_natural_height (
    GTK_SCROLLED_WINDOW (scroll), true);
  gtk_scrolled_window_set_max_content_height (
    GTK_SCROLLED_WINDOW (scroll), 240);
  gtk_box_append (GTK_BOX (self->vbox), scroll);

  /* add treeview */
  self->treeview =
    GTK_TREE_VIEW (gtk_tree_view_new ());
  gtk_widget_set_visible (
    GTK_WIDGET (self->treeview), TRUE);
  gtk_scrolled_window_set_child (
    GTK_SCROLLED_WINDOW (scroll),
    GTK_WIDGET (self->treeview));

#if 0
  /* add button group */
  self->btn_box =
    GTK_BUTTON_BOX (
      gtk_button_box_new (
        GTK_ORIENTATION_HORIZONTAL));
  gtk_widget_set_visible (
    GTK_WIDGET (self->btn_box), TRUE);
  gtk_container_add (
    GTK_CONTAINER (self->vbox),
    GTK_WIDGET (self->btn_box));
  self->ok_btn =
    GTK_BUTTON (gtk_button_new_with_label (_("OK")));
  gtk_widget_set_visible (
    GTK_WIDGET (self->ok_btn), TRUE);
  gtk_container_add (
    GTK_CONTAINER (self->btn_box),
    GTK_WIDGET (self->ok_btn));
#endif

  /* init tree view */
  GtkCellRenderer *   renderer;
  GtkTreeViewColumn * column;

  /* column for icon */
  renderer = gtk_cell_renderer_pixbuf_new ();
  column = gtk_tree_view_column_new_with_attributes (
    "icon", renderer, "icon-name", 0, NULL);
  gtk_tree_view_append_column (
    self->treeview, column);

  /* column for name */
  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes (
    "name", renderer, "text", 1, NULL);
  gtk_tree_view_append_column (
    self->treeview, column);

  /* set search column */
  gtk_tree_view_set_search_column (
    self->treeview, 1);

  /* set headers invisible */
  gtk_tree_view_set_headers_visible (
    self->treeview, false);

#if 0
  g_signal_connect (
    G_OBJECT (self->ok_btn), "clicked",
    G_CALLBACK (on_ok_clicked), self);
#endif
}
