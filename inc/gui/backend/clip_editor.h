/*
 * Copyright (C) 2019-2021 Alexandros Theodotou <alex at zrythm dot org>
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
 * The clip/region editor backend.
 */

#ifndef __GUI_BACKEND_CLIP_EDITOR_H__
#define __GUI_BACKEND_CLIP_EDITOR_H__

#include <stdbool.h>

#include "audio/region_identifier.h"
#include "gui/backend/audio_clip_editor.h"
#include "gui/backend/automation_editor.h"
#include "gui/backend/chord_editor.h"
#include "gui/backend/piano_roll.h"

typedef struct ZRegion            ZRegion;
typedef struct ArrangerSelections ArrangerSelections;

/**
 * @addtogroup gui_backend
 *
 * @{
 */

#define CLIP_EDITOR_SCHEMA_VERSION 1

#define CLIP_EDITOR (PROJECT->clip_editor)

/**
 * Clip editor serializable backend.
 *
 * The actual widgets should reflect the
 * information here.
 */
typedef struct ClipEditor
{
  int schema_version;

  /** ZRegion currently attached to the clip
   * editor. */
  RegionIdentifier region_id;

  /** Whether \ref region_id is a valid region. */
  bool has_region;

  PianoRoll *        piano_roll;
  AudioClipEditor *  audio_clip_editor;
  AutomationEditor * automation_editor;
  ChordEditor *      chord_editor;

  /** Flag used by rulers when region first
   * changes. */
  int region_changed;

  /* --- caches --- */
  ZRegion * region;

  Track * track;
} ClipEditor;

static const cyaml_schema_field_t clip_editor_fields_schema[] = {
  YAML_FIELD_INT (ClipEditor, schema_version),
  YAML_FIELD_MAPPING_EMBEDDED (
    ClipEditor,
    region_id,
    region_identifier_fields_schema),
  YAML_FIELD_INT (ClipEditor, has_region),
  YAML_FIELD_MAPPING_PTR (
    ClipEditor,
    piano_roll,
    piano_roll_fields_schema),
  YAML_FIELD_MAPPING_PTR (
    ClipEditor,
    automation_editor,
    automation_editor_fields_schema),
  YAML_FIELD_MAPPING_PTR (
    ClipEditor,
    chord_editor,
    chord_editor_fields_schema),
  YAML_FIELD_MAPPING_PTR (
    ClipEditor,
    audio_clip_editor,
    audio_clip_editor_fields_schema),

  CYAML_FIELD_END
};

static const cyaml_schema_value_t clip_editor_schema = {
  YAML_VALUE_PTR (ClipEditor, clip_editor_fields_schema),
};

/**
 * Inits the ClipEditor after a Project is loaded.
 */
void
clip_editor_init_loaded (ClipEditor * self);

/**
 * Inits the clip editor.
 */
void
clip_editor_init (ClipEditor * self);

/**
 * Creates a new clip editor.
 */
ClipEditor *
clip_editor_new (void);

/**
 * Sets the track and refreshes the piano roll
 * widgets.
 *
 * To be called only from GTK threads.
 */
void
clip_editor_set_region (
  ClipEditor * self,
  ZRegion *    region,
  bool         fire_events);

ZRegion *
clip_editor_get_region (ClipEditor * self);

ArrangerSelections *
clip_editor_get_arranger_selections (ClipEditor * self);

#if 0
/**
 * Returns the ZRegion that widgets are expected
 * to use.
 */
ZRegion *
clip_editor_get_region_for_widgets (
  ClipEditor * self);
#endif

Track *
clip_editor_get_track (ClipEditor * self);

/**
 * To be called when recalculating the graph.
 */
void
clip_editor_set_caches (ClipEditor * self);

ClipEditor *
clip_editor_clone (const ClipEditor * src);

void
clip_editor_free (ClipEditor * self);

/**
 * @}
 */

#endif
