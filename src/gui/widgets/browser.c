/*
 * gui/widgets/browser.c - The plugin, etc., browser on the right
 *
 * Copyright (C) 2018 Alexandros Theodotou
 *
 * This file is part of Zrythm
 *
 * Zrythm is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Zrythm is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Zrythm.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "settings_manager.h"
#include "zrythm_system.h"
#include "gui/widget_manager.h"

#include <gtk/gtk.h>

static GtkTreeModel *
create_model_for_types ()
{
  GtkListStore *list_store;
  GtkTreePath *path;
  GtkTreeIter iter;
  gint i;

  list_store = gtk_list_store_new (1,
                                   G_TYPE_STRING);

  for (i = 0; i < 10; i++)
    {
      gchar *some_data;

      some_data = g_strdup_printf ("test %d",
                                   i);

      // Add a new row to the model
      gtk_list_store_append (list_store, &iter);
      gtk_list_store_set (list_store, &iter,
                          0, some_data,
                          -1);

      // As the store will keep a copy of the string internally,
      // we free some_data.
      g_free (some_data);
    }

  return GTK_TREE_MODEL (list_store);
}

static void
expander_callback (GObject    *object,
                   GParamSpec *param_spec,
                   gpointer    user_data)
{
  GtkExpander *expander;

  GtkWidget * scrolled_window = GTK_WIDGET (user_data);
  expander = GTK_EXPANDER (object);

  if (gtk_expander_get_expanded (expander))
    {
      gtk_widget_set_vexpand (GTK_WIDGET (scrolled_window),
                             TRUE);
    }
  else
    {
      gtk_widget_set_vexpand (GTK_WIDGET (scrolled_window),
                             FALSE);
    }
}

/**
 * Creates a GtkTreeView using the given model,
 * and puts it in the given expander in a GtkScrolledWindow
 *
 * @return the scroll window
 */
static GtkScrolledWindow *
create_tree_view (GtkTreeModel * model,
                  GtkExpander  * expander)
{
  /* instantiate tree view using model */
  GtkWidget * tree_view = gtk_tree_view_new_with_model (
      GTK_TREE_MODEL (model));

  /* init tree view */
  GtkCellRenderer * renderer =
    gtk_cell_renderer_text_new ();
  GtkTreeViewColumn * column =
    gtk_tree_view_column_new_with_attributes ("Test",
                                              renderer,
                                              "text",
                                              0,
                                              NULL);
  gtk_tree_view_append_column ( GTK_TREE_VIEW (tree_view),
                               column);

  /* hide headers and allow multi-selection */
  gtk_tree_view_set_headers_visible (
            GTK_TREE_VIEW (tree_view),
            FALSE);
  gtk_tree_selection_set_mode (
      gtk_tree_view_get_selection (GTK_TREE_VIEW (tree_view)),
      GTK_SELECTION_MULTIPLE);

  /* add treeview to scroll window */
  GtkWidget * scrolled_window =
    gtk_scrolled_window_new (NULL, NULL);
  gtk_container_add (GTK_CONTAINER (scrolled_window),
                     GTK_WIDGET (tree_view));

  /* add scroll window to expander */
  gtk_container_add (
      GTK_CONTAINER (expander),
      scrolled_window);

  /* connect signal to expand/hide */
  g_signal_connect (expander, "notify::expanded",
                  G_CALLBACK (expander_callback),
                  scrolled_window);

  return GTK_SCROLLED_WINDOW (scrolled_window);
}

void
setup_browser ()
{
  g_message ("Setting up library browser...");

  /* get the paned window */
  GtkWidget * paned = GET_WIDGET ("gpaned-browser");

  /* set divider position */
  int divider_pos = get_int ("browser-divider-position");
  gtk_paned_set_position (GTK_PANED (paned),
                          divider_pos);

  /* get each expander */
  GtkWidget * collections = GET_WIDGET ("gexpander-collections");
  GtkWidget * types = GET_WIDGET ("gexpander-types");
  GtkWidget * categories = GET_WIDGET ("gexpander-categories");

  /* create each tree */
  create_tree_view (create_model_for_types (),
                    GTK_EXPANDER (collections));
  create_tree_view (create_model_for_types (),
                    GTK_EXPANDER (types));
  GtkScrolledWindow * scrolled_window =
    create_tree_view (create_model_for_types (),
                      GTK_EXPANDER (categories));

  /* expand category by default */
  gtk_expander_set_expanded (GTK_EXPANDER (categories),
                             TRUE);
  gtk_widget_set_vexpand (GTK_WIDGET (scrolled_window),
                          TRUE);
}

