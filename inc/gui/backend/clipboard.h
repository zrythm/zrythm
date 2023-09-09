// SPDX-FileCopyrightText: © 2020-2022 Alexandros Theodotou <alex@zrythm.org>
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

static const cyaml_strval_t clipboard_type_strings[] = {
  {"Timeline selections",    CLIPBOARD_TYPE_TIMELINE_SELECTIONS  },
  { "MIDI selections",       CLIPBOARD_TYPE_MIDI_SELECTIONS      },
  { "Automation selections", CLIPBOARD_TYPE_AUTOMATION_SELECTIONS},
  { "Chord selections",      CLIPBOARD_TYPE_CHORD_SELECTIONS     },
  { "Audio selections",      CLIPBOARD_TYPE_AUDIO_SELECTIONS     },
  { "Mixer selections",      CLIPBOARD_TYPE_MIXER_SELECTIONS     },
  { "Tracklist selections",  CLIPBOARD_TYPE_TRACKLIST_SELECTIONS },
};

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

static const cyaml_schema_field_t clipboard_fields_schema[] = {
  YAML_FIELD_ENUM (Clipboard, type, clipboard_type_strings),
  YAML_FIELD_MAPPING_PTR_OPTIONAL (
    Clipboard,
    timeline_sel,
    timeline_selections_fields_schema),
  YAML_FIELD_MAPPING_PTR_OPTIONAL (
    Clipboard,
    ma_sel,
    midi_arranger_selections_fields_schema),
  YAML_FIELD_MAPPING_PTR_OPTIONAL (
    Clipboard,
    chord_sel,
    chord_selections_fields_schema),
  YAML_FIELD_MAPPING_PTR_OPTIONAL (
    Clipboard,
    automation_sel,
    automation_selections_fields_schema),
  YAML_FIELD_MAPPING_PTR_OPTIONAL (
    Clipboard,
    audio_sel,
    audio_selections_fields_schema),
  YAML_FIELD_MAPPING_PTR_OPTIONAL (
    Clipboard,
    mixer_sel,
    mixer_selections_fields_schema),
  YAML_FIELD_MAPPING_PTR_OPTIONAL (
    Clipboard,
    tracklist_sel,
    tracklist_selections_fields_schema),

  CYAML_FIELD_END
};

static const cyaml_schema_value_t clipboard_schema = {
  YAML_VALUE_PTR (Clipboard, clipboard_fields_schema),
};

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
