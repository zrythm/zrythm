/*
 * gui/widgets/start_assistant.c - Start assistant to be shown when launching Zrythm
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

#include "zrythm_app.h"
#include "gui/widgets/start_assistant.h"
#include "utils/io.h"

G_DEFINE_TYPE (StartAssistantWidget, start_assistant_widget, GTK_TYPE_ASSISTANT)

static void
start_assistant_widget_class_init (StartAssistantWidgetClass * klass)
{
  gtk_widget_class_set_template_from_resource (GTK_WIDGET_CLASS (klass),
                                               "/online/alextee/zrythm/ui/start_assistant.ui");

  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass),
                                                StartAssistantWidget,
                                                projects);
  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass),
                                                StartAssistantWidget,
                                                create_new_project);
}

static void
start_assistant_widget_init (StartAssistantWidget * self)
{
  gtk_widget_init_template (GTK_WIDGET (self));
}

static GtkTreeModel *
create_model (StartAssistantWidget * self)
{
  gint i = 0;
  GtkListStore *store;
  GtkTreeIter iter;

  /* create list store */
  store = gtk_list_store_new (NUM_COLUMNS,
                              G_TYPE_STRING,
                              G_TYPE_STRING);

  /* add data to the list store */
  for (i = 0; i < self->num_project_infos; i++)
    {
      gchar *icon_name;
      gboolean sensitive;

      gtk_list_store_append (store, &iter);
      gtk_list_store_set (store, &iter,
                          COLUMN_NAME, self->project_infos[i].name,
                          COLUMN_FILENAME, self->project_infos[i].filename,
                          -1);
    }

  return GTK_TREE_MODEL (store);
}

static void
add_columns (GtkTreeView *treeview)
{
  GtkCellRenderer *renderer;
  GtkTreeViewColumn *column;
  GtkTreeModel *model = gtk_tree_view_get_model (treeview);

  /* column for name */
  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Name",
                                                     renderer,
                                                     "text",
                                                     COLUMN_NAME,
                                                     NULL);
  gtk_tree_view_column_set_sort_column_id (column, COLUMN_NAME);
  gtk_tree_view_append_column (treeview, column);

  /* column for filename */
  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Path",
                                                     renderer,
                                                     "text",
                                                     COLUMN_FILENAME,
                                                     NULL);
  gtk_tree_view_column_set_sort_column_id (column, COLUMN_FILENAME);
  gtk_tree_view_append_column (treeview, column);
}

StartAssistantWidget *
start_assistant_widget_new (GtkWindow * parent,
                            int show_create_new_project)
{
  StartAssistantWidget * self = g_object_new (START_ASSISTANT_WIDGET_TYPE, NULL);

  for (self->num_project_infos = 0;
       self->num_project_infos < G_ZRYTHM_APP->num_recent_projects;
       self->num_project_infos++)
    {
      int i = self->num_project_infos;
      char * strip_ext = io_file_strip_ext (G_ZRYTHM_APP->recent_projects[i]);
      char * strip_path = io_file_strip_path (strip_ext);
      self->project_infos[i].name = strip_path;
      self->project_infos[i].filename = G_ZRYTHM_APP->recent_projects[i];
    }

  /* set model to tree view */
  GtkTreeModel *model = create_model (self);
  gtk_tree_view_set_model (GTK_TREE_VIEW (self->projects),
                           model);
  gtk_tree_view_set_search_column (GTK_TREE_VIEW (self->projects),
                                   COLUMN_NAME);
  g_object_unref (model);

  /* add columns to the tree view */
  add_columns (GTK_TREE_VIEW (self->projects));

  gtk_window_set_position (GTK_WINDOW (self),
                           GTK_WIN_POS_CENTER_ALWAYS);
  gtk_window_set_transient_for (GTK_WINDOW (self),
                                parent);

  return self;
}


