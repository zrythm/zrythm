// SPDX-FileCopyrightText: © 2019, 2021-2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef __GUI_WIDGETS_EXPORT_MIDI_FILE_DIALOG_H__
#define __GUI_WIDGETS_EXPORT_MIDI_FILE_DIALOG_H__

#include "gtk_wrapper.h"

class Region;

/**
 * Runs the dialog asynchronously.
 *
 * @param region Region, if exporting region.
 */
void
export_midi_file_dialog_widget_run (
  GtkWindow *                parent,
  const TimelineSelections * sel);

#endif
