/*
 * Copyright (C) 2020-2022 Alexandros Theodotou <alex at zrythm dot org>
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
 * Clipboard (copy/paste).
 */

#ifndef __GUI_BACKEND_CLIPBOARD_H__
#define __GUI_BACKEND_CLIPBOARD_H__

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
  CLIPBOARD_TYPE_MIXER_SELECTIONS,
  CLIPBOARD_TYPE_TRACKLIST_SELECTIONS,
} ClipboardType;

static const cyaml_strval_t
clipboard_type_strings[] =
{
  { "Timeline selections",
    CLIPBOARD_TYPE_TIMELINE_SELECTIONS },
  { "MIDI selections",
    CLIPBOARD_TYPE_MIDI_SELECTIONS },
  { "Automation selections",
    CLIPBOARD_TYPE_AUTOMATION_SELECTIONS },
  { "Chord selections",
    CLIPBOARD_TYPE_CHORD_SELECTIONS },
  { "Mixer selections",
    CLIPBOARD_TYPE_MIXER_SELECTIONS },
  { "Tracklist selections",
    CLIPBOARD_TYPE_TRACKLIST_SELECTIONS },
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
  MixerSelections *        mixer_sel;
  TracklistSelections *    tracklist_sel;
} Clipboard;

static const cyaml_schema_field_t
  clipboard_fields_schema[] =
{
  YAML_FIELD_ENUM (
    Clipboard, type, clipboard_type_strings),
  YAML_FIELD_MAPPING_PTR_OPTIONAL (
    Clipboard, timeline_sel,
    timeline_selections_fields_schema),
  YAML_FIELD_MAPPING_PTR_OPTIONAL (
    Clipboard, ma_sel,
    midi_arranger_selections_fields_schema),
  YAML_FIELD_MAPPING_PTR_OPTIONAL (
    Clipboard, chord_sel,
    chord_selections_fields_schema),
  YAML_FIELD_MAPPING_PTR_OPTIONAL (
    Clipboard, automation_sel,
    automation_selections_fields_schema),
  YAML_FIELD_MAPPING_PTR_OPTIONAL (
    Clipboard, mixer_sel,
    mixer_selections_fields_schema),
  YAML_FIELD_MAPPING_PTR_OPTIONAL (
    Clipboard, tracklist_sel,
    tracklist_selections_fields_schema),

  CYAML_FIELD_END
};

static const cyaml_schema_value_t
  clipboard_schema =
{
  YAML_VALUE_PTR (
    Clipboard, clipboard_fields_schema),
};

/**
 * Creates a new Clipboard instance for the given
 * arranger selections.
 */
Clipboard *
clipboard_new_for_arranger_selections (
  ArrangerSelections * sel,
  bool                 clone);

Clipboard *
clipboard_new_for_mixer_selections (
  MixerSelections * sel,
  bool              clone);

Clipboard *
clipboard_new_for_tracklist_selections (
  TracklistSelections * sel,
  bool                  clone);

/**
 * Gets the ArrangerSelections, if this clipboard
 * contains arranger selections.
 */
ArrangerSelections *
clipboard_get_selections (
  Clipboard * self);

/**
 * Frees the clipboard and all associated data.
 */
void
clipboard_free (
  Clipboard * self);

/**
 * @}
 */

#endif
