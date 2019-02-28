/*
 * Copyright (C) 2019 Alexandros Theodotou <alex at zrythm dot org>
 *
 * This file is part of Zrythm
 *
 * Zrythm is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Zrythm is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Zrythm.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef __AUDIO_CLIP_EDITOR_H__
#define __AUDIO_CLIP_EDITOR_H__

#include "gui/backend/piano_roll.h"
#include "gui/backend/audio_clip_editor.h"

#define CLIP_EDITOR (&PROJECT->clip_editor)
#define CLIP_EDITOR_SELECTED_REGION (CLIP_EDITOR->region)

typedef struct Region Region;

/**
 * Piano roll serializable backend.
 *
 * The actual widgets should reflect the information here.
 */
typedef struct ClipEditor
{
  /** Region currently attached to the clip editor. */
  int                      region_id;
  Region *                 region; ///< cache

  PianoRoll                piano_roll;
  AudioClipEditor          audio_clip_editor;
} ClipEditor;

static const cyaml_schema_field_t
clip_editor_fields_schema[] =
{
	CYAML_FIELD_INT (
			"region_id", CYAML_FLAG_DEFAULT,
			ClipEditor, region_id),

	CYAML_FIELD_END
};

static const cyaml_schema_value_t
clip_editor_schema =
{
	CYAML_VALUE_MAPPING (
    CYAML_FLAG_POINTER,
		ClipEditor, clip_editor_fields_schema),
};

void
clip_editor_init (ClipEditor * self);

/**
 * Sets the track and refreshes the piano roll widgets.
 *
 * To be called only from GTK threads.
 */
void
clip_editor_set_region (Region * region);

#endif

