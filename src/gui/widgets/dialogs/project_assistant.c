// SPDX-FileCopyrightText: © 2018-2023 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "gui/backend/wrapped_object_with_change_signal.h"
#include "gui/widgets/dialogs/create_project_dialog.h"
#include "gui/widgets/dialogs/project_assistant.h"
#include "gui/widgets/item_factory.h"
#include "project.h"
#include "utils/arrays.h"
#include "utils/datetime.h"
#include "utils/error.h"
#include "utils/flags.h"
#include "utils/gtk.h"
#include "utils/io.h"
#include "utils/objects.h"
#include "utils/resources.h"
#include "utils/string.h"
#include "utils/ui.h"
#include "zrythm.h"
#include "zrythm_app.h"

#include <glib/gi18n.h>

#define FILE_NOT_FOUND_STR _ ("<File not found>")

G_DEFINE_TYPE (ProjectAssistantWidget, project_assistant_widget, ADW_TYPE_WINDOW)

static ProjectInfo *
project_info_new (const char * name, const char * filename)
{
  ProjectInfo * self = object_new (ProjectInfo);
  self->name = g_strdup (name);
  if (filename)
    {
      self->filename = g_strdup (filename);
      self->modified = io_file_get_last_modified_datetime (filename);
      if (self->modified == -1)
        {
          self->modified_str = g_strdup (FILE_NOT_FOUND_STR);
          self->modified = G_MAXINT64;
        }
      else
        {
          self->modified_str = datetime_epoch_to_str (self->modified, NULL);
        }
      g_return_val_if_fail (self->modified_str, NULL);
    }
  else
    {
      self->filename = g_strdup ("-");
      self->modified = 0;
      self->modified_str = g_strdup ("-");
    }

  return self;
}

static void
project_info_free (ProjectInfo * self)
{
  g_free_and_null (self->name);
  g_free_and_null (self->filename);
  g_free_and_null (self->modified_str);

  object_zero_and_free (self);
}

static void
project_info_destroy_func (void * data)
{
  project_info_free ((ProjectInfo *) data);
}

static ProjectInfo *
get_selected_project (ProjectAssistantWidget * self)
{
  GtkSingleSelection * sel = GTK_SINGLE_SELECTION (
    gtk_column_view_get_model (self->recent_projects_column_view));
  GObject * gobj = gtk_single_selection_get_selected_item (sel);
  if (!gobj)
    return NULL;

  WrappedObjectWithChangeSignal * wrapped_obj =
    Z_WRAPPED_OBJECT_WITH_CHANGE_SIGNAL (gobj);
  ProjectInfo * nfo = (ProjectInfo *) wrapped_obj->obj;

  return nfo;
}

static ProjectInfo *
get_selected_template (ProjectAssistantWidget * self)
{
  GtkSingleSelection * sel = GTK_SINGLE_SELECTION (
    gtk_column_view_get_model (self->templates_column_view));
  GObject * gobj = gtk_single_selection_get_selected_item (sel);
  if (!gobj)
    return NULL;

  WrappedObjectWithChangeSignal * wrapped_obj =
    Z_WRAPPED_OBJECT_WITH_CHANGE_SIGNAL (gobj);
  ProjectInfo * nfo = (ProjectInfo *) wrapped_obj->obj;

  return nfo;
}

static void
on_visible_child_name_changed (
  GObject *    gobject,
  GParamSpec * pspec,
  gpointer     user_data)
{
  ProjectAssistantWidget * self = (ProjectAssistantWidget *) user_data;

  const char * child_name = adw_view_stack_get_visible_child_name (self->stack);

  if (string_is_equal (child_name, "open-recent"))
    {
      gtk_button_set_label (self->ok_btn, _ ("_Open Selected"));
      gtk_widget_set_visible (GTK_WIDGET (self->open_from_path_btn), true);
    }
  else if (string_is_equal (child_name, "create-new"))
    {
      gtk_button_set_label (self->ok_btn, _ ("_Create"));
      gtk_widget_set_visible (GTK_WIDGET (self->open_from_path_btn), false);
    }
}

