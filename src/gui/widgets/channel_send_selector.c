/*
 * Copyright (C) 2020 Alexandros Theodotou <alex at zrythm dot org>
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
#include "plugins/plugin_identifier.h"
#include "gui/widgets/channel_send.h"
#include "gui/widgets/channel_send_selector.h"
#include "project.h"
#include "zrythm_app.h"

#include <glib/gi18n.h>

G_DEFINE_TYPE (
  ChannelSendSelectorWidget,
  channel_send_selector_widget,
  GTK_TYPE_POPOVER)

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

  int                   track_pos;
  PluginIdentifier      pl_id;
} ChannelSendTarget;

static Track *
get_track_from_target (
  ChannelSendTarget * target)
{
  if (target->type == TARGET_TYPE_NONE)
    return NULL;

  return TRACKLIST->tracks[target->track_pos];
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
    gtk_tree_selection_get_selected_rows (
      ts, NULL);
  if (!selected_rows)
    return;

  GtkTreePath * tp =
    (GtkTreePath *)
      g_list_first (selected_rows)->data;
  GtkTreeIter iter;
  gtk_tree_model_get_iter (
    self->model, &iter, tp);
  GValue value = G_VALUE_INIT;
  gtk_tree_model_get_value (
    self->model, &iter, 2, &value);
  ChannelSendTarget * target =
    g_value_get_pointer (&value);

  Track * src_track =
    channel_send_get_track (self->send_widget->send);
  Track * dest_track =
    get_track_from_target (target);
  UndoableAction * ua = NULL;
  switch (target->type)
    {
    case TARGET_TYPE_NONE:
      if (!self->send_widget->send->is_empty)
        {
          ua =
            channel_send_action_new_disconnect (
              self->send_widget->send);
          undo_manager_perform (UNDO_MANAGER, ua);
        }
      break;
    case TARGET_TYPE_TRACK:
      switch (src_track->out_signal_type)
        {
        case TYPE_EVENT:
          if (!port_identifier_is_equal (
                &self->send_widget->send->
                  dest_midi_id,
                &dest_track->processor->midi_in->id))
            {
              ua =
                channel_send_action_new_connect_midi (
                  self->send_widget->send,
                  dest_track->processor->midi_in);
              undo_manager_perform (
                UNDO_MANAGER, ua);
            }
          break;
        case TYPE_AUDIO:
          if (!port_identifier_is_equal (
                &self->send_widget->send->
                  dest_l_id,
                &dest_track->processor->stereo_in->
                  l->id))
            {
              ua =
                channel_send_action_new_connect_audio (
                  self->send_widget->send,
                  dest_track->processor->stereo_in);
              undo_manager_perform (
                UNDO_MANAGER, ua);
            }
          break;
        default:
          break;
        }
      break;
    case TARGET_TYPE_PLUGIN_SIDECHAIN:
      break;
    }

  gtk_widget_queue_draw (
    GTK_WIDGET (self->send_widget));
}

static void
setup_treeview (
  ChannelSendSelectorWidget * self)
{
  /* icon, name, pointer to data */
  GtkListStore * list_store =
    gtk_list_store_new (
      3, G_TYPE_STRING, G_TYPE_STRING,
      G_TYPE_POINTER);

  GtkTreeSelection * sel =
    gtk_tree_view_get_selection (
      GTK_TREE_VIEW (self->treeview));

  ChannelSendTarget * target =
    calloc (1, sizeof (ChannelSendTarget));
  target->type = TARGET_TYPE_NONE;
  GtkTreeIter iter;
  gtk_list_store_append (list_store, &iter);
  gtk_list_store_set (
    list_store, &iter,
    0, "edit-none",
    1, _("None"),
    2, target,
    -1);
  int select_idx = 0;
  int count = 1;

  Track * track =
    channel_send_get_track (self->send_widget->send);
  for (int i = 0; i < TRACKLIST->num_tracks; i++)
    {
      Track * target_track = TRACKLIST->tracks[i];

      g_message ("target %s", target_track->name);

      /* skip tracks with non-matching signal types */
      if (target_track == track ||
          track->out_signal_type !=
            target_track->in_signal_type ||
          !track_type_is_fx (target_track->type))
        continue;
      g_message ("adding %s", target_track->name);

      /* create target */
      target =
        calloc (1, sizeof (ChannelSendTarget));
      target->type = TARGET_TYPE_TRACK;
      target->track_pos = i;

      /* add it to list */
      gtk_list_store_append (list_store, &iter);
      gtk_list_store_set (
        list_store, &iter,
        0, "media-album-track",
        1, target_track->name,
        2, target,
        -1);

      if (!self->send_widget->send->is_empty &&
          target_track ==
            channel_send_get_target_track (
              self->send_widget->send))
        {
          select_idx = count;
        }
      count++;
    }

  /* TODO plugin sidechain inputs */

  self->model = GTK_TREE_MODEL (list_store);
  gtk_tree_view_set_model (
    self->treeview, self->model);

  GtkTreePath * path =
    gtk_tree_path_new_from_indices (select_idx, -1);
  gtk_tree_selection_select_path (
    sel, path);
  gtk_tree_path_free (path);

  g_signal_connect (
    G_OBJECT (
      gtk_tree_view_get_selection (
        GTK_TREE_VIEW (self->treeview))), "changed",
     G_CALLBACK (on_selection_changed), self);
}

ChannelSendSelectorWidget *
channel_send_selector_widget_new (
  ChannelSendWidget * send)
{
  ChannelSendSelectorWidget * self =
    g_object_new (
      CHANNEL_SEND_SELECTOR_WIDGET_TYPE,
      "relative-to", GTK_WIDGET (send),
      NULL);
  self->send_widget = send;

  setup_treeview (self);

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
  self->vbox =
    GTK_BOX (
      gtk_box_new (GTK_ORIENTATION_VERTICAL, 0));
  gtk_widget_set_visible (
    GTK_WIDGET (self->vbox), TRUE);
  gtk_container_add (
    GTK_CONTAINER (self),
    GTK_WIDGET (self->vbox));

  /* add scroll */
  GtkWidget * scroll =
    gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (
    GTK_SCROLLED_WINDOW (scroll),
    GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
  gtk_scrolled_window_set_propagate_natural_height (
    GTK_SCROLLED_WINDOW (scroll), true);
  gtk_scrolled_window_set_max_content_height (
    GTK_SCROLLED_WINDOW (scroll), 240);
  gtk_container_add (
    GTK_CONTAINER (self->vbox), scroll);
  gtk_widget_set_visible (scroll, true);

  /* add treeview */
  self->treeview =
    GTK_TREE_VIEW (gtk_tree_view_new ());
  gtk_widget_set_visible (
    GTK_WIDGET (self->treeview), TRUE);
  gtk_container_add (
    GTK_CONTAINER (scroll),
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
  GtkCellRenderer * renderer;
  GtkTreeViewColumn * column;

  /* column for icon */
  renderer = gtk_cell_renderer_pixbuf_new ();
  column =
    gtk_tree_view_column_new_with_attributes (
      "icon", renderer,
      "icon-name", 0,
      NULL);
  gtk_tree_view_append_column (
    self->treeview, column);

  /* column for name */
  renderer = gtk_cell_renderer_text_new ();
  column =
    gtk_tree_view_column_new_with_attributes (
      "name", renderer,
      "text", 1,
      NULL);
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
