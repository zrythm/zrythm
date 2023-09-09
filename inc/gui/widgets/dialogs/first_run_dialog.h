// SPDX-FileCopyrightText: © 2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef __GUI_WIDGETS_FIRST_RUN_DIALOG_H__
#define __GUI_WIDGETS_FIRST_RUN_DIALOG_H__

#include <adwaita.h>
#include <gtk/gtk.h>

#define FIRST_RUN_DIALOG_WIDGET_TYPE (first_run_dialog_widget_get_type ())
G_DECLARE_FINAL_TYPE (
  FirstRunDialogWidget,
  first_run_dialog_widget,
  Z,
  FIRST_RUN_DIALOG_WIDGET,
  GtkDialog)

#define FIRST_RUN_DIALOG_RESET_RESPONSE 450

typedef struct _IdeFileChooserEntry IdeFileChooserEntry;

typedef struct _FirstRunDialogWidget
{
  GtkDialog parent_instance;

  AdwComboRow *         language_dropdown;
  IdeFileChooserEntry * fc_entry;

  GtkLabel * lang_error_txt;
} FirstRunDialogWidget;

/**
 * Returns a new instance.
 */
FirstRunDialogWidget *
first_run_dialog_widget_new (GtkWindow * parent);

void
first_run_dialog_widget_reset (FirstRunDialogWidget * self);

void
first_run_dialog_widget_ok (FirstRunDialogWidget * self);

#endif
