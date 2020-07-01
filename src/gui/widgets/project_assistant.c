/*
 * Copyright (C) 2018-2020 Alexandros Theodotou <alex at zrythm dot org>
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

#include "gui/widgets/create_project_dialog.h"
#include "gui/widgets/project_assistant.h"
#include "project.h"
#include "utils/arrays.h"
#include "utils/io.h"
#include "utils/resources.h"
#include "zrythm.h"
#include "zrythm_app.h"

#include <glib/gi18n.h>

G_DEFINE_TYPE (ProjectAssistantWidget,
               project_assistant_widget,
               GTK_TYPE_ASSISTANT)

/*static void*/
/*set_label (*/
  /*ProjectAssistantWidget * self)*/
/*{*/
  /*gtk_label_set_text (*/
    /*self->label, "TODO fill this in");*/
/*}*/

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
  if (selected_rows && g_list_first (selected_rows))
    {
      GtkTreePath * tp =
        (GtkTreePath *)
        g_list_first (selected_rows)->data;
      if (gtk_tree_model_get_iter (
            self->project_model, &iter, tp))
        {
          GValue value = G_VALUE_INIT;
          gtk_tree_model_get_value (
            self->project_model, &iter,
            COLUMN_PROJECT_INFO, &value);
          self->project_selection =
            g_value_get_pointer (&value);
          if (self->project_selection)
            {
              char * last_modified =
                io_file_get_last_modified_datetime (
                  self->project_selection->filename);
              if (last_modified)
                {
                  gtk_assistant_set_page_complete (
                    GTK_ASSISTANT (self),
                    gtk_assistant_get_nth_page (GTK_ASSISTANT (self), 0),
                    1);
                  g_free (last_modified);
                }
              else
                {
                  gtk_assistant_set_page_complete (
                    GTK_ASSISTANT (self),
                    gtk_assistant_get_nth_page (
                      GTK_ASSISTANT (self), 0),
                    0);
                }
            }

          g_list_free_full (
            selected_rows,
            (GDestroyNotify) gtk_tree_path_free);

          gtk_widget_set_sensitive (
            GTK_WIDGET (self->remove_btn), 1);
        }
    }
  else
    {
      gtk_widget_set_sensitive (
        GTK_WIDGET (self->remove_btn), 0);
    }
}

static void
on_templates_selection_changed (
  GtkTreeSelection *     ts,
  ProjectAssistantWidget * self)
{
  GtkTreeIter iter;

  GList * selected_rows =
    gtk_tree_selection_get_selected_rows (
      self->templates_selection,
      NULL);
  if (selected_rows)
    {
      GtkTreePath * tp =
        (GtkTreePath *)g_list_first (selected_rows)->data;
      gtk_tree_model_get_iter (self->template_model,
                               &iter,
                               tp);
      GValue value = G_VALUE_INIT;
      gtk_tree_model_get_value (self->template_model,
                                &iter,
                                COLUMN_PROJECT_INFO,
                                &value);
      self->template_selection =
        g_value_get_pointer (&value);
      gtk_assistant_set_page_complete (
        GTK_ASSISTANT (self),
        gtk_assistant_get_nth_page (
          GTK_ASSISTANT (self), 0),
        1);

      g_list_free_full (
        selected_rows,
        (GDestroyNotify) gtk_tree_path_free);
    }
}


static void
on_create_new_project_toggled (
  GtkToggleButton *togglebutton,
  ProjectAssistantWidget * self)
{
  int active =
    gtk_toggle_button_get_active (togglebutton);

  gtk_widget_set_visible (
    GTK_WIDGET (self->templates_box), active);
  gtk_widget_set_sensitive (
    GTK_WIDGET (self->projects), !active);
  self->load_template = active;

  if (active)
    {
      gtk_assistant_set_page_complete (
        GTK_ASSISTANT (self),
        gtk_assistant_get_nth_page (
          GTK_ASSISTANT (self), 0),
        1);
      gtk_widget_set_sensitive (
        GTK_WIDGET (self->remove_btn), 0);
    }
  else
    {
      if (gtk_tree_selection_count_selected_rows (
            self->projects_selection) <= 0)
        {
          gtk_assistant_set_page_complete (
            GTK_ASSISTANT (self),
            gtk_assistant_get_nth_page (
              GTK_ASSISTANT (self), 0),
            0);
          gtk_widget_set_sensitive (
            GTK_WIDGET (self->remove_btn), 0);
        }
      else
        {
          gtk_widget_set_sensitive (
            GTK_WIDGET (self->remove_btn), 1);
        }
    }
}

static GtkTreeModel *
create_project_model (
  ProjectAssistantWidget * self)
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

