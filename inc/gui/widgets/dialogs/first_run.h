// SPDX-FileCopyrightText: © 2022-2023 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef __GUI_WIDGETS_FIRST_RUN_DIALOG_H__
#define __GUI_WIDGETS_FIRST_RUN_DIALOG_H__

#include <adwaita.h>
#include <gtk/gtk.h>

#define FIRST_RUN_WINDOW_TYPE (first_run_window_get_type ())
G_DECLARE_FINAL_TYPE (
  FirstRunWindow,
  first_run_window,
  Z,
  FIRST_RUN_WINDOW,
  AdwWindow)

#define FIRST_RUN_DIALOG_RESET_RESPONSE 450

typedef struct _IdeFileChooserEntry IdeFileChooserEntry;

typedef struct _FirstRunWindow
{
  AdwWindow parent_instance;

  AdwPreferencesPage * pref_page;

  AdwComboRow *         language_dropdown;
  GtkLabel *            lang_error_txt;
  IdeFileChooserEntry * fc_entry;

  GtkButton * ok_btn;
  GtkButton * reset_btn;
  GtkButton * cancel_btn;
} FirstRunWindow;

/**
 * Returns a new instance.
 */
FirstRunWindow *
first_run_window_new (GtkWindow * parent);

#endif
