/*
 * Copyright (C) 2018-2019 Alexandros Theodotou <alex at zrythm dot org>
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

#if 0
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

#define MIDI_NOTE_WIDGET_TYPE \
  (midi_note_widget_get_type ())
G_DECLARE_FINAL_TYPE (
  MidiNoteWidget,
  midi_note_widget,
  Z, MIDI_NOTE_WIDGET,
  ArrangerObjectWidget)

/**
 * @addtogroup widgets
 *
 * @{
 */

/**
 * GUI widget for a MidiNote.
 */
typedef struct _MidiNoteWidget
{
  ArrangerObjectWidget parent_instance;

  /** The MidiNote associated with this widget. */
  MidiNote *         midi_note;

} MidiNoteWidget;

/**
 * Creates a midi_note.
 */
MidiNoteWidget *
midi_note_widget_new (MidiNote * midi_note);

/**
 * @}
 */

#endif
#endif
