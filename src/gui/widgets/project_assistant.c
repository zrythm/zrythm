/*
 * Copyright (C) 2018-2021 Alexandros Theodotou <alex at zrythm dot org>
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

#include "gui/widgets/dialogs/create_project_dialog.h"
#include "gui/widgets/project_assistant.h"
#include "project.h"
#include "utils/arrays.h"
#include "utils/gtk.h"
#include "utils/io.h"
#include "utils/objects.h"
#include "utils/resources.h"
#include "zrythm.h"
#include "zrythm_app.h"

#include <glib/gi18n.h>

/**
 * Project file information.
 */
typedef struct ProjectInfo
{
  char *   name;
  /** Full path. */
  char *   filename;
  char *   modified;
} ProjectInfo;


/* FIXME move this to ZrythmApp so it can be
 * accessed globally */
typedef struct ProjectAssistantData
{
  GtkBuilder *   builder;
  GtkAssistant * assistant;
  ProjectInfo    project_infos[300];
  int            num_project_infos;
  ProjectInfo    template_infos[300];
  int            num_template_infos;

  /** Whether a template should be loaded. */
  bool           load_template;

  bool           zrythm_already_running;

  guint          cancel_id;

  /** The selected project/template. */
  ProjectInfo *  project_selection;
  ProjectInfo *  template_selection;
} ProjectAssistantData;

enum
{
  COLUMN_NAME,
  COLUMN_FILENAME,
  COLUMN_MODIFIED,
  COLUMN_PROJECT_INFO,
  NUM_COLUMNS
};

#if 0
static void
project_info_free_elements (
  ProjectInfo * info)
{
  if (info->name)
    g_free (info->name);
  if (info->filename)
    g_free (info->filename);
  if (info->modified)
    g_free (info->modified);
}
#endif

static void
on_projects_selection_changed (
  GtkTreeSelection *     ts,
  ProjectAssistantData * data)
{
  GtkButton * remove_btn =
    GTK_BUTTON (
      gtk_builder_get_object (
        data->builder, "remove_btn"));

  GList * selected_rows =
    gtk_tree_selection_get_selected_rows (
      ts, NULL);
  GtkTreeIter iter;
  if (selected_rows && g_list_first (selected_rows))
    {
      GtkTreePath * tp =
        (GtkTreePath *)
        g_list_first (selected_rows)->data;
      GtkTreeView * projects =
        GTK_TREE_VIEW (
          gtk_builder_get_object (
            data->builder, "projects"));
      GtkTreeModel * project_model =
        gtk_tree_view_get_model (projects);
      if (gtk_tree_model_get_iter (
            project_model, &iter, tp))
        {
          GValue value = G_VALUE_INIT;
          gtk_tree_model_get_value (
            project_model, &iter,
            COLUMN_PROJECT_INFO, &value);
          data->project_selection =
            g_value_get_pointer (&value);
          g_debug (
            "selected %s",
            data->project_selection->filename);
          if (data->project_selection)
            {
              char * last_modified =
                io_file_get_last_modified_datetime (
                  data->project_selection->filename);
              if (last_modified)
                {
                  gtk_assistant_set_page_complete (
                    data->assistant,
                    gtk_assistant_get_nth_page (
                      data->assistant, 0),
                    1);
                  g_free (last_modified);
                }
              else
                {
                  gtk_assistant_set_page_complete (
                    data->assistant,
                    gtk_assistant_get_nth_page (
                      data->assistant, 0),
                    0);
                }
            }

          g_list_free_full (
            selected_rows,
            (GDestroyNotify) gtk_tree_path_free);

          gtk_widget_set_sensitive (
            GTK_WIDGET (remove_btn), 1);
        }
    }
  else
    {
      gtk_widget_set_sensitive (
        GTK_WIDGET (remove_btn), 0);
    }
}

