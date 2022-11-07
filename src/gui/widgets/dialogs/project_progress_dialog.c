// SPDX-FileCopyrightText: © 2021-2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * \file
 *
 * Project dialog.
 */

#include <stdio.h>

#include "gui/widgets/dialogs/project_progress_dialog.h"
#include "gui/widgets/main_window.h"
#include "project.h"
#include "utils/flags.h"
#include "utils/io.h"
#include "utils/math.h"
#include "utils/resources.h"
#include "utils/ui.h"
#include "zrythm_app.h"

#include <glib/gi18n.h>
#include <gtk/gtk.h>

G_DEFINE_TYPE (
  ProjectProgressDialogWidget,
  project_progress_dialog_widget,
  GENERIC_PROGRESS_DIALOG_WIDGET_TYPE)

/**
 * Creates an project dialog widget and displays it.
 */
ProjectProgressDialogWidget *
project_progress_dialog_widget_new (
  ProjectSaveData * project_save_data)
{
  g_type_ensure (GENERIC_PROGRESS_DIALOG_WIDGET_TYPE);

  ProjectProgressDialogWidget * self =
    g_object_new (PROJECT_PROGRESS_DIALOG_WIDGET_TYPE, NULL);

  GenericProgressDialogWidget * generic_progress_dialog =
    Z_GENERIC_PROGRESS_DIALOG_WIDGET (self);

  project_save_data->progress_info->label_str =
    g_strdup ("Saving...");
  strcpy (
    project_save_data->progress_info.label_done_str,
    _ ("Saved"));

  generic_progress_dialog_widget_setup (
    generic_progress_dialog, _ ("Project Progress"),
    &project_save_data->progress_info, F_AUTO_CLOSE,
    F_NOT_CANCELABLE);

  return self;
}

static void
project_progress_dialog_widget_class_init (
  ProjectProgressDialogWidgetClass * _klass)
{
}

static void
project_progress_dialog_widget_init (
  ProjectProgressDialogWidget * self)
{
  g_type_ensure (GENERIC_PROGRESS_DIALOG_WIDGET_TYPE);
}
