/*
 * Copyright (C) 2019 Alexandros Theodotou <alex at zrythm dot org>
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

#include "audio/automatable.h"
#include "audio/automation_track.h"
#include "audio/channel_track.h"
#include "audio/engine.h"
#include "gui/backend/events.h"
#include "gui/widgets/automatable_selector_popover.h"
#include "plugins/plugin.h"
#include "project.h"
#include "utils/gtk.h"
#include "utils/resources.h"

#include <gtk/gtk.h>
#include <glib/gi18n.h>

G_DEFINE_TYPE (
  AutomatableSelectorPopoverWidget,
  automatable_selector_popover_widget,
  GTK_TYPE_POPOVER)

/**
 * Updates the info label based on the currently
 * selected Automatable.
 */
static int
update_info_label (
  AutomatableSelectorPopoverWidget * self)
{
  if (self->selected_automatable)
    {
      Automatable * a = self->selected_automatable;
      char * val_type =
        automatable_stringize_value_type (a);

      char * label =
        g_strdup_printf (
        "%s\nType: %s\nMin: %f\n"
        "Max: %f",
        a->label,
        val_type ? val_type : "Unknown",
        (double) a->minf,
        (double) a->maxf);

      gtk_label_set_text (
        self->info, label);
    }
  else
    {
      gtk_label_set_text (
        self->info, _("No control selected"));
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
  gtk_tree_model_iter_nth_child (
    self->type_model, &iter, NULL,
    self->selected_type);
  gtk_tree_selection_select_iter (sel, &iter);

  /* select current automatable */
  if (self->selected_automatable)
    {
      sel =
        gtk_tree_view_get_selection (
          GTK_TREE_VIEW (
            self->automatable_treeview));

      /* find the iter corresponding to the
       * automatable */
      int valid =
        gtk_tree_model_get_iter_first (
          self->automatable_model, &iter);
      while (valid)
        {
          Automatable * a;
          gtk_tree_model_get (
            self->automatable_model, &iter, 2,
            &a, -1);
          if (a == self->selected_automatable)
            {
              gtk_tree_selection_select_iter (
                sel, &iter);
              break;
            }
          valid =
            gtk_tree_model_iter_next (
              self->automatable_model, &iter);
        }
    }

  update_info_label (self);
}

static GtkTreeModel *
create_model_for_automatables (
  AutomatableSelectorPopoverWidget * self,
  AutomatableSelectorType            type)
{
  GtkListStore *list_store;
  GtkTreeIter iter;

  /* file name, index */
  list_store =
    gtk_list_store_new (
      3, G_TYPE_STRING, G_TYPE_STRING,
      G_TYPE_POINTER);

  g_message ("creating model for automatables");

  Track * track =
    self->owner->track;
  AutomationTrack * at;
  if (type == AS_TYPE_CHANNEL)
    {
  g_message ("creating model for automatables CHANNEL: num ats %d", track->channel->num_ats);
      for (int i = 0;
           i < track->channel->num_ats; i++)
        {
          /* get selected automation track */
          at =
            track->channel->ats[i];

          g_message ("checking %s",
            at->automatable->label);

           /*if this automation track is not already*/
           /*in a visible lane*/
          if (!at->created || !at->visible ||
              at == self->owner)
            {
              Automatable * a = at->automatable;

              // Add a new row to the model
              gtk_list_store_append (list_store, &iter);
              gtk_list_store_set (
                list_store, &iter,
                0, "z-text-x-csrc",
                1, a->label,
                2, a,
                -1);
            }
        }
    }
  else if (type > AS_TYPE_CHANNEL)
    {
      Plugin * plugin =
        track->channel->plugins[type - 1];

      if (plugin)
        {
          for (int j = 0;
               j < plugin->num_ats; j++)
            {
              /* get selected automation track */
              at =
                plugin->ats[j];

               /*if this automation track is not already*/
               /*in a visible lane*/
              if (!at->created || !at->visible ||
                  at == self->owner)
                {
                  Automatable * a =
                    at->automatable;

                  // Add a new row to the model
                  gtk_list_store_append (
                    list_store, &iter);
                  gtk_list_store_set (
                    list_store, &iter,
                    0, "z-plugins",
                    1, a->label,
                    2, a,
                    -1);
                }
            }
        }
    }

  return GTK_TREE_MODEL (list_store);
}


static GtkTreeModel *
create_model_for_types (
  AutomatableSelectorPopoverWidget * self)
{
  GtkListStore *list_store;
  GtkTreeIter iter;

  /* icon, type name, type enum */
  list_store =
    gtk_list_store_new (
      3, G_TYPE_STRING, G_TYPE_STRING,
      G_TYPE_INT);

  gtk_list_store_append (list_store, &iter);
  gtk_list_store_set (
    list_store, &iter,
    0, "z-text-x-csrc",
    1, "Channel",
    2, AS_TYPE_CHANNEL,
    -1);

  Track * track =
    self->owner->track;

  for (int i = 0; i < STRIP_SIZE; i++)
    {
      Plugin * plugin = track->channel->plugins[i];

      if (plugin)
        {
          char * label =
            g_strdup_printf (
              "[%d] %s", i, plugin->descr->name);
          gtk_list_store_append (list_store, &iter);
          gtk_list_store_set (
            list_store, &iter,
            0, "z-plugins",
            1, label,
            2, AS_TYPE_PLUGIN_0 + i,
            -1);
          g_free (label);
        }
    }

  return GTK_TREE_MODEL (list_store);
}

static GtkTreeView *
tree_view_create (
  AutomatableSelectorPopoverWidget * self,
  GtkTreeModel * model);

static void
on_selection_changed (
  GtkTreeSelection * ts,
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
    gtk_tree_selection_get_selected_rows (
      ts, NULL);
  if (selected_rows)
    {
      GtkTreePath * tp =
        (GtkTreePath *)
          g_list_first (selected_rows)->data;
      GtkTreeIter iter;
      gtk_tree_model_get_iter (model,
                               &iter,
                               tp);
      GValue value = G_VALUE_INIT;

      if (model == self->type_model)
        {
          gtk_tree_model_get_value (model,
                                    &iter,
                                    2,
                                    &value);
          self->selected_type =
            g_value_get_int (&value);
          self->automatable_model =
            create_model_for_automatables (
              self, self->selected_type);
          self->automatable_treeview =
            tree_view_create (
              self,
              self->automatable_model);
          z_gtk_container_destroy_all_children (
            GTK_CONTAINER (
              self->automatable_treeview_box));
          gtk_container_add (
            GTK_CONTAINER (
              self->automatable_treeview_box),
            GTK_WIDGET (
              self->automatable_treeview));

          self->selected_automatable = NULL;
          update_info_label (self);
        }
      else if (model ==
                 self->automatable_model)
        {
          gtk_tree_model_get_value (model,
                                    &iter,
                                    2,
                                    &value);
          Automatable * a =
            g_value_get_pointer (&value);

          self->selected_automatable = a;
          update_info_label (self);
        }
    }
}

static GtkTreeView *
tree_view_create (
  AutomatableSelectorPopoverWidget * self,
  GtkTreeModel * model)
{
  /* instantiate tree view using model */
  GtkWidget * tree_view =
    gtk_tree_view_new_with_model (
      GTK_TREE_MODEL (model));

  /* init tree view */
  GtkCellRenderer * renderer;
  GtkTreeViewColumn * column;

  /* column for icon */
  renderer = gtk_cell_renderer_pixbuf_new ();
  column =
    gtk_tree_view_column_new_with_attributes (
      "icon", renderer, "icon-name", 0, NULL);
  gtk_tree_view_append_column (
    GTK_TREE_VIEW (tree_view), column);

  /* column for name */
  renderer = gtk_cell_renderer_text_new ();
  column =
    gtk_tree_view_column_new_with_attributes (
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
    G_OBJECT (
      gtk_tree_view_get_selection (
        GTK_TREE_VIEW (tree_view))),
    "changed",
    G_CALLBACK (on_selection_changed), self);

  return GTK_TREE_VIEW (tree_view);
}

static void
on_closed (
  AutomatableSelectorPopoverWidget *self,
  gpointer                          user_data)
{
  /* if the selected automatable changed */
  if (self->selected_automatable &&
      self->owner->automatable !=
        self->selected_automatable)
    {
      /* set the previous automation track
       * invisible */
      self->owner->visible = 0;

      g_message ("selected at: %s",
        self->selected_automatable->label);

      /* swap indices */
      AutomationTracklist * atl =
        automation_track_get_automation_tracklist (
          self->owner);
      AutomationTrack * selected_at =
        automation_tracklist_get_at_from_automatable (
          atl, self->selected_automatable);
      g_return_if_fail (selected_at);
      automation_tracklist_set_at_index (
        atl, self->owner, selected_at->index, 0);

      selected_at->created = 1;
      selected_at->visible = 1;
      EVENTS_PUSH (
        ET_AUTOMATION_TRACK_ADDED, selected_at);
    }
  else
    {
      g_message (
        "same automatable selected, doing nothing");
    }
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
      AUTOMATABLE_SELECTOR_POPOVER_WIDGET_TYPE, NULL);

  self->owner = owner;

  /* set selected type */
  self->selected_type = AS_TYPE_CHANNEL;
  Automatable * a = self->owner->automatable;
  if (a->type >=
        AUTOMATABLE_TYPE_CHANNEL_FADER)
    self->selected_type = AS_TYPE_CHANNEL;
  else if (a->slot > -1)
    self->selected_type =
      AS_TYPE_PLUGIN_0 + a->slot;

  /* set selected automatable */
  self->selected_automatable = a;

  /* create model/treeview for types */
  self->type_model =
    create_model_for_types (self);
  self->type_treeview =
    tree_view_create (
      self,
      self->type_model);
  gtk_container_add (
    GTK_CONTAINER (self->type_treeview_box),
    GTK_WIDGET (self->type_treeview));

  /* create model/treeview for automatables */
  self->automatable_model =
    create_model_for_automatables (
      self, self->selected_type);
  self->automatable_treeview =
    tree_view_create (
      self,
      self->automatable_model);
  gtk_container_add (
    GTK_CONTAINER (self->automatable_treeview_box),
    GTK_WIDGET (self->automatable_treeview));

  /* select the automatable */
  self->selecting_manually = 1;
  select_automatable (self);

  return self;
}

static void
automatable_selector_popover_widget_class_init (
  AutomatableSelectorPopoverWidgetClass * _klass)
{
  GtkWidgetClass * klass = GTK_WIDGET_CLASS (_klass);
  resources_set_class_template (
    klass, "automatable_selector.ui");

  gtk_widget_class_bind_template_child (
    klass,
    AutomatableSelectorPopoverWidget,
    type_treeview_box);
  gtk_widget_class_bind_template_child (
    klass,
    AutomatableSelectorPopoverWidget,
    automatable_treeview_box);
  gtk_widget_class_bind_template_child (
    klass,
    AutomatableSelectorPopoverWidget,
    info);
  gtk_widget_class_bind_template_callback (
    klass,
    on_closed);
}

static void
automatable_selector_popover_widget_init (
  AutomatableSelectorPopoverWidget * self)
{
  gtk_widget_init_template (GTK_WIDGET (self));
}
