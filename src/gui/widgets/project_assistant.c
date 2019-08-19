/*
 * Copyright (C) 2018-2019 Alexandros Theodotou <alex at zrythm dot org>
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

#include "zrythm.h"
#include "gui/widgets/project_assistant.h"
#include "utils/arrays.h"
#include "utils/io.h"
#include "utils/resources.h"

#include <glib/gi18n.h>

G_DEFINE_TYPE (ProjectAssistantWidget,
               project_assistant_widget,
               GTK_TYPE_ASSISTANT)

static void
set_label (
  ProjectAssistantWidget * self)
{
  gtk_label_set_text (
    self->label, "TODO fill this in");
}

static void
on_projects_selection_changed (
  GtkTreeSelection *     ts,
  ProjectAssistantWidget * self)
{
  GtkTreeIter iter;

  GList * selected_rows =
    gtk_tree_selection_get_selected_rows (
      self->projects_selection,
      NULL);
  if (selected_rows)
    {
      GtkTreePath * tp =
        (GtkTreePath *)g_list_first (selected_rows)->data;
      gtk_tree_model_get_iter (self->model,
                               &iter,
                               tp);
      GValue value = G_VALUE_INIT;
      gtk_tree_model_get_value (self->model,
                                &iter,
                                COLUMN_PROJECT_INFO,
                                &value);
      self->selection = g_value_get_pointer (&value);
      if (self->selection)
        {
          gtk_assistant_set_page_complete (
            GTK_ASSISTANT (self),
            gtk_assistant_get_nth_page (GTK_ASSISTANT (self), 0),
            1);
          gtk_assistant_set_page_complete (
            GTK_ASSISTANT (self),
            gtk_assistant_get_nth_page (GTK_ASSISTANT (self), 1),
            1);
          set_label (self);
        }
    }
}

void
on_create_new_project_toggled (
  GtkToggleButton *togglebutton,
  ProjectAssistantWidget * self)
{
  if (gtk_toggle_button_get_active (togglebutton))
    {
      /*gtk_tree_selection_unselect_all (self->projects_selection);*/
      gtk_widget_set_sensitive (
        GTK_WIDGET (self->projects), 0);
      gtk_assistant_set_page_complete (
        GTK_ASSISTANT (self),
        gtk_assistant_get_nth_page (
          GTK_ASSISTANT (self), 0),
        1);
      gtk_assistant_set_page_complete (
        GTK_ASSISTANT (self),
        gtk_assistant_get_nth_page (
          GTK_ASSISTANT (self), 1),
        1);
        self->selection = NULL;
    }
  else
    {
      gtk_widget_set_sensitive (GTK_WIDGET (self->projects), 1);
    }
}


static GtkTreeModel *
create_model (ProjectAssistantWidget * self)
{
  gint i = 0;
  GtkListStore *store;
  GtkTreeIter iter;

  /* create list store */
  store = gtk_list_store_new (NUM_COLUMNS,
                              G_TYPE_STRING,
                              G_TYPE_STRING,
                              G_TYPE_STRING,
                              G_TYPE_POINTER);

  /* add data to the list store */
  ProjectInfo * nfo;
  for (i = 0; i < self->num_project_infos; i++)
    {
      nfo = &self->project_infos[i];
      gtk_list_store_append (store, &iter);
      gtk_list_store_set (
        store, &iter,
        COLUMN_NAME, nfo->name,
        COLUMN_FILENAME, nfo->filename,
        COLUMN_MODIFIED, nfo->modified,
        COLUMN_PROJECT_INFO, nfo,
        -1);
    }

  return GTK_TREE_MODEL (store);
}

static void
add_columns (GtkTreeView *treeview)
{
  GtkCellRenderer *renderer;
  GtkTreeViewColumn *column;
  /*GtkTreeModel *model = gtk_tree_view_get_model (treeview);*/

  /* column for name */
  renderer = gtk_cell_renderer_text_new ();
  column =
    gtk_tree_view_column_new_with_attributes (
      _("Name"), renderer, "text", COLUMN_NAME,
      NULL);
  gtk_tree_view_column_set_sort_column_id (
    column, COLUMN_NAME);
  gtk_tree_view_append_column (treeview, column);

  /* column for filename */
  renderer = gtk_cell_renderer_text_new ();
  column =
    gtk_tree_view_column_new_with_attributes (
      _("Path"), renderer, "text",
      COLUMN_FILENAME, NULL);
  gtk_tree_view_column_set_sort_column_id (
    column, COLUMN_FILENAME);
  gtk_tree_view_append_column (treeview, column);

  /* column for modified */
  renderer = gtk_cell_renderer_text_new ();
  column =
    gtk_tree_view_column_new_with_attributes (
      _("Last Modified"), renderer, "text",
      COLUMN_MODIFIED, NULL);
  gtk_tree_view_column_set_sort_column_id (
    column, COLUMN_MODIFIED);
  gtk_tree_view_append_column (treeview, column);
}