static void
on_project_selection_changed (
  GtkSelectionModel *      sel_model,
  guint                    position,
  guint                    n_items,
  ProjectAssistantWidget * self)
{
  GtkSingleSelection * sel = GTK_SINGLE_SELECTION (sel_model);

  WrappedObjectWithChangeSignal * wrapped_obj =
    Z_WRAPPED_OBJECT_WITH_CHANGE_SIGNAL (
      gtk_single_selection_get_selected_item (sel));
  ProjectInfo * nfo = (ProjectInfo *) wrapped_obj->obj;
  g_debug (
    "selected %s (modified %" G_GINT64_FORMAT ")", nfo->filename, nfo->modified);

  gtk_widget_set_sensitive (
    GTK_WIDGET (self->ok_btn),
    !string_is_equal (nfo->modified_str, FILE_NOT_FOUND_STR));
}

static void
on_template_selection_changed (
  GtkSelectionModel *      sel_model,
  guint                    position,
  guint                    n_items,
  ProjectAssistantWidget * self)
{
  GtkSingleSelection * sel = GTK_SINGLE_SELECTION (sel_model);

  WrappedObjectWithChangeSignal * wrapped_obj =
    Z_WRAPPED_OBJECT_WITH_CHANGE_SIGNAL (
      gtk_single_selection_get_selected_item (sel));
  ProjectInfo * nfo = (ProjectInfo *) wrapped_obj->obj;
  g_debug ("selected %s", nfo->filename);
}

static void
refresh_projects (ProjectAssistantWidget * self)
{
  GListStore * store =
    z_gtk_column_view_get_list_store (self->recent_projects_column_view);

  GPtrArray * wrapped_projects = g_ptr_array_new ();
  for (size_t i = 0; i < self->project_infos_arr->len; i++)
    {
      ProjectInfo * nfo =
        (ProjectInfo *) g_ptr_array_index (self->project_infos_arr, i);
      WrappedObjectWithChangeSignal * wrapped_nfo =
        wrapped_object_with_change_signal_new (
          nfo, WRAPPED_OBJECT_TYPE_PROJECT_INFO);
      g_ptr_array_add (wrapped_projects, wrapped_nfo);
    }

  z_gtk_list_store_splice (store, wrapped_projects);
}

static void
refresh_templates (ProjectAssistantWidget * self)
{
  GListStore * store =
    z_gtk_column_view_get_list_store (self->templates_column_view);

  GPtrArray * wrapped_projects = g_ptr_array_new ();
  for (size_t i = 0; i < self->templates_arr->len; i++)
    {
      ProjectInfo * nfo =
        (ProjectInfo *) g_ptr_array_index (self->templates_arr, i);
      WrappedObjectWithChangeSignal * wrapped_nfo =
        wrapped_object_with_change_signal_new (
          nfo, WRAPPED_OBJECT_TYPE_PROJECT_INFO);
      g_ptr_array_add (wrapped_projects, wrapped_nfo);
    }

  z_gtk_list_store_splice (store, wrapped_projects);
}

static void
post_finish (ProjectAssistantWidget * self, bool zrythm_already_running, bool quit)
{
  if (self)
    gtk_window_destroy (GTK_WINDOW (self));

  if (quit)
    {
      if (!zrythm_already_running)
        {
          g_application_quit (G_APPLICATION (zrythm_app));
        }
    }
  else
    {
      if (zrythm_already_running)
        {
          GError * err = NULL;
          bool     success = project_load (
            ZRYTHM->open_filename, ZRYTHM->opening_template, &err);
          if (!success)
            {
              HANDLE_ERROR (err, "%s", _ ("Failed to load project"));
            }
        }
      else
        {
          /*gtk_window_close (*/
          /*GTK_WINDOW (data->assistant));*/
          g_message ("activating zrythm_app.load_project");
          g_action_group_activate_action (
            G_ACTION_GROUP (zrythm_app), "load_project", NULL);
        }
    }
}

