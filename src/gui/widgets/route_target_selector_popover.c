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

#include "audio/channel.h"
#include "audio/engine.h"
#include "audio/track.h"
#include "gui/widgets/channel.h"
#include "gui/widgets/route_target_selector.h"
#include "gui/widgets/route_target_selector_popover.h"
#include "project.h"
#include "utils/error.h"
#include "utils/flags.h"
#include "utils/gtk.h"
#include "utils/resources.h"
#include "zrythm_app.h"

#include <glib/gi18n.h>
#include <gtk/gtk.h>

G_DEFINE_TYPE (
  RouteTargetSelectorPopoverWidget,
  route_target_selector_popover_widget,
  GTK_TYPE_POPOVER)

static int
update_info_label (
  RouteTargetSelectorPopoverWidget * self,
  const char *                       label)
{
  gtk_label_set_text (self->info, label);

  return G_SOURCE_REMOVE;
}

static GtkTreeModel *
create_model_for_routes (
  RouteTargetSelectorPopoverWidget * self,
  RouteTargetSelectorType            type)
{
  GtkListStore * list_store;
  GtkTreeIter    iter;

  /* label, pointer to channel */
  list_store =
    gtk_list_store_new (2, G_TYPE_STRING, G_TYPE_POINTER);

  Track * track;
  for (int i = 0; i < TRACKLIST->num_tracks; i++)
    {
      track = TRACKLIST->tracks[i];

      if (
        type == ROUTE_TARGET_TYPE_MASTER
        && track->type == TRACK_TYPE_MASTER)
        {
          // Add a new row to the model
          gtk_list_store_append (list_store, &iter);
          gtk_list_store_set (
            list_store, &iter, 0, track->name, 1, track, -1);
        }
      else if (
        type == ROUTE_TARGET_TYPE_GROUP
        && (track->type == TRACK_TYPE_AUDIO_GROUP || track->type == TRACK_TYPE_MIDI_GROUP))
        {
          if (self->owner->channel)
            {
              Track * owner_track =
                channel_get_track (self->owner->channel);
              if (
                track != owner_track
                && track->in_signal_type
                     == owner_track->out_signal_type)
                {
                  // Add a new row to the model
                  gtk_list_store_append (list_store, &iter);
                  gtk_list_store_set (
                    list_store, &iter, 0, track->name, 1,
                    track, -1);
                }
            }
        }
      else if (
        type == ROUTE_TARGET_TYPE_INSTRUMENT
        && track->type == TRACK_TYPE_INSTRUMENT)
        {
          if (self->owner->channel)
            {
              Track * owner_track =
                channel_get_track (self->owner->channel);
              if (track != owner_track)
                {
                  // Add a new row to the model
                  gtk_list_store_append (list_store, &iter);
                  gtk_list_store_set (
                    list_store, &iter, 0, track->name, 1,
                    track, -1);
                }
            }
        }
    }

  return GTK_TREE_MODEL (list_store);
}

static GtkTreeModel *
create_model_for_types (
  RouteTargetSelectorPopoverWidget * self)
{
  GtkListStore * list_store;
  GtkTreeIter    iter;

  /* type name, type enum */
  list_store =
    gtk_list_store_new (2, G_TYPE_STRING, G_TYPE_INT);

  gtk_list_store_append (list_store, &iter);
  gtk_list_store_set (
    list_store, &iter, 0, _ ("None"), 1,
    ROUTE_TARGET_TYPE_NONE, -1);

  Track * track = channel_get_track (self->owner->channel);
  if (track->out_signal_type == TYPE_AUDIO)
    {
      gtk_list_store_append (list_store, &iter);
      gtk_list_store_set (
        list_store, &iter, 0, _ ("Master"), 1,
        ROUTE_TARGET_TYPE_MASTER, -1);
    }

  gtk_list_store_append (list_store, &iter);
  gtk_list_store_set (
    list_store, &iter, 0, _ ("Group"), 1,
    ROUTE_TARGET_TYPE_GROUP, -1);

  Track * owner_track =
    channel_get_track (self->owner->channel);
  TrackType type = owner_track->type;
  if (
    type == TRACK_TYPE_MIDI || type == TRACK_TYPE_MIDI_GROUP
    || type == TRACK_TYPE_MIDI_BUS || type == TRACK_TYPE_CHORD)
    {
      gtk_list_store_append (list_store, &iter);
      gtk_list_store_set (
        list_store, &iter, 0, _ ("Instrument"), 1,
        ROUTE_TARGET_TYPE_INSTRUMENT, -1);
    }

  return GTK_TREE_MODEL (list_store);
}

