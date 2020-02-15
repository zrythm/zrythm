/*
 * Copyright (C) 2019 Alexandros Theodotou <alex at zrythm dot org>
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

#include "audio/region_identifier.h"
#include "gui/backend/audio_clip_editor.h"
#include "gui/backend/automation_editor.h"
#include "gui/backend/chord_editor.h"
#include "gui/backend/piano_roll.h"

/**
 * @addtogroup gui_backend
 *
 * @{
 */

#define CLIP_EDITOR (&PROJECT->clip_editor)

typedef struct ZRegion ZRegion;

/**
 * Clip editor serializable backend.
 *
 * The actual widgets should reflect the
 * information here.
 */
typedef struct ClipEditor
{
  /** ZRegion currently attached to the clip
   * editor. */
  RegionIdentifier  region_id;

  /**
   * This will be set to the same as above after
   * the arrangers are switched.
   *
   * Related widgets should use this.
   */
  RegionIdentifier  region_id_cache;

  /** Whether \ref region_id is a valid region. */
  int              has_region;

  /**
   * Whether \ref region_id_cache is a valid
   * region.
   *
   * FIXME explain when this should be set.
   */
  int              had_region;

  PianoRoll        piano_roll;
  AudioClipEditor  audio_clip_editor;
  AutomationEditor automation_editor;
  ChordEditor      chord_editor;

  /** Flag used by rulers when region first
   * changes. */
  int              region_changed;
} ClipEditor;

static const cyaml_schema_field_t
clip_editor_fields_schema[] =
{
  CYAML_FIELD_MAPPING (
    "region_id", CYAML_FLAG_DEFAULT,
    ClipEditor, region_id,
    region_identifier_fields_schema),
  CYAML_FIELD_MAPPING (
    "piano_roll", CYAML_FLAG_DEFAULT,
    ClipEditor, piano_roll,
    piano_roll_fields_schema),
  CYAML_FIELD_MAPPING (
    "automation_editor", CYAML_FLAG_DEFAULT,
    ClipEditor, automation_editor,
    automation_editor_fields_schema),
  CYAML_FIELD_MAPPING (
    "chord_editor", CYAML_FLAG_DEFAULT,
    ClipEditor, chord_editor,
    chord_editor_fields_schema),

  CYAML_FIELD_END
};

static const cyaml_schema_value_t
clip_editor_schema =
{
  CYAML_VALUE_MAPPING (
    CYAML_FLAG_POINTER,
    ClipEditor, clip_editor_fields_schema),
};

/**
 * Inits the ClipEditor after a Project is loaded.
 */
void
clip_editor_init_loaded (
  ClipEditor * self);

void
clip_editor_init (
  ClipEditor * self);

/**
 * Sets the track and refreshes the piano roll widgets.
 *
 * To be called only from GTK threads.
 */
void
clip_editor_set_region (
  ClipEditor * self,
  ZRegion *     region);

ZRegion *
clip_editor_get_region (
  ClipEditor * self);

/**
 * Returns the ZRegion that widgets are expected
 * to use.
 */
ZRegion *
clip_editor_get_region_for_widgets (
  ClipEditor * self);

/**
 * Causes the selected ZRegion to be redrawin in the
 * UI, if any.
 */
void
clip_editor_redraw_region (
  ClipEditor * self);

/**
 * @}
 */

#endif
