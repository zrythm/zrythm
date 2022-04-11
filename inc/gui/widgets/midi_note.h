// SPDX-FileCopyrightText: © 2018-2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * \file
 *
 * MidiNote widget API.
 */

#ifndef __GUI_WIDGETS_MIDI_NOTE_H__
#define __GUI_WIDGETS_MIDI_NOTE_H__

#include "audio/midi_note.h"
#include "gui/widgets/arranger_object.h"
#include "utils/ui.h"

#include <gtk/gtk.h>

/**
 * @addtogroup widgets
 *
 * @{
 */

/**
 * @param cr Arranger's cairo context.
 * @param arr_rect Arranger's rectangle.
 */
NONNULL
void
midi_note_draw (
  MidiNote *    self,
  GtkSnapshot * snapshot);

NONNULL
void
midi_note_get_adjusted_color (
  MidiNote * self,
  GdkRGBA *  color);

/**
 * @}
 */

#endif
