/*
 * utils/dialogs.h - Dialogs
 *
 * Copyright (C) 2018 Alexandros Theodotou
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef __UTILS_DIALOGS_H__
#define __UTILS_DIALOGS_H__

#include <gtk/gtk.h>

/**
 * Creates and returns an open project dialog.
 */
GtkDialog *
dialogs_get_open_project_dialog (GtkWindow * parent);

/**
 * Creates and returns the overwrite plugin dialog.
 */
GtkDialog *
dialogs_get_overwrite_plugin_dialog (GtkWindow * parent);

GtkDialog *
dialogs_get_error_instantiating_plugin_dialog (
  GtkWindow * parent);

#endif
