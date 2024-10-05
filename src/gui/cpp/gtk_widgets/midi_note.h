// SPDX-FileCopyrightText: © 2018-2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * @file
 *
 * MidiNote widget API.
 */

#ifndef __GUI_WIDGETS_MIDI_NOTE_H__
#define __GUI_WIDGETS_MIDI_NOTE_H__

#include "dsp/midi_note.h"
#include "gui/cpp/gtk_widgets/arranger_object.h"
#include "utils/ui.h"

#include "gtk_wrapper.h"

/**
 * @addtogroup widgets
 *
 * @{
 */

/**
 * @param cr Arranger's cairo context.
 * @param arr_rect Arranger's rectangle.
 */
ATTR_NONNULL void
midi_note_draw (MidiNote * self, GtkSnapshot * snapshot);

ATTR_NONNULL void
midi_note_get_adjusted_color (MidiNote * self, Color &color);

/**
 * @}
 */

#endif