static void
on_templates_selection_changed (
  GtkTreeSelection *     ts,
  ProjectAssistantData * data)
{
  GtkTreeIter iter;

  GList * selected_rows =
    gtk_tree_selection_get_selected_rows (
      ts, NULL);
  if (selected_rows)
    {
      GtkTreePath * tp =
        (GtkTreePath *)g_list_first (selected_rows)->data;
      GtkTreeView * templates =
        GTK_TREE_VIEW (
          gtk_builder_get_object (
            data->builder, "templates"));
      GtkTreeModel * template_model =
        gtk_tree_view_get_model (templates);
      gtk_tree_model_get_iter (
        template_model, &iter, tp);
      GValue value = G_VALUE_INIT;
      gtk_tree_model_get_value (
        template_model, &iter,
        COLUMN_PROJECT_INFO, &value);
      data->template_selection =
        g_value_get_pointer (&value);
      gtk_assistant_set_page_complete (
        data->assistant,
        gtk_assistant_get_nth_page (
          data->assistant, 0),
        1);

      g_list_free_full (
        selected_rows,
        (GDestroyNotify) gtk_tree_path_free);
    }
}


static void
on_create_new_project_toggled (
  GtkCheckButton *       btn,
  ProjectAssistantData * data)
{
  bool active =
    gtk_check_button_get_active (btn);

  GtkBox * templates_box =
    GTK_BOX (
      gtk_builder_get_object (
        data->builder, "templates_box"));
  gtk_widget_set_visible (
    GTK_WIDGET (templates_box), active);

  GtkTreeView * projects =
    GTK_TREE_VIEW (
      gtk_builder_get_object (
        data->builder, "projects"));
  gtk_widget_set_sensitive (
    GTK_WIDGET (projects), !active);
  data->load_template = active;

  GtkButton * remove_btn =
    GTK_BUTTON (
      gtk_builder_get_object (
        data->builder, "remove_btn"));

  if (active)
    {
      gtk_assistant_set_page_complete (
        data->assistant,
        gtk_assistant_get_nth_page (
          data->assistant, 0),
        1);
      gtk_widget_set_sensitive (
        GTK_WIDGET (remove_btn), 0);
    }
  else
    {
      GtkTreeSelection * selection =
        gtk_tree_view_get_selection (
          GTK_TREE_VIEW (projects));
      if (gtk_tree_selection_count_selected_rows (
            selection) <= 0)
        {
          gtk_assistant_set_page_complete (
            data->assistant,
            gtk_assistant_get_nth_page (
              data->assistant, 0),
            0);
          gtk_widget_set_sensitive (
            GTK_WIDGET (remove_btn), false);
        }
      else
        {
          gtk_widget_set_sensitive (
            GTK_WIDGET (remove_btn), true);
        }
    }
}

