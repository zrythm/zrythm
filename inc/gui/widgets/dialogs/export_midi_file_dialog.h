/*
 * Copyright (C) 2019, 2021 Alexandros Theodotou <alex at zrythm dot org>
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

#ifndef __GUI_WIDGETS_EXPORT_MIDI_FILE_DIALOG_H__
#define __GUI_WIDGETS_EXPORT_MIDI_FILE_DIALOG_H__

#include "audio/channel.h"
#include "gui/widgets/meter.h"

#include <gtk/gtk.h>

typedef struct _MidiControllerMbWidget
  MidiControllerMbWidget;

typedef struct _ExportMidiFileDialogWidget
{
  GtkFileChooserDialog * dialog;

  /** Description to show for the region. */
  GtkLabel *           description;

  /** ZRegion, if exporting region. */
  ZRegion *             region;
} ExportMidiFileDialogWidget;

/**
 * Runs a new dialog and returns the filepath if
 * selected or NULL if canceled.
 */
char *
export_midi_file_dialog_widget_run_for_region (
  GtkWindow * parent,
  ZRegion *   region);

#endif