static void
create_project_dialog_response_cb (
  GtkDialog * dialog,
  gint        response_id,
  gpointer    user_data)
{
  bool zrythm_already_running = GPOINTER_TO_INT (user_data);

  bool quit = false;
  if (response_id != GTK_RESPONSE_OK)
    quit = true;

  g_message (
    "%s (%s): creating project %s", __func__, __FILE__,
    ZRYTHM->create_project_path);

  GtkWindow * parent = gtk_window_get_transient_for (GTK_WINDOW (dialog));
  post_finish (
    Z_IS_PROJECT_ASSISTANT_WIDGET (parent)
      ? Z_PROJECT_ASSISTANT_WIDGET (parent)
      : NULL,
    zrythm_already_running, quit);
  gtk_window_destroy (GTK_WINDOW (dialog));
}

static void
on_key_release (
  GtkEventControllerKey * controller,
  guint                   keyval,
  guint                   keycode,
  GdkModifierType         state,
  gpointer                user_data)
{
  ProjectAssistantWidget * self = (ProjectAssistantWidget *) user_data;

  /*g_debug ("key release %u", keyval);*/

  if (keyval == GDK_KEY_Delete)
    {
      g_debug ("delete");

      ProjectInfo * nfo = get_selected_project (self);
      if (nfo)
        {
          /* remove from gsettings */
          zrythm_remove_recent_project (nfo->filename);

          /* remove from ptr array */
          g_ptr_array_remove (self->project_infos_arr, nfo);

          /* refresh column view */
          refresh_projects (self);
        }
    }
}

/**
 * Runs the project assistant.
 *
 * @param zrythm_already_running If true, the logic applied is
 *   different (eg, close does not quit the program). Used when
 *   doing Ctrl+N. This should be set to false if during
 *   startup.
 * @param template Template to create a new project from, if
 *   non-NULL.
 */
void
project_assistant_widget_present (
  GtkWindow * parent,
  bool        zrythm_already_running,
  const char * template)
{
  ProjectAssistantWidget * self =
    g_object_new (PROJECT_ASSISTANT_WIDGET_TYPE, NULL);

  self->zrythm_already_running = zrythm_already_running;

  self->parent = parent;
  self->template = g_strdup (template);

  if (template)
    {
      ZRYTHM->creating_project = true;
      ZRYTHM->open_filename = g_build_filename (template, PROJECT_FILE, NULL);
      ZRYTHM->opening_template = true;
      g_message ("Creating project from template: %s", ZRYTHM->open_filename);

      CreateProjectDialogWidget * create_prj_dialog =
        create_project_dialog_widget_new ();
      g_signal_connect (
        G_OBJECT (create_prj_dialog), "response",
        G_CALLBACK (create_project_dialog_response_cb),
        GINT_TO_POINTER (self->zrythm_already_running));
      gtk_window_set_transient_for (
        GTK_WINDOW (create_prj_dialog), GTK_WINDOW (self->parent));
      gtk_window_present (GTK_WINDOW (create_prj_dialog));
    }
  else
    {
      gtk_window_set_transient_for (GTK_WINDOW (self), parent);
      gtk_window_present (GTK_WINDOW (self));
    }
}

static void
run_create_project_dialog (
  ProjectAssistantWidget * self,
  GtkWindow *              transient_parent)
{
  CreateProjectDialogWidget * create_prj_dialog =
    create_project_dialog_widget_new ();
  g_signal_connect (
    G_OBJECT (create_prj_dialog), "response",
    G_CALLBACK (create_project_dialog_response_cb),
    GINT_TO_POINTER (self->zrythm_already_running));

  gtk_window_set_transient_for (
    GTK_WINDOW (create_prj_dialog), GTK_WINDOW (transient_parent));

  gtk_window_present (GTK_WINDOW (create_prj_dialog));
}

static bool
on_close_request (GtkWindow * window, ProjectAssistantWidget * self)
{
  g_debug ("close request");

  ZRYTHM->creating_project = true;
  ZRYTHM->open_filename = NULL;

  /* window already destroyed, set transient to splash screen
   * instead */
  run_create_project_dialog (self, GTK_WINDOW (self->parent));

  /* close normally */
  return false;
}