static GtkTreeModel *
create_project_model (
  ProjectAssistantData * data)
{
  gint i = 0;
  GtkListStore *store;
  GtkTreeIter iter;

  /* create list store */
  store =
    gtk_list_store_new (
      NUM_COLUMNS,
      G_TYPE_STRING,
      G_TYPE_STRING,
      G_TYPE_STRING,
      G_TYPE_POINTER);

  /* add data to the list store */
  ProjectInfo * nfo;
  for (i = 0; i < data->num_project_infos; i++)
    {
      nfo = &data->project_infos[i];
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
  ProjectAssistantData * data)
{
  gint i = 0;
  GtkListStore *store;
  GtkTreeIter iter;

  /* create list store */
  store =
    gtk_list_store_new (
      NUM_COLUMNS,
      G_TYPE_STRING,
      G_TYPE_STRING,
      G_TYPE_STRING,
      G_TYPE_POINTER);

  /* add data to the list store */
  ProjectInfo * nfo;
  for (i = 0; i < data->num_template_infos; i++)
    {
      nfo = &data->template_infos[i];
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
  ProjectAssistantData * data)
{
  g_return_if_fail (data);
  g_return_if_fail (data->builder);

  /* set model to tree view */
  GtkTreeModel * model =
    create_project_model (data);

  GtkTreeView * projects =
    GTK_TREE_VIEW (
      gtk_builder_get_object (
        data->builder, "projects"));
  gtk_tree_view_set_model (projects, model);
}

static void
refresh_templates (
  ProjectAssistantData * data)
{
  /* set model to tree view */
  GtkTreeModel * model =
    create_template_model (data);
  GtkTreeView * templates =
    GTK_TREE_VIEW (
      gtk_builder_get_object (
        data->builder, "templates"));
  gtk_tree_view_set_model (
    GTK_TREE_VIEW (templates), model);

  /* select blank project */
  GtkTreeSelection *selection =
    gtk_tree_view_get_selection (
      GTK_TREE_VIEW (templates));
  GtkTreeIter iter;
  gtk_tree_model_get_iter_first (model, &iter);
  gtk_tree_selection_select_iter (
    selection, &iter);
}

static void
post_finish (
  ProjectAssistantData * data,
  bool                   quit)
{
  gtk_window_destroy (
    GTK_WINDOW (data->assistant));

  if (quit)
    {
      if (!data->zrythm_already_running)
        {
          g_application_quit (
            G_APPLICATION (zrythm_app));
        }
    }
  else
    {
      if (data->zrythm_already_running)
        {
          project_load (
            ZRYTHM->open_filename,
            ZRYTHM->opening_template);
        }
      else
        {
          /*gtk_window_close (*/
            /*GTK_WINDOW (data->assistant));*/
          g_message (
            "activating zrythm_app.load_project");
          g_action_group_activate_action (
            G_ACTION_GROUP (zrythm_app),
            "load_project", NULL);
        }
    }
}

static void
create_project_dialog_response_cb (
  GtkDialog * dialog,
  gint        response_id,
  gpointer    user_data)
{
  ProjectAssistantData * data =
    (ProjectAssistantData *) user_data;

  bool quit = false;
  if (response_id != GTK_RESPONSE_OK)
    quit = true;

  g_message (
    "%s (%s): creating project %s",
    __func__, __FILE__,
    ZRYTHM->create_project_path);

  post_finish (data, quit);
  gtk_window_destroy (GTK_WINDOW (dialog));
}

static void
on_project_remove_clicked (
  GtkButton *            btn,
  ProjectAssistantData * data)
{
  if (data->project_selection)
    {
      /* remove from gsettings */
      zrythm_remove_recent_project (
        data->project_selection->filename);

      /* remove from treeview */
      int ii;
      for (ii = 0;
           ii < data->num_project_infos; ii++)
        {
          if (&data->project_infos[ii] ==
                data->project_selection)
            {
              --data->num_project_infos;
              for (int jj = ii;
                   jj < data->num_project_infos;
                   jj++)
                {
                  data->project_infos[jj] =
                    data->project_infos[jj + 1];
                }
              break;
            }
        }
      /* refresh treeview */
      refresh_projects (data);

      /* select next project */
      GtkTreePath * path =
        gtk_tree_path_new_from_indices (
          ii, -1);
      GtkTreeView * projects =
        GTK_TREE_VIEW (
          gtk_builder_get_object (
            data->builder, "projects"));
      GtkTreeSelection *selection =
        gtk_tree_view_get_selection (projects);
      GtkTreeModel * prj_model =
        gtk_tree_view_get_model (projects);
      GtkTreeIter iter;
      int iter_set =
        gtk_tree_model_get_iter (
          prj_model, &iter, path);
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
            data->assistant,
            gtk_assistant_get_nth_page (
              data->assistant, 0),
            0);
        }
    }
}