static void
refresh_projects (
  ProjectAssistantWidget *self)
{
  /* set model to tree view */
  self->model = create_model (self);
  gtk_tree_view_set_model (
    GTK_TREE_VIEW (self->projects),
    self->model);
}

static void
on_project_remove_clicked (
  GtkButton * btn,
  ProjectAssistantWidget * self)
{
  if (self->selection)
    {
      /* remove from gsettings */
      zrythm_remove_recent_project (
        self->selection->filename);

      /* remove from treeview */
      for (int ii = 0;
           ii < self->num_project_infos; ii++)
        {
          if (&self->project_infos[ii] ==
                self->selection)
            {
              --self->num_project_infos;
              for (int jj = ii;
                   jj < self->num_project_infos;
                   jj++)
                {
                  self->project_infos[jj] =
                    self->project_infos[jj + 1];
                }
              break;
            }
        }


        /* refresh treeview */
        refresh_projects (self);
    }
}


ProjectAssistantWidget *
project_assistant_widget_new (GtkWindow * parent,
                            int show_create_new_project)
{
  ProjectAssistantWidget * self =
    g_object_new (
      PROJECT_ASSISTANT_WIDGET_TYPE,
      "modal", 1,
      "urgency-hint", 1,
      NULL);

  for (self->num_project_infos = 0;
       self->num_project_infos <
         ZRYTHM->num_recent_projects;
       self->num_project_infos++)
    {
      int i = self->num_project_infos;
      char * dir =
        io_get_dir (ZRYTHM->recent_projects[i]);
      char * project_name = g_path_get_basename (dir);
      self->project_infos[i].name = project_name;
      self->project_infos[i].filename =
        ZRYTHM->recent_projects[i];
      self->project_infos[i].modified =
        io_file_get_last_modified_datetime (
          ZRYTHM->recent_projects[i]);
      if (self->project_infos[i].modified == NULL)
        {
          g_message (
            "Project file for <%s> not found.",
            self->project_infos[i].name);
          self->project_infos[i].modified =
            g_strdup ("<File not found>");
        }
      g_free (dir);
    }

  /* set model to tree view */
  refresh_projects (self);
  gtk_tree_view_set_search_column (
    GTK_TREE_VIEW (self->projects),
    COLUMN_NAME);

  /* add columns to the tree view */
  add_columns (GTK_TREE_VIEW (self->projects));

  gtk_window_set_position (
    GTK_WINDOW (self), GTK_WIN_POS_CENTER_ALWAYS);
  gtk_window_set_transient_for (
    GTK_WINDOW (self), parent);
  gtk_window_set_attached_to (
    GTK_WINDOW (self), GTK_WIDGET (parent));
  gtk_window_set_keep_above (
    GTK_WINDOW (self), 1);

  return self;
}

static void
project_assistant_widget_class_init (
  ProjectAssistantWidgetClass * _klass)
{
  GtkWidgetClass * klass =
    GTK_WIDGET_CLASS (_klass);
  resources_set_class_template (
    klass, "project_assistant.ui");

  gtk_widget_class_bind_template_child (
    klass,
    ProjectAssistantWidget,
    projects);
  gtk_widget_class_bind_template_child (
    klass,
    ProjectAssistantWidget,
    projects_selection);
  gtk_widget_class_bind_template_child (
    klass,
    ProjectAssistantWidget,
    remove_btn);
  gtk_widget_class_bind_template_child (
    klass,
    ProjectAssistantWidget,
    create_new_project);
  gtk_widget_class_bind_template_child (
    klass,
    ProjectAssistantWidget,
    label);
  gtk_widget_class_bind_template_callback (
    klass,
    on_create_new_project_toggled);
  gtk_widget_class_bind_template_callback (
    klass,
    on_projects_selection_changed);
}

static void
project_assistant_widget_init (
  ProjectAssistantWidget * self)
{
  gtk_widget_init_template (GTK_WIDGET (self));

  g_signal_connect(
    G_OBJECT (self->remove_btn), "clicked",
    G_CALLBACK (on_project_remove_clicked), self);

}