static void
on_ok_clicked (GtkButton * btn, ProjectAssistantWidget * self)
{
  ZRYTHM->creating_project = true;

  const char * child_name = adw_view_stack_get_visible_child_name (self->stack);

  ProjectInfo * selected_project = get_selected_project (self);
  ProjectInfo * selected_template = get_selected_template (self);

  if (string_is_equal (child_name, "create-new"))
    {
      g_return_if_fail (selected_template);

      /* if we are loading a blank template */
      if (selected_template->filename[0] == '-')
        {
          ZRYTHM->open_filename = NULL;
          g_message ("Creating blank project");
        }
      else
        {
          ZRYTHM->open_filename = selected_template->filename;
          g_message (
            "Creating project from template: %s", ZRYTHM->open_filename);
          ZRYTHM->opening_template = true;
        }
    }
  /* else if we are loading a project */
  else if (string_is_equal (child_name, "open-recent"))
    {
      if (!selected_project)
        {
          ui_show_error_message (false, _ ("No project selected"));
          return;
        }
      ZRYTHM->open_filename = selected_project->filename;
      g_return_if_fail (ZRYTHM->open_filename);
      g_message ("Loading project: %s", ZRYTHM->open_filename);
      ZRYTHM->creating_project = false;
    }

  /* if not loading a project, show dialog to select directory
   * and name */
  if (ZRYTHM->creating_project)
    {
      run_create_project_dialog (self, GTK_WINDOW (self));
    }
  else
    {
      post_finish (self, self->zrythm_already_running, false);
    }
}

static void
open_ready_cb (
  GtkFileDialog *          dialog,
  GAsyncResult *           res,
  ProjectAssistantWidget * self)
{
  GError * err = NULL;
  GFile *  file = gtk_file_dialog_open_finish (dialog, res, &err);
  if (!file)
    {
      g_message ("no project selected: %s", err->message);
      g_error_free (err);
      return;
    }

  char * path = g_file_get_path (file);
  g_return_if_fail (path);
  g_object_unref (file);

  ZRYTHM->open_filename = path;
  g_message ("Loading project: %s", ZRYTHM->open_filename);

  post_finish (self, self->zrythm_already_running, false);
}

static void
on_open_from_path_clicked (GtkButton * btn, ProjectAssistantWidget * self)
{
  ZRYTHM->creating_project = true;

  GtkFileDialog * dialog = gtk_file_dialog_new ();
  gtk_file_dialog_set_title (dialog, _ ("Select Project"));
  gtk_file_dialog_set_modal (dialog, true);
  GtkFileFilter * filter = gtk_file_filter_new ();
  gtk_file_filter_add_mime_type (filter, "application/x-zrythm-project");
  gtk_file_filter_add_suffix (filter, "zpj");
  GListStore * list_store = g_list_store_new (GTK_TYPE_FILE_FILTER);
  g_list_store_append (list_store, filter);
  gtk_file_dialog_set_filters (dialog, G_LIST_MODEL (list_store));
  g_object_unref (list_store);
  gtk_file_dialog_open (
    dialog, GTK_WINDOW (self), NULL, (GAsyncReadyCallback) open_ready_cb, self);
}

static void
on_project_activate (
  GtkColumnView * column_view,
  guint           position,
  gpointer        user_data)
{
  ProjectAssistantWidget * self = (ProjectAssistantWidget *) user_data;

  GListStore * list_store = z_gtk_column_view_get_list_store (column_view);
  WrappedObjectWithChangeSignal * wrapped_obj =
    Z_WRAPPED_OBJECT_WITH_CHANGE_SIGNAL (
      g_list_model_get_item (G_LIST_MODEL (list_store), position));
  ProjectInfo * nfo = (ProjectInfo *) wrapped_obj->obj;

  g_debug ("activated %s", nfo->filename);

  on_ok_clicked (self->ok_btn, self);
}

static char *
get_prj_name (void * data)
{
  WrappedObjectWithChangeSignal * wrapped_obj =
    Z_WRAPPED_OBJECT_WITH_CHANGE_SIGNAL (data);
  ProjectInfo * nfo = (ProjectInfo *) wrapped_obj->obj;

  return g_strdup (nfo->name);
}

