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
#include "zrythm_app.h"
#include "gui/widget_manager.h"
#include "plugins/plugin.h"
#include "plugins/plugin_manager.h"

#include <gtk/gtk.h>

static GtkTreeModel * plugins_tree_model;

void
on_drag_data_get (GtkWidget        *widget,
               GdkDragContext   *context,
               GtkSelectionData *data,
               guint             info,
               guint             time,
               gpointer          user_data)
{
  GtkTreeSelection * ts = (GtkTreeSelection *) user_data;
  GList * selected_rows = gtk_tree_selection_get_selected_rows (ts,
                                                                NULL);
  GtkTreePath * tp = (GtkTreePath *)g_list_first (selected_rows)->data;
  gint * indices = gtk_tree_path_get_indices (tp);
  GtkTreeRowReference *rr = gtk_tree_row_reference_new (plugins_tree_model,
                                                        tp);
  GtkTreeIter iter;
  gtk_tree_model_get_iter (plugins_tree_model,
                           &iter,
                           tp);
  GValue value = G_VALUE_INIT;
  gtk_tree_model_get_value (plugins_tree_model,
                            &iter,
                            1,
                            &value);
  Plugin * plugin = g_value_get_pointer (&value);

  gtk_selection_data_set (data,
        gdk_atom_intern_static_string ("PLUGIN"),
        32,
        (const guchar *)&plugin,
        sizeof (Plugin));
}

static GtkTreeModel *
create_model_for_types ()
{
  GtkListStore *list_store;
  GtkTreePath *path;
  GtkTreeIter iter;
  gint i;

  /* plugin name, index */
  list_store = gtk_list_store_new (2,
                                   G_TYPE_STRING, G_TYPE_INT);

  for (i = 0; i < PLUGIN_MANAGER->num_plugins; i++)
    {
      const gchar * name = PLUGIN_MANAGER->plugins[i]->descr.name;

      // Add a new row to the model
      gtk_list_store_append (list_store, &iter);
      gtk_list_store_set (list_store, &iter,
                          0, name,
                          -1);
      gtk_list_store_set (list_store, &iter,
                          1, i,
                          -1);
    }

  return GTK_TREE_MODEL (list_store);
}

static GtkTreeModel *
create_model_for_categories ()
{
  GtkListStore *list_store;
  GtkTreePath *path;
  GtkTreeIter iter;
  gint i;

  /* plugin name, index */
  list_store = gtk_list_store_new (1,
                                   G_TYPE_STRING);

  for (i = 0; i < PLUGIN_MANAGER->num_plugin_categories; i++)
    {
      const gchar * name = PLUGIN_MANAGER->plugin_categories[i];

      // Add a new row to the model
      gtk_list_store_append (list_store, &iter);
      gtk_list_store_set (list_store, &iter,
                          0, name,
                          -1);
    }

  return GTK_TREE_MODEL (list_store);
}

static GtkTreeModel *
create_model_for_plugins ()
{
  GtkListStore *list_store;
  GtkTreePath *path;
  GtkTreeIter iter;
  gint i;

  /* plugin name, index */
  list_store = gtk_list_store_new (2,
                                   G_TYPE_STRING, G_TYPE_POINTER);

  for (i = 0; i < PLUGIN_MANAGER->num_plugins; i++)
    {
      const gchar * name = PLUGIN_MANAGER->plugins[i]->descr.name;

      // Add a new row to the model
      gtk_list_store_append (list_store, &iter);
      gtk_list_store_set (list_store, &iter,
                          0, name,
                          -1);
      gtk_list_store_set (list_store, &iter,
                          1, PLUGIN_MANAGER->plugins[i],
                          -1);
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

static GtkWidget *
tree_view_create (GtkTreeModel * model,
                  int          allow_multi,
                  int          dnd)
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

  if (allow_multi)
    gtk_tree_selection_set_mode (
        gtk_tree_view_get_selection (GTK_TREE_VIEW (tree_view)),
        GTK_SELECTION_MULTIPLE);

  if (dnd)
    {
      gtk_tree_view_enable_model_drag_source (GTK_TREE_VIEW (tree_view),
                                    GDK_BUTTON1_MASK,
                                    WIDGET_MANAGER->entries,
                                    WIDGET_MANAGER->num_entries,
                                    GDK_ACTION_COPY);
      g_signal_connect (GTK_WIDGET (tree_view),
                        "drag-data-get",
                        G_CALLBACK (on_drag_data_get),
                        gtk_tree_view_get_selection (GTK_TREE_VIEW (tree_view)));
    }


  return tree_view;
}

static GtkWidget *
add_scroll_window (GtkTreeView * tree_view)
{
  /* add treeview to scroll window */
  GtkWidget * scrolled_window =
    gtk_scrolled_window_new (NULL, NULL);
  gtk_container_add (GTK_CONTAINER (scrolled_window),
                     GTK_WIDGET (tree_view));
  return scrolled_window;
}


/**
 * Creates a GtkTreeView using the given model,
 * and puts it in the given expander in a GtkScrolledWindow
 *
 * @return the scroll window
 * TODO FIXME
 */
static GtkScrolledWindow *
create_tree_view_add_to_expander (GtkTreeModel * model,
                  GtkExpander  * expander)
{
  GtkWidget * scrolled_window = add_scroll_window
                  (GTK_TREE_VIEW (tree_view_create (model,
                                                    1,0)));

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
setup_browser (GtkWidget * paned,
               GtkWidget * collections,
               GtkWidget * types,
               GtkWidget * categories,
               GtkWidget * plugins_box)
{
  g_message ("Setting up library browser...");

  /* set divider position */
  int divider_pos = get_int ("browser-divider-position");
  gtk_paned_set_position (GTK_PANED (paned),
                          divider_pos);

  /* create each tree */
  create_tree_view_add_to_expander (create_model_for_types (),
                    GTK_EXPANDER (collections));
  create_tree_view_add_to_expander (create_model_for_types (),
                    GTK_EXPANDER (types));
  GtkScrolledWindow * scrolled_window =
  create_tree_view_add_to_expander (create_model_for_categories (),
                      GTK_EXPANDER (categories));

  /* expand category by default */
  gtk_expander_set_expanded (GTK_EXPANDER (categories),
                             TRUE);
  gtk_widget_set_vexpand (GTK_WIDGET (scrolled_window),
                          TRUE);

  /* populate plugins */
  plugins_tree_model = create_model_for_plugins ();
  GtkWidget * plugin_scroll_window = add_scroll_window (GTK_TREE_VIEW (tree_view_create (plugins_tree_model, 0, 1)));
  gtk_box_pack_start (GTK_BOX (plugins_box),
                      plugin_scroll_window,
                      1, 1, 0);
}

