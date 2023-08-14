// SPDX-FileCopyrightText: © 2019-2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * \file
 *
 * Chord editor backend.
 */

#ifndef __GUI_BACKEND_CHORD_EDITOR_H__
#define __GUI_BACKEND_CHORD_EDITOR_H__

#include "dsp/chord_descriptor.h"
#include "dsp/scale.h"
#include "gui/backend/editor_settings.h"
#include "utils/yaml.h"

typedef struct ChordDescriptor ChordDescriptor;
typedef struct ChordPreset     ChordPreset;

/**
 * @addtogroup gui_backend
 *
 * @{
 */

#define CHORD_EDITOR_SCHEMA_VERSION 1

#define CHORD_EDITOR (CLIP_EDITOR->chord_editor)

#define CHORD_EDITOR_NUM_CHORDS 12

/**
 * Backend for the chord editor.
 */
typedef struct ChordEditor
{
  int schema_version;

  /**
   * The chords to show on the left.
   *
   * Currently fixed to 12 chords whose order cannot
   * be edited. Chords cannot be added or removed.
   */
  ChordDescriptor * chords[128];
  int               num_chords;

  EditorSettings editor_settings;
} ChordEditor;

static const cyaml_schema_field_t chord_editor_fields_schema[] = {
  YAML_FIELD_INT (ChordEditor, schema_version),
  YAML_FIELD_FIXED_SIZE_PTR_ARRAY_VAR_COUNT (
    ChordEditor,
    chords,
    chord_descriptor_schema),
  YAML_FIELD_MAPPING_EMBEDDED (
    ChordEditor,
    editor_settings,
    editor_settings_fields_schema),

  CYAML_FIELD_END
};

static const cyaml_schema_value_t chord_editor_schema = {
  CYAML_VALUE_MAPPING (
    CYAML_FLAG_POINTER,
    ChordEditor,
    chord_editor_fields_schema),
};

/**
 * Inits the ChordEditor after a Project has been
 * loaded.
 */
void
chord_editor_init_loaded (ChordEditor * self);

/**
 * Initializes the ChordEditor.
 */
void
chord_editor_init (ChordEditor * self);

ChordEditor *
chord_editor_clone (ChordEditor * src);

void
chord_editor_apply_single_chord (
  ChordEditor *           self,
  const ChordDescriptor * chord,
  const int               idx,
  bool                    undoable);

void
chord_editor_apply_chords (
  ChordEditor *            self,
  const ChordDescriptor ** chords,
  bool                     undoable);

void
chord_editor_apply_preset (
  ChordEditor * self,
  ChordPreset * pset,
  bool          undoable);

void
chord_editor_apply_preset_from_scale (
  ChordEditor *    self,
  MusicalScaleType scale,
  MusicalNote      root_note,
  bool             undoable);

void
chord_editor_transpose_chords (
  ChordEditor * self,
  bool          up,
  bool          undoable);

/**
 * Returns the ChordDescriptor for the given note
 * number, otherwise NULL if the given note number
 * is not in the proper range.
 */
NONNULL ChordDescriptor *
chord_editor_get_chord_from_note_number (
  const ChordEditor * self,
  midi_byte_t         note_number);

NONNULL int
chord_editor_get_chord_index (
  const ChordEditor *     self,
  const ChordDescriptor * chord);

ChordEditor *
chord_editor_new (void);

void
chord_editor_free (ChordEditor * self);

/**
 * @}
 */

#endif