static char *
get_prj_path (void * data)
{
  WrappedObjectWithChangeSignal * wrapped_obj =
    Z_WRAPPED_OBJECT_WITH_CHANGE_SIGNAL (data);
  ProjectInfo * nfo = (ProjectInfo *) wrapped_obj->obj;

  return g_strdup (nfo->filename);
}

static gint64
get_prj_last_modified (void * data)
{
  WrappedObjectWithChangeSignal * wrapped_obj =
    Z_WRAPPED_OBJECT_WITH_CHANGE_SIGNAL (data);
  ProjectInfo * nfo = (ProjectInfo *) wrapped_obj->obj;

  return nfo->modified;
}

static void
set_column_view_model (GtkColumnView * column_view)
{
  GListStore *         store;
  GtkSorter *          sorter;
  GtkSortListModel *   sort_list_model;
  GtkSingleSelection * sel;

  store = g_list_store_new (WRAPPED_OBJECT_WITH_CHANGE_SIGNAL_TYPE);
  sorter = gtk_column_view_get_sorter (column_view);
  sorter = g_object_ref (sorter);
  sort_list_model = gtk_sort_list_model_new (G_LIST_MODEL (store), sorter);
  sel = GTK_SINGLE_SELECTION (
    gtk_single_selection_new (G_LIST_MODEL (sort_list_model)));
  gtk_column_view_set_model (column_view, GTK_SELECTION_MODEL (sel));
}

static void
add_column_view_columns (
  ProjectAssistantWidget * self,
  GtkColumnView *          column_view,
  GPtrArray *              item_factories)
{
  GtkSorter *     sorter;
  GtkExpression * expression;

  /* name */
  expression = gtk_cclosure_expression_new (
    G_TYPE_STRING, NULL, 0, NULL, G_CALLBACK (get_prj_name), NULL, NULL);
  sorter = GTK_SORTER (gtk_string_sorter_new (expression));
  item_factory_generate_and_append_column (
    column_view, item_factories, ITEM_FACTORY_TEXT, Z_F_NOT_EDITABLE,
    Z_F_RESIZABLE, sorter,
    column_view == self->recent_projects_column_view
      ? _ ("Name")
      : _ ("Template Name"));

  /* path */
  expression = gtk_cclosure_expression_new (
    G_TYPE_STRING, NULL, 0, NULL, G_CALLBACK (get_prj_path), NULL, NULL);
  sorter = GTK_SORTER (gtk_string_sorter_new (expression));
  item_factory_generate_and_append_column (
    column_view, item_factories, ITEM_FACTORY_TEXT, Z_F_NOT_EDITABLE,
    Z_F_RESIZABLE, sorter, _ ("Path"));

  /* last modified */
  expression = gtk_cclosure_expression_new (
    G_TYPE_INT64, NULL, 0, NULL, G_CALLBACK (get_prj_last_modified), NULL, NULL);
  sorter = GTK_SORTER (gtk_numeric_sorter_new (expression));
  item_factory_generate_and_append_column (
    column_view, item_factories, ITEM_FACTORY_POSITION, Z_F_NOT_EDITABLE,
    Z_F_RESIZABLE, sorter, _ ("Last Modified"));
}

static void
project_assistant_finalize (ProjectAssistantWidget * self)
{
  g_ptr_array_unref (self->recent_projects_item_factories);
  g_ptr_array_unref (self->templates_item_factories);

  G_OBJECT_CLASS (project_assistant_widget_parent_class)
    ->finalize (G_OBJECT (self));
}

static void
project_assistant_widget_class_init (ProjectAssistantWidgetClass * _klass)
{
  GtkWidgetClass * klass = GTK_WIDGET_CLASS (_klass);
  resources_set_class_template (klass, "project_assistant.ui");
  gtk_widget_class_set_css_name (klass, "project-assistant");

#define BIND_CHILD(x) \
  gtk_widget_class_bind_template_child (klass, ProjectAssistantWidget, x)

  BIND_CHILD (stack);
  BIND_CHILD (recent_projects_column_view);
  BIND_CHILD (templates_column_view);
  BIND_CHILD (ok_btn);
  BIND_CHILD (open_from_path_btn);
  /*BIND_CHILD (cancel_btn);*/

#undef BIND_CHILD

  GObjectClass * oklass = G_OBJECT_CLASS (klass);
  oklass->finalize = (GObjectFinalizeFunc) project_assistant_finalize;
}