static GtkTreeModel *
create_template_model (
  ProjectAssistantWidget * self)
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
  for (i = 0; i < self->num_template_infos; i++)
    {
      nfo = &self->template_infos[i];
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
  self->project_model =
    create_project_model (self);
  gtk_tree_view_set_model (
    GTK_TREE_VIEW (self->projects),
    self->project_model);
}

static void
refresh_templates (
  ProjectAssistantWidget *self)
{
  /* set model to tree view */
  self->template_model =
    create_template_model (self);
  gtk_tree_view_set_model (
    GTK_TREE_VIEW (self->templates),
    self->template_model);

  /* select blank project */
  GtkTreeSelection *selection =
    gtk_tree_view_get_selection (
      GTK_TREE_VIEW (self->templates));
  GtkTreeIter iter;
  gtk_tree_model_get_iter_first (
    self->template_model,
    &iter);
  gtk_tree_selection_select_iter (
    selection, &iter);
}

static void
on_project_remove_clicked (
  GtkButton * btn,
  ProjectAssistantWidget * self)
{
  if (self->project_selection)
    {
      /* remove from gsettings */
      zrythm_remove_recent_project (
        self->project_selection->filename);

      /* remove from treeview */
      int ii;
      for (ii = 0;
           ii < self->num_project_infos; ii++)
        {
          if (&self->project_infos[ii] ==
                self->project_selection)
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

      /* select next project */
      GtkTreePath * path =
        gtk_tree_path_new_from_indices (
          ii, -1);
      GtkTreeSelection *selection =
        gtk_tree_view_get_selection (
          GTK_TREE_VIEW (self->projects));
      GtkTreeIter iter;
      int iter_set =
        gtk_tree_model_get_iter (
          self->project_model, &iter, path);
      if (iter_set)
        {
          gtk_tree_selection_select_iter (
            selection, &iter);
          gtk_widget_set_sensitive (
            GTK_WIDGET (btn), 1);
        }
      else
        {
          gtk_widget_set_sensitive (
            GTK_WIDGET (btn), 0);
          gtk_assistant_set_page_complete (
            GTK_ASSISTANT (self),
            gtk_assistant_get_nth_page (
              GTK_ASSISTANT (self), 0),
            0);
        }
    }
}

static int cancel = 1;

static void
on_finish (
  GtkAssistant * assistant,
  void * user_data)
{
  ProjectAssistantWidget * self =
    Z_PROJECT_ASSISTANT_WIDGET (assistant);
  ZRYTHM->creating_project = 1;
  if (user_data == &cancel) /* if cancel */
    {
      gtk_widget_destroy (GTK_WIDGET (self));
      ZRYTHM->open_filename = NULL;
    }
  /* if we are loading a template and template
   * exists */
  else if (self->load_template &&
           self->template_selection &&
           self->template_selection->
             filename[0] != '-')
    {
      ZRYTHM->open_filename =
        self->template_selection->filename;
      g_message (
        "Creating project from template: %s",
        ZRYTHM->open_filename);
      ZRYTHM->opening_template = 1;
    }
  /* if we are loading a project */
  else if (!self->load_template &&
           self->project_selection)
    {
      ZRYTHM->open_filename =
        self->project_selection->filename;
      g_message (
        "Loading project: %s",
        ZRYTHM->open_filename);
      ZRYTHM->creating_project = 0;
    }
  /* no selection, load blank project */
  else
    {
      ZRYTHM->open_filename = NULL;
      g_message (
        "Creating blank project");
    }

  /* if not loading a project, show dialog to
   * select directory and name */
  int quit = 0;
  if (ZRYTHM->creating_project)
    {
      CreateProjectDialogWidget * dialog =
        create_project_dialog_widget_new ();

      int ret =
        gtk_dialog_run (GTK_DIALOG (dialog));
      if (ret != GTK_RESPONSE_OK)
        quit = 1;
      gtk_widget_destroy (GTK_WIDGET (dialog));

      g_message (
        "%s (%s): creating project %s",
        __func__, __FILE__,
        ZRYTHM->create_project_path);
    }

  if (quit)
    {
      g_application_quit (
        G_APPLICATION (zrythm_app));
    }
  else
    {
      gtk_widget_set_visible (
        GTK_WIDGET (self), 0);
      g_action_group_activate_action (
        G_ACTION_GROUP (zrythm_app),
        "load_project", NULL);
    }
}

static int
on_key_release (
  GtkWidget * widget,
  GdkEventKey *  event,
  ProjectAssistantWidget * self)
{
  if (event->keyval == GDK_KEY_Return ||
      event->keyval == GDK_KEY_KP_Enter)
    {
      on_finish (GTK_ASSISTANT (self), NULL);
    }
  return FALSE;
}

ProjectAssistantWidget *
project_assistant_widget_new (
  GtkWindow * parent,
  int         show_create_new_project)
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
        g_strdup (ZRYTHM->recent_projects[i]);
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

  self->template_infos[0].name =
    g_strdup_printf (
      "%s", _("Blank project"));
  self->template_infos[0].filename =
    g_strdup ("-");
  self->template_infos[0].modified =
    g_strdup ("-");
  self->num_template_infos = 1;
  int count = 0;
  while (ZRYTHM->templates[count])
    {
      self->template_infos[
        self->num_template_infos].name =
          g_path_get_basename (
            ZRYTHM->templates[count]);
      self->template_infos[
        self->num_template_infos].filename =
          g_build_filename (
            ZRYTHM->templates[count],
            PROJECT_FILE,
            NULL);
      self->template_infos[
        self->num_template_infos].modified =
          io_file_get_last_modified_datetime (
            self->template_infos[
              self->num_template_infos].filename);
      self->num_template_infos++;
      count++;
    }

  /* set model to tree view */
  refresh_projects (self);

  /* select first project */
  GtkTreeSelection *selection =
    gtk_tree_view_get_selection (
      GTK_TREE_VIEW (self->projects));
  gtk_tree_selection_set_mode (
    selection, GTK_SELECTION_BROWSE);
  GtkTreeIter iter;
  int ret =
    gtk_tree_model_get_iter_first (
      self->project_model, &iter);
  if (ret)
    gtk_tree_selection_select_iter (
      selection, &iter);

  refresh_templates (self);
  gtk_tree_view_set_search_column (
    GTK_TREE_VIEW (self->projects),
    COLUMN_NAME);
  gtk_tree_view_set_search_column (
    GTK_TREE_VIEW (self->templates),
    COLUMN_NAME);

  /* add columns to the tree view */
  add_columns (GTK_TREE_VIEW (self->projects));
  add_columns (GTK_TREE_VIEW (self->templates));

  /* select the first template */
  g_return_val_if_fail (
    self->num_template_infos > 0, NULL);
  self->template_selection =
    &self->template_infos[0];

  gtk_window_set_position (
    GTK_WINDOW (self), GTK_WIN_POS_CENTER_ALWAYS);
  gtk_window_set_transient_for (
    GTK_WINDOW (self), parent);
  gtk_window_set_attached_to (
    GTK_WINDOW (self), GTK_WIDGET (parent));

  g_signal_connect (
    G_OBJECT (self), "apply",
    G_CALLBACK (on_finish), self);
  g_signal_connect (
    G_OBJECT (self), "cancel",
    G_CALLBACK (on_finish), &cancel);

  return self;
}

