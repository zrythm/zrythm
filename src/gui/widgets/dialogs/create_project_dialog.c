// SPDX-FileCopyrightText: © 2019-2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "gui/widgets/dialogs/create_project_dialog.h"
#include "gui/widgets/file_chooser_button.h"
#include "settings/settings.h"
#include "utils/io.h"
#include "utils/objects.h"
#include "utils/resources.h"
#include "zrythm.h"
#include "zrythm_app.h"

#include <glib/gi18n.h>

G_DEFINE_TYPE (
  CreateProjectDialogWidget,
  create_project_dialog_widget,
  GTK_TYPE_DIALOG)

static void
on_response (
  GtkDialog *              dialog,
  gint                     response_id,
  CreateProjectDialogWidget * self)
{
  switch (response_id)
    {
    case GTK_RESPONSE_CANCEL:
    case GTK_RESPONSE_CLOSE:
      return;
    }

  /* get the zrythm project name */
  char * str =
    g_settings_get_string (S_GENERAL, "last-project-dir");
  ZRYTHM->create_project_path = g_build_filename (
    str, gtk_editable_get_text (GTK_EDITABLE (self->name)),
    NULL);
  g_free (str);

  /* TODO validate */
}

static void
on_name_activate (
  GtkEntry *                  entry,
  CreateProjectDialogWidget * self)
{
  gtk_dialog_response (GTK_DIALOG (self), GTK_RESPONSE_OK);
}

static void
on_name_changed (
  GtkEditable *               editable,
  CreateProjectDialogWidget * self)
{
  gtk_dialog_set_response_sensitive (
    GTK_DIALOG (self), GTK_RESPONSE_OK,
    strlen (gtk_editable_get_text (GTK_EDITABLE (self->name)))
      > 0);
}

static void
on_fc_file_set (
  GtkNativeDialog * dialog,
  gint              response_id,
  gpointer          user_data)
{
  CreateProjectDialogWidget * self =
    Z_CREATE_PROJECT_DIALOG_WIDGET (user_data);

  GtkFileChooser * file_chooser = GTK_FILE_CHOOSER (dialog);
  GFile * file = gtk_file_chooser_get_file (file_chooser);
  char *  str = g_file_get_path (file);
  g_settings_set_string (S_GENERAL, "last-project-dir", str);
  g_free (str);

  file_chooser_button_widget_std_response (
    Z_FILE_CHOOSER_BUTTON_WIDGET (self->fc), dialog,
    response_id);
}

CreateProjectDialogWidget *
create_project_dialog_widget_new ()
{
  CreateProjectDialogWidget * self = g_object_new (
    CREATE_PROJECT_DIALOG_WIDGET_TYPE, "title",
    _ ("Create New Project"), "icon-name", "zrythm", NULL);

  char * str =
    g_settings_get_string (S_GENERAL, "last-project-dir");
  file_chooser_button_widget_set_path (self->fc, str);

  /* get next available "Untitled Project" */
  char * untitled_project = g_strdup (_ ("Untitled Project"));
  char * tmp = g_build_filename (str, untitled_project, NULL);
  char * dir = io_get_next_available_filepath (tmp);
  g_free (tmp);
  g_free (untitled_project);
  untitled_project = g_path_get_basename (dir);
  g_free (dir);
  g_free (str);
  gtk_editable_set_text (
    GTK_EDITABLE (self->name), untitled_project);
  gtk_editable_select_region (
    GTK_EDITABLE (self->name), 0,
    (int) strlen (untitled_project));
  g_free (untitled_project);

  gtk_widget_grab_focus (GTK_WIDGET (self->name));

  return self;
}

static void
create_project_dialog_widget_init (
  CreateProjectDialogWidget * self)
{
  g_type_ensure (FILE_CHOOSER_BUTTON_WIDGET_TYPE);

  gtk_widget_init_template (GTK_WIDGET (self));

  gtk_dialog_add_button (
    GTK_DIALOG (self), _("_Cancel"), GTK_RESPONSE_CANCEL);
  gtk_dialog_add_button (
    GTK_DIALOG (self), _("_OK"), GTK_RESPONSE_OK);
  gtk_dialog_set_default_response (
    GTK_DIALOG (self), GTK_RESPONSE_OK);

  file_chooser_button_widget_setup (
    self->fc, GTK_WINDOW (self),
    _ ("Select parent directory to save the project "
       "in"),
    GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER);
  file_chooser_button_widget_set_response_callback (
    self->fc, G_CALLBACK (on_fc_file_set), self, NULL);

  g_signal_connect (
    self, "response", G_CALLBACK (on_response), self);
}

static void
create_project_dialog_widget_class_init (
  CreateProjectDialogWidgetClass * _klass)
{
  GtkWidgetClass * klass = GTK_WIDGET_CLASS (_klass);
  resources_set_class_template (
    klass, "create_project_dialog.ui");

#define BIND_CHILD(x) \
  gtk_widget_class_bind_template_child ( \
    klass, CreateProjectDialogWidget, x)

  BIND_CHILD (fc);
  BIND_CHILD (name);

  gtk_widget_class_bind_template_callback (
    klass, on_name_changed);
  gtk_widget_class_bind_template_callback (
    klass, on_name_activate);

#undef BIND_CHILD
}
