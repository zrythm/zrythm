// SPDX-FileCopyrightText: © 2020-2023 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * \file
 *
 * Clipboard (copy/paste).
 */

#ifndef __GUI_BACKEND_CLIPBOARD_H__
#define __GUI_BACKEND_CLIPBOARD_H__

#include "gui/backend/audio_selections.h"
#include "gui/backend/automation_selections.h"
#include "gui/backend/chord_selections.h"
#include "gui/backend/midi_arranger_selections.h"
#include "gui/backend/mixer_selections.h"
#include "gui/backend/timeline_selections.h"
#include "gui/backend/tracklist_selections.h"
#include "utils/yaml.h"

#include <gtk/gtk.h>

/**
 * @addtogroup gui_backend
 */

/**
 * Clipboard type.
 */
typedef enum ClipboardType
{
  CLIPBOARD_TYPE_TIMELINE_SELECTIONS,
  CLIPBOARD_TYPE_MIDI_SELECTIONS,
  CLIPBOARD_TYPE_AUTOMATION_SELECTIONS,
  CLIPBOARD_TYPE_CHORD_SELECTIONS,
  CLIPBOARD_TYPE_AUDIO_SELECTIONS,
  CLIPBOARD_TYPE_MIXER_SELECTIONS,
  CLIPBOARD_TYPE_TRACKLIST_SELECTIONS,
} ClipboardType;

#if 0
static const char * clipboard_type_strings[] = {
  "Timeline selections",
   "MIDI selections",
   "Automation selections",
   "Chord selections",
   "Audio selections",
   "Mixer selections",
   "Tracklist selections",
};
#endif

/**
 * Clipboard struct.
 */
typedef struct Clipboard
{
  ClipboardType            type;
  TimelineSelections *     timeline_sel;
  MidiArrangerSelections * ma_sel;
  ChordSelections *        chord_sel;
  AutomationSelections *   automation_sel;
  AudioSelections *        audio_sel;
  MixerSelections *        mixer_sel;
  TracklistSelections *    tracklist_sel;
} Clipboard;

/**
 * Creates a new Clipboard instance for the given
 * arranger selections.
 */
Clipboard *
clipboard_new_for_arranger_selections (ArrangerSelections * sel, bool clone);

Clipboard *
clipboard_new_for_mixer_selections (MixerSelections * sel, bool clone);

Clipboard *
clipboard_new_for_tracklist_selections (TracklistSelections * sel, bool clone);

/**
 * Gets the ArrangerSelections, if this clipboard
 * contains arranger selections.
 */
ArrangerSelections *
clipboard_get_selections (Clipboard * self);

/**
 * Frees the clipboard and all associated data.
 */
void
clipboard_free (Clipboard * self);

/**
 * @}
 */

#endif
