/*
 * Copyright (C) 2019 Alexandros Theodotou <alex at zrythm dot org>
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

#define EXPORT_MIDI_FILE_DIALOG_WIDGET_TYPE \
  (export_midi_file_dialog_widget_get_type ())
G_DECLARE_FINAL_TYPE (
  ExportMidiFileDialogWidget,
  export_midi_file_dialog_widget,
  Z,
  EXPORT_MIDI_FILE_DIALOG_WIDGET,
  GtkFileChooserDialog)

typedef struct _MidiControllerMbWidget
  MidiControllerMbWidget;

typedef struct _ExportMidiFileDialogWidget
{
  GtkFileChooserDialog parent_instance;

  /** Description to show for the region. */
  GtkLabel *           description;

  /** Region, if exporting region. */
  Region *             region;
} ExportMidiFileDialogWidget;

/**
 * Creates a ExportMidiFileDialog.
 */
ExportMidiFileDialogWidget *
export_midi_file_dialog_widget_new_for_region (
  GtkWindow * parent,
  Region *    region);

#endif