static GtkTreeView *
tree_view_create (
  RouteTargetSelectorPopoverWidget * self,
  GtkTreeModel *                     model);

static void
on_selection_changed (
  GtkTreeSelection *                 ts,
  RouteTargetSelectorPopoverWidget * self)
{
  GtkTreeView *  tv = gtk_tree_selection_get_tree_view (ts);
  GtkTreeModel * model = gtk_tree_view_get_model (tv);
  GList *        selected_rows =
    gtk_tree_selection_get_selected_rows (ts, NULL);
  if (selected_rows)
    {
      GtkTreePath * tp =
        (GtkTreePath *) g_list_first (selected_rows)->data;
      GtkTreeIter iter;
      gtk_tree_model_get_iter (model, &iter, tp);
      GValue value = G_VALUE_INIT;

      if (model == self->type_model)
        {
          gtk_tree_model_get_value (model, &iter, 1, &value);
          self->type = g_value_get_int (&value);
          self->route_model =
            create_model_for_routes (self, self->type);
          self->route_treeview =
            tree_view_create (self, self->route_model);
          z_gtk_widget_destroy_all_children (
            GTK_WIDGET (self->route_treeview_box));
          gtk_box_append (
            GTK_BOX (self->route_treeview_box),
            GTK_WIDGET (self->route_treeview));
          update_info_label (self, _ ("No output selected"));

          if (self->type == ROUTE_TARGET_TYPE_NONE)
            {
              self->new_track = NULL;
            }
        }
      else if (model == self->route_model)
        {
          gtk_tree_model_get_value (model, &iter, 1, &value);
          Track * track = g_value_get_pointer (&value);

          char * label = g_strdup_printf (
            _ ("Routing to %s"), track->name);
          update_info_label (self, label);
          g_free (label);

          self->new_track = track;
        }
    }
  g_list_free (selected_rows);
}

static GtkTreeView *
tree_view_create (
  RouteTargetSelectorPopoverWidget * self,
  GtkTreeModel *                     model)
{
  /* instantiate tree view using model */
  GtkWidget * tree_view =
    gtk_tree_view_new_with_model (GTK_TREE_MODEL (model));

  /* init tree view */
  GtkCellRenderer *   renderer;
  GtkTreeViewColumn * column;

  /* column for name */
  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes (
    "name", renderer, "text", 0, NULL);
  gtk_tree_view_append_column (
    GTK_TREE_VIEW (tree_view), column);

  /* set search column */
  gtk_tree_view_set_search_column (
    GTK_TREE_VIEW (tree_view), 0);

  /* set headers invisible */
  gtk_tree_view_set_headers_visible (
    GTK_TREE_VIEW (tree_view), 0);

  g_signal_connect (
    G_OBJECT (gtk_tree_view_get_selection (
      GTK_TREE_VIEW (tree_view))),
    "changed", G_CALLBACK (on_selection_changed), self);

  gtk_widget_set_visible (tree_view, 1);

  return GTK_TREE_VIEW (tree_view);
}

static void
on_closed (
  RouteTargetSelectorPopoverWidget * self,
  gpointer                           user_data)
{
  /*gtk_widget_destroy (GTK_WIDGET (self));*/

  Channel * ch = self->owner->channel;
  g_return_if_fail (ch);
  Track * prev_output_track = channel_get_output_track (ch);

  Track * own_track = channel_get_track (ch);
  g_return_if_fail (IS_TRACK_AND_NONNULL (own_track));
  Track * own_track_clone = track_clone (own_track, NULL);
  g_return_if_fail (IS_TRACK_AND_NONNULL (own_track_clone));
  TracklistSelections * sel = tracklist_selections_new (false);
  tracklist_selections_add_track (
    sel, own_track_clone, F_NO_PUBLISH_EVENTS);

  /* if new track selected, update routing */
  if (self->new_track)
    {
      if (self->new_track != prev_output_track)
        {
          GError * err = NULL;
          bool     ret =
            tracklist_selections_action_perform_set_direct_out (
              sel, PORT_CONNECTIONS_MGR, self->new_track, &err);
          if (!ret)
            {
              HANDLE_ERROR (
                err, "%s", _ ("Failed to change direct out"));
            }
        }
    }
  else if (
    self->type == ROUTE_TARGET_TYPE_NONE
    && prev_output_track != NULL)
    {
      GError * err = NULL;
      bool     ret =
        tracklist_selections_action_perform_set_direct_out (
          sel, PORT_CONNECTIONS_MGR, NULL, &err);
      if (!ret)
        {
          HANDLE_ERROR (
            err, "%s", _ ("Failed to remove direct out"));
        }
    }

  route_target_selector_widget_refresh (
    self->owner, self->owner->channel);
  g_object_set (self->owner, "active", 0, NULL);

  tracklist_selections_free (sel);
}

