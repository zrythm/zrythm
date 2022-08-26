// SPDX-FileCopyrightText: © 2018-2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "audio/engine.h"
#include "gui/backend/event.h"
#include "gui/backend/event_manager.h"
#include "gui/widgets/center_dock.h"
#include "gui/widgets/expander_box.h"
#include "gui/widgets/main_window.h"
#include "gui/widgets/right_dock_edge.h"
#include "gui/widgets/track_visibility_tree.h"
#include "plugins/plugin.h"
#include "plugins/plugin_manager.h"
#include "project.h"
#include "settings/settings.h"
#include "utils/gtk.h"
#include "utils/resources.h"
#include "utils/string.h"
#include "utils/ui.h"
#include "zrythm_app.h"

#include <glib/gi18n.h>
#include <gtk/gtk.h>

G_DEFINE_TYPE (
  TrackVisibilityTreeWidget,
  track_visibility_tree_widget,
  GTK_TYPE_BOX)

enum
{
  COL_CHECKBOX,
  COL_NAME,
  COL_TRACK,
  NUM_COLS
};

static void
visibility_toggled (
  GtkCellRendererToggle *     cell,
  gchar *                     path_str,
  TrackVisibilityTreeWidget * self)
{
  GtkTreeModel * model = self->tree_model;
  GtkTreeIter    iter;
  GtkTreePath *  path =
    gtk_tree_path_new_from_string (path_str);
  gboolean fixed;

  /* get toggled iter */
  gtk_tree_model_get_iter (model, &iter, path);
  gtk_tree_model_get (model, &iter, COL_CHECKBOX, &fixed, -1);

  /* get track */
  Track * track;
  gtk_tree_model_get (model, &iter, COL_TRACK, &track, -1);

  /* do something with the value */
  fixed ^= 1;
  track->visible = fixed;

  /* set new value */
  gtk_list_store_set (
    GTK_LIST_STORE (model), &iter, COL_CHECKBOX, fixed, -1);

  /* clean up */
  gtk_tree_path_free (path);

  EVENTS_PUSH (ET_TRACK_VISIBILITY_CHANGED, track);
}

static GtkTreeModel *
create_model (void)
{
  gint           i = 0;
  GtkListStore * store;
  GtkTreeIter    iter;

  /* create list store */
  store = gtk_list_store_new (
    NUM_COLS, G_TYPE_BOOLEAN, G_TYPE_STRING, G_TYPE_POINTER);

  /* add data to the list store */
  Track * track;
  for (i = 0; i < TRACKLIST->num_tracks; i++)
    {
      track = TRACKLIST->tracks[i];

      gtk_list_store_append (store, &iter);
      gtk_list_store_set (
        store, &iter, COL_CHECKBOX, track->visible, COL_NAME,
        track->name, COL_TRACK, track, -1);
    }

  return GTK_TREE_MODEL (store);
}

static void
tree_view_setup (
  TrackVisibilityTreeWidget * self,
  GtkTreeView *               tree_view)
{
  /* init tree view */
  GtkCellRenderer *   renderer;
  GtkTreeViewColumn * column;

  /* column for checkbox */
  renderer = gtk_cell_renderer_toggle_new ();
  g_signal_connect (
    renderer, "toggled", G_CALLBACK (visibility_toggled),
    self);
  column = gtk_tree_view_column_new_with_attributes (
    _ ("Visible"), renderer, "active", COL_CHECKBOX, NULL);

  /* set this column to a fixed sizing (of 50
   * pixels) */
  gtk_tree_view_column_set_sizing (
    GTK_TREE_VIEW_COLUMN (column), GTK_TREE_VIEW_COLUMN_FIXED);
  gtk_tree_view_column_set_fixed_width (
    GTK_TREE_VIEW_COLUMN (column), 50);
  gtk_tree_view_append_column (
    GTK_TREE_VIEW (tree_view), column);

  /* column for name */
  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes (
    _ ("Track Name"), renderer, "text", COL_NAME, NULL);
  gtk_tree_view_append_column (
    GTK_TREE_VIEW (tree_view), column);
}

/**
 * Refreshes the tree model.
 */
void
track_visibility_tree_widget_refresh (
  TrackVisibilityTreeWidget * self)
{
  GtkTreeModel * model = self->tree_model;
  self->tree_model = create_model ();
  gtk_tree_view_set_model (self->tree, self->tree_model);

  if (model)
    g_object_unref (model);
}

TrackVisibilityTreeWidget *
track_visibility_tree_widget_new ()
{
  TrackVisibilityTreeWidget * self =
    g_object_new (TRACK_VISIBILITY_TREE_WIDGET_TYPE, NULL);

  /* setup tree */
  tree_view_setup (self, self->tree);

  return self;
}

static void
track_visibility_tree_widget_class_init (
  TrackVisibilityTreeWidgetClass * _klass)
{
}

static void
track_visibility_tree_widget_init (
  TrackVisibilityTreeWidget * self)
{
  self->scroll =
    GTK_SCROLLED_WINDOW (gtk_scrolled_window_new ());
  gtk_box_append (GTK_BOX (self), GTK_WIDGET (self->scroll));
  gtk_widget_set_hexpand (GTK_WIDGET (self->scroll), true);
  self->tree = GTK_TREE_VIEW (gtk_tree_view_new ());
  gtk_scrolled_window_set_child (
    self->scroll, GTK_WIDGET (self->tree));
}