static void
finalize (
  ProjectAssistantWidget * self)
{
  int i;
  for (i = 0; i < self->num_project_infos; i++)
    {
      project_info_free_elements (
        &self->project_infos[i]);
    }
  for (i = 0; i < self->num_template_infos; i++)
    {
      project_info_free_elements (
        &self->template_infos[i]);
    }

  G_OBJECT_CLASS (
    project_assistant_widget_parent_class)->
      finalize (G_OBJECT (self));
}

static void
project_assistant_widget_class_init (
  ProjectAssistantWidgetClass * _klass)
{
  GtkWidgetClass * klass =
    GTK_WIDGET_CLASS (_klass);
  resources_set_class_template (
    klass, "project_assistant.ui");

#define BIND_CHILD(x) \
  gtk_widget_class_bind_template_child ( \
    klass, \
    ProjectAssistantWidget, \
    x)

  BIND_CHILD (projects);
  BIND_CHILD (projects_selection);
  BIND_CHILD (templates);
  BIND_CHILD (templates_selection);
  BIND_CHILD (templates_box);
  BIND_CHILD (remove_btn);
  BIND_CHILD (create_new_project);

#define BIND_CALLBACK(x) \
  gtk_widget_class_bind_template_callback ( \
    klass, \
    x)

  BIND_CALLBACK (on_create_new_project_toggled);
  BIND_CALLBACK (on_projects_selection_changed);
  BIND_CALLBACK (on_templates_selection_changed);

#undef BIND_CHILD
#undef BIND_CALLBACK

  GObjectClass * oklass =
    G_OBJECT_CLASS (klass);
  oklass->finalize = (GObjectFinalizeFunc) finalize;
}

static void
project_assistant_widget_init (
  ProjectAssistantWidget * self)
{
  gtk_widget_init_template (GTK_WIDGET (self));

  gtk_window_set_title (
    GTK_WINDOW (self), _("Project Assistant"));
  g_signal_connect(
    G_OBJECT (self->remove_btn), "clicked",
    G_CALLBACK (on_project_remove_clicked), self);
  g_signal_connect(
    G_OBJECT (self), "key-release-event",
    G_CALLBACK (on_key_release), self);
}
