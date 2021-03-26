/*
 * Copyright (C) 2019-2021 Alexandros Theodotou <alex at zrythm dot org>
 *
 * This file is part of Zrythm
 *
 * Zrythm is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, version 3.
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
 * Chord editor backend.
 */

#ifndef __GUI_BACKEND_CHORD_EDITOR_H__
#define __GUI_BACKEND_CHORD_EDITOR_H__

#include "audio/chord_descriptor.h"
#include "gui/backend/editor_settings.h"

#include <cyaml/cyaml.h>

typedef struct ChordDescriptor ChordDescriptor;

/**
 * @addtogroup gui_backend
 *
 * @{
 */

#define CHORD_EDITOR_SCHEMA_VERSION 1

#define CHORD_EDITOR \
  (&CLIP_EDITOR->chord_editor)

/**
 * Backend for the chord editor.
 */
typedef struct ChordEditor
{
  int              schema_version;

  /**
   * The chords to show on the left.
   *
   * Currently fixed to 12 chords whose order cannot
   * be edited. Chords cannot be added or removed.
   */
  ChordDescriptor * chords[128];
  int               num_chords;

  EditorSettings  editor_settings;
} ChordEditor;

static const cyaml_schema_field_t
chord_editor_fields_schema[] =
{
  YAML_FIELD_INT (
    ChordEditor, schema_version),
  YAML_FIELD_FIXED_SIZE_PTR_ARRAY_VAR_COUNT (
    ChordEditor, chords, chord_descriptor_schema),
  YAML_FIELD_MAPPING_EMBEDDED (
    ChordEditor, editor_settings,
    editor_settings_fields_schema),

  CYAML_FIELD_END
};

static const cyaml_schema_value_t
chord_editor_schema =
{
  CYAML_VALUE_MAPPING (
    CYAML_FLAG_POINTER,
    ChordEditor, chord_editor_fields_schema),
};

/**
 * Inits the ChordEditor after a Project has been
 * loaded.
 */
void
chord_editor_init_loaded (
  ChordEditor * self);

/**
 * Initializes the ChordEditor.
 */
void
chord_editor_init (
  ChordEditor * self);

/**
 * @}
 */

#endif
