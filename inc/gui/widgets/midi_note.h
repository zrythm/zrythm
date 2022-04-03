/*
 * Copyright (C) 2018-2022 Alexandros Theodotou <alex at zrythm dot org>
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
