// SPDX-FileCopyrightText: © 2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense
/*
 * This file incorporates work covered by the following copyright and
 * permission notice:
 *
 * ---
 *
 * Copyright (C) 2016-2022 Christian Hergert <chergert@redhat.com>
 *
 * This file is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This file is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * ---
 */

#pragma once
#include "gtk_wrapper.h"

G_BEGIN_DECLS

#define IDE_TYPE_FILE_CHOOSER_ENTRY (ide_file_chooser_entry_get_type ())

G_DECLARE_FINAL_TYPE (
  IdeFileChooserEntry,
  ide_file_chooser_entry,
  IDE,
  FILE_CHOOSER_ENTRY,
  GtkWidget)

GtkWidget *
ide_file_chooser_entry_new (const gchar * title, GtkFileChooserAction action);
GFile *
ide_file_chooser_entry_get_file (IdeFileChooserEntry * self);
void
ide_file_chooser_entry_set_file (IdeFileChooserEntry * self, GFile * file);
GtkEntry *
ide_file_chooser_entry_get_entry (IdeFileChooserEntry * self);

G_END_DECLS