static void
project_assistant_widget_init (ProjectAssistantWidget * self)
{
  g_type_ensure (ADW_TYPE_TOOLBAR_VIEW);

  gtk_widget_init_template (GTK_WIDGET (self));

  gtk_widget_set_size_request (GTK_WIDGET (self), 380, 180);

  self->recent_projects_item_factories =
    g_ptr_array_new_with_free_func (item_factory_free_func);
  self->templates_item_factories =
    g_ptr_array_new_with_free_func (item_factory_free_func);

  /* create sortable recent projects list store */
  set_column_view_model (self->recent_projects_column_view);
  set_column_view_model (self->templates_column_view);
  GtkSelectionModel * sel;
  sel = gtk_column_view_get_model (self->recent_projects_column_view);
  g_signal_connect (
    sel, "selection-changed", G_CALLBACK (on_project_selection_changed), self);
  sel = gtk_column_view_get_model (self->templates_column_view);
  g_signal_connect (
    sel, "selection-changed", G_CALLBACK (on_template_selection_changed), self);

  /* add columns */
  add_column_view_columns (
    self, self->recent_projects_column_view,
    self->recent_projects_item_factories);
  add_column_view_columns (
    self, self->templates_column_view, self->templates_item_factories);

  self->project_infos_arr =
    g_ptr_array_new_with_free_func (project_info_destroy_func);
  self->templates_arr =
    g_ptr_array_new_with_free_func (project_info_destroy_func);

  /* fill recent projects */
  for (int i = 0; i < ZRYTHM->num_recent_projects; i++)
    {
      char * dir = io_get_dir (ZRYTHM->recent_projects[i]);
      char * project_name = g_path_get_basename (dir);

      ProjectInfo * prj_nfo =
        project_info_new (project_name, ZRYTHM->recent_projects[i]);
      g_ptr_array_add (self->project_infos_arr, prj_nfo);

      g_free (project_name);
      g_free (dir);
    }

  /* fill templates */
  {
    ProjectInfo * blank_template = project_info_new (_ ("Blank project"), NULL);
    g_ptr_array_add (self->templates_arr, blank_template);
  }
  int count = 0;
  char * template;
  while ((template = ZRYTHM->templates[count]) != NULL)
    {
      char * name = g_path_get_basename (template);
      char * filename = g_build_filename (template, PROJECT_FILE, NULL);

      ProjectInfo * template_nfo = project_info_new (name, filename);
      g_ptr_array_add (self->templates_arr, template_nfo);

      g_free (name);
      g_free (filename);

      count++;
    }

  refresh_projects (self);
  refresh_templates (self);

  /* add delete handler */
  GtkEventController * key_controller = gtk_event_controller_key_new ();
  g_signal_connect (
    G_OBJECT (key_controller), "key-released", G_CALLBACK (on_key_release),
    self);
  gtk_widget_add_controller (
    GTK_WIDGET (self->recent_projects_column_view), key_controller);

  /* add row activation */
  g_signal_connect (
    self->recent_projects_column_view, "activate",
    G_CALLBACK (on_project_activate), self);

  g_signal_connect (
    self->stack, "notify::visible-child-name",
    G_CALLBACK (on_visible_child_name_changed), self);

  g_signal_connect (self->ok_btn, "clicked", G_CALLBACK (on_ok_clicked), self);
  g_signal_connect (
    self->open_from_path_btn, "clicked", G_CALLBACK (on_open_from_path_clicked),
    self);
  g_signal_connect (self, "close-request", G_CALLBACK (on_close_request), self);

  gtk_window_set_focus (
    GTK_WINDOW (self), GTK_WIDGET (self->recent_projects_column_view));

  /* close on escape */
  z_gtk_window_make_escapable (GTK_WINDOW (self));

  gtk_accessible_update_property (
    GTK_ACCESSIBLE (self->recent_projects_column_view),
    GTK_ACCESSIBLE_PROPERTY_LABEL, "Recent projects", -1);
}
