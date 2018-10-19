/*
 * utils/dialogs.c - Dialogs
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef __UTILS_DIALOGS_H__
#define __UTILS_DIALOGS_H__

#include "gui/widgets/main_window.h"
#include "utils/dialogs.h"

#include <gtk/gtk.h>

/**
 * Creates and returns an open project dialog.
 */
GtkDialog * dialogs_get_open_project_dialog (GtkWindow * parent)
{
  GtkFileChooserAction action = GTK_FILE_CHOOSER_ACTION_OPEN;

  GtkWidget * dialog = gtk_file_chooser_dialog_new ("Open Project",
                                        GTK_WINDOW (MAIN_WINDOW),
                                        action,
                                        "_Cancel",
                                        GTK_RESPONSE_CANCEL,
                                        "_Open",
                                        GTK_RESPONSE_ACCEPT,
                                        NULL);
  return dialog;
}

#endif