static int
select_group_func (
  GtkTreeModel *                     model,
  GtkTreePath *                      path,
  GtkTreeIter *                      iter,
  RouteTargetSelectorPopoverWidget * self)
{
  Track * iter_track;
  gtk_tree_model_get (model, iter, 1, &iter_track, -1);

  Track * output_track =
    channel_get_output_track (self->owner->channel);
  if (iter_track == output_track)
    {
      GtkTreeSelection * rts =
        gtk_tree_view_get_selection (self->route_treeview);
      gtk_tree_selection_select_iter (rts, iter);

      return TRUE;
    }

  return FALSE;
}

/**
 * Creates a new RouteTargetSelectorPopoverWidget.
 */
RouteTargetSelectorPopoverWidget *
route_target_selector_popover_widget_new (
  RouteTargetSelectorWidget * owner)
{
  RouteTargetSelectorPopoverWidget * self = g_object_new (
    ROUTE_TARGET_SELECTOR_POPOVER_WIDGET_TYPE, NULL);

  self->owner = owner;
  self->type_model = create_model_for_types (self);
  self->type_treeview =
    tree_view_create (self, self->type_model);
  gtk_box_append (
    GTK_BOX (self->type_treeview_box),
    GTK_WIDGET (self->type_treeview));

  Track * track = channel_get_track (self->owner->channel);
  if (track->out_signal_type == TYPE_AUDIO)
    {
      self->route_model = create_model_for_routes (
        self, ROUTE_TARGET_TYPE_MASTER);
    }
  else
    {
      self->route_model = create_model_for_routes (
        self, ROUTE_TARGET_TYPE_NONE);
    }
  self->route_treeview =
    tree_view_create (self, self->route_model);
  gtk_box_append (
    GTK_BOX (self->route_treeview_box),
    GTK_WIDGET (self->route_treeview));

  update_info_label (self, _ ("No output selected"));

  /* select output */
  if (self->owner->channel && self->owner->channel->has_output)
    {
      Track * output_track =
        channel_get_output_track (self->owner->channel);

      GtkTreePath * path;
      GtkTreeIter   iter;

      if (output_track->type == TRACK_TYPE_MASTER)
        {
          /* select Master group */
          path = gtk_tree_path_new_from_string ("1");
          gtk_tree_model_get_iter (
            self->type_model, &iter, path);
          GtkTreeSelection * tts =
            gtk_tree_view_get_selection (self->type_treeview);
          gtk_tree_selection_select_path (tts, path);
          gtk_tree_path_free (path);

          /* select master route */
          path = gtk_tree_path_new_from_string ("0");
          gtk_tree_model_get_iter (
            self->route_model, &iter, path);
          GtkTreeSelection * rts =
            gtk_tree_view_get_selection (self->route_treeview);
          gtk_tree_selection_select_path (rts, path);
          gtk_tree_path_free (path);
        }
      else
        {
          path = gtk_tree_path_new_from_string ("2");
          gtk_tree_model_get_iter (
            self->type_model, &iter, path);
          GtkTreeSelection * tts =
            gtk_tree_view_get_selection (self->type_treeview);
          gtk_tree_selection_select_path (tts, path);
          gtk_tree_path_free (path);

          gtk_tree_model_foreach (
            self->route_model,
            (GtkTreeModelForeachFunc) select_group_func, self);
        }
    }

  return self;
}

static void
route_target_selector_popover_widget_class_init (
  RouteTargetSelectorPopoverWidgetClass * _klass)
{
  GtkWidgetClass * klass = GTK_WIDGET_CLASS (_klass);
  resources_set_class_template (
    klass, "route_target_selector.ui");

  gtk_widget_class_bind_template_child (
    klass, RouteTargetSelectorPopoverWidget,
    type_treeview_box);
  gtk_widget_class_bind_template_child (
    klass, RouteTargetSelectorPopoverWidget,
    route_treeview_box);
  gtk_widget_class_bind_template_child (
    klass, RouteTargetSelectorPopoverWidget, info);
  gtk_widget_class_bind_template_callback (klass, on_closed);
}

static void
route_target_selector_popover_widget_init (
  RouteTargetSelectorPopoverWidget * self)
{
  gtk_widget_init_template (GTK_WIDGET (self));
}