static void
on_finish (
  ProjectAssistantData * data,
  bool                   cancel)
{
  ZRYTHM->creating_project = 1;

  if (cancel)
    {
#if 0
      gtk_window_close (
        GTK_WINDOW (data->assistant));
      g_object_unref (data->builder);
#endif
      ZRYTHM->open_filename = NULL;
    }
  /* else if we are loading a template and template
   * exists */
  else if (
    data->load_template
    && data->template_selection
    && data->template_selection->filename[0] != '-')
    {
      ZRYTHM->open_filename =
        data->template_selection->filename;
      g_message (
        "Creating project from template: %s",
        ZRYTHM->open_filename);
      ZRYTHM->opening_template = 1;
    }
  /* if we are loading a project */
  else if (!data->load_template &&
           data->project_selection)
    {
      ZRYTHM->open_filename =
        data->project_selection->filename;
      g_return_if_fail (ZRYTHM->open_filename);
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
  if (ZRYTHM->creating_project)
    {
      CreateProjectDialogWidget * dialog =
        create_project_dialog_widget_new ();
      g_signal_connect (
        G_OBJECT (dialog), "response",
        G_CALLBACK (
          create_project_dialog_response_cb), data);
      gtk_window_set_transient_for (
        GTK_WINDOW (dialog),
        GTK_WINDOW (data->assistant));
      gtk_widget_show (GTK_WIDGET (dialog));
      return;
    }

  /* at this point remove the cancel callback (it
   * gets called for some reason after the apply
   * callback) */
  if (data->cancel_id)
    {
      g_signal_handler_disconnect (
        data->assistant, data->cancel_id);
    }

  post_finish (data, false);
}

static void
on_apply (
  GtkAssistant *         assistant,
  ProjectAssistantData * data)
{
  g_debug ("on apply");
  on_finish (data, false);
}

static void
on_cancel (
  GtkAssistant *         assistant,
  ProjectAssistantData * data)
{
  g_debug ("on cancel");
  on_finish (data, true);
}

static int
on_key_release (
  GtkEventControllerKey * controller,
  guint keyval,
  guint keycode,
  GdkModifierType state,
  gpointer user_data)
{
  ProjectAssistantData * data =
    (ProjectAssistantData *) user_data;

  if (keyval == GDK_KEY_Return ||
      keyval == GDK_KEY_KP_Enter)
    {
      on_finish (data, false);
    }
  return FALSE;
}

/**
 * Runs the project assistant.
 *
 * @param zrythm_already_running If true, the logic
 *   applied is different (eg, close does not quit
 *   the program). Used when doing Ctrl+N. This
 *   should be set to false if during startup.
 */
void
project_assistant_widget_present (
  GtkWindow * parent,
  bool        show_create_new_project,
  bool        zrythm_already_running)
{
  GtkBuilder * builder =
    gtk_builder_new_from_resource (
      RESOURCES_TEMPLATE_PATH
      "/project_assistant.ui");
  GtkAssistant * assistant =
    GTK_ASSISTANT (
      gtk_builder_get_object (
        builder, "assistant"));

  ProjectAssistantData * data =
    object_new (ProjectAssistantData);
  data->assistant = assistant;
  data->builder = builder;

  GtkButton * remove_btn =
    GTK_BUTTON (
      gtk_builder_get_object (
        builder, "remove_btn"));
  g_signal_connect(
    G_OBJECT (remove_btn), "clicked",
    G_CALLBACK (on_project_remove_clicked), data);

  GtkEventController * key_controller =
    gtk_event_controller_key_new ();
  g_signal_connect (
    G_OBJECT (key_controller), "key-released",
    G_CALLBACK (on_key_release), data);
  gtk_widget_add_controller (
    GTK_WIDGET (assistant), key_controller);

  for (data->num_project_infos = 0;
       data->num_project_infos <
         ZRYTHM->num_recent_projects;
       data->num_project_infos++)
    {
      int i = data->num_project_infos;
      char * dir =
        io_get_dir (ZRYTHM->recent_projects[i]);
      char * project_name = g_path_get_basename (dir);
      data->project_infos[i].name = project_name;
      data->project_infos[i].filename =
        g_strdup (ZRYTHM->recent_projects[i]);
      data->project_infos[i].modified =
        io_file_get_last_modified_datetime (
          ZRYTHM->recent_projects[i]);
      if (data->project_infos[i].modified == NULL)
        {
          g_message (
            "Project file for <%s> not found.",
            data->project_infos[i].name);
          data->project_infos[i].modified =
            g_strdup ("<File not found>");
        }
      g_free (dir);
    }

  data->template_infos[0].name =
    g_strdup_printf (
      "%s", _("Blank project"));
  data->template_infos[0].filename =
    g_strdup ("-");
  data->template_infos[0].modified =
    g_strdup ("-");
  data->num_template_infos = 1;
  int count = 0;
  while (ZRYTHM->templates[count])
    {
      data->template_infos[
        data->num_template_infos].name =
          g_path_get_basename (
            ZRYTHM->templates[count]);
      data->template_infos[
        data->num_template_infos].filename =
          g_build_filename (
            ZRYTHM->templates[count],
            PROJECT_FILE,
            NULL);
      data->template_infos[
        data->num_template_infos].modified =
          io_file_get_last_modified_datetime (
            data->template_infos[
              data->num_template_infos].filename);
      data->num_template_infos++;
      count++;
    }

  /* set model to tree view */
  refresh_projects (data);

  /* select first project */
  GtkTreeView * projects =
    GTK_TREE_VIEW (
      gtk_builder_get_object (
        data->builder, "projects"));
  GtkTreeSelection * projects_selection =
    gtk_tree_view_get_selection (projects);
  gtk_tree_selection_set_mode (
    projects_selection, GTK_SELECTION_BROWSE);
  GtkTreeModel * prj_model =
    gtk_tree_view_get_model (projects);
  GtkTreeIter iter;
  int ret =
    gtk_tree_model_get_iter_first (
      prj_model, &iter);
  if (ret)
    gtk_tree_selection_select_iter (
      projects_selection, &iter);
  gtk_tree_view_set_search_column (
    GTK_TREE_VIEW (projects), COLUMN_NAME);

  refresh_templates (data);
  GtkTreeView * templates =
    GTK_TREE_VIEW (
      gtk_builder_get_object (
        data->builder, "templates"));
  gtk_tree_view_set_search_column (
    GTK_TREE_VIEW (templates), COLUMN_NAME);
  GtkTreeSelection * templates_selection =
    gtk_tree_view_get_selection (templates);

  /* add columns to the tree view */
  add_columns (GTK_TREE_VIEW (projects));
  add_columns (GTK_TREE_VIEW (templates));

  /* select the first template */
  g_return_if_fail (
    data->num_template_infos > 0);
  data->template_selection =
    &data->template_infos[0];

  g_signal_connect (
    G_OBJECT (assistant), "apply",
    G_CALLBACK (on_apply), data);
  data->cancel_id =
    g_signal_connect (
      G_OBJECT (assistant), "cancel",
      G_CALLBACK (on_cancel), data);

  GtkCheckButton * create_new_project =
    GTK_CHECK_BUTTON (
      gtk_builder_get_object (
        data->builder, "create_new_project"));
  g_signal_connect (
    G_OBJECT (create_new_project), "toggled",
    G_CALLBACK (on_create_new_project_toggled),
    data);

  g_signal_connect (
    G_OBJECT (projects_selection), "changed",
    G_CALLBACK (on_projects_selection_changed),
    data);
  g_signal_connect (
    G_OBJECT (templates_selection), "changed",
    G_CALLBACK (on_templates_selection_changed),
    data);

  gtk_window_set_transient_for (
    GTK_WINDOW (assistant), parent);
  gtk_window_present (GTK_WINDOW (assistant));
}

#if 0
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
}

#endif
