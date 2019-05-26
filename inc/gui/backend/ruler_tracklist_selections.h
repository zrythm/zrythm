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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

/**
 * \file
 *
 * Ruler tracklist selections.
 */

#ifndef __GUI_BACKEND_RULER_TRACKLIST_SELECTIONS_H__
#define __GUI_BACKEND_RULER_TRACKLIST_SELECTIONS_H__

#include "audio/chord.h"
#include "utils/yaml.h"

#define RT_SELECTIONS \
  (&PROJECT->ruler_tracklist_selections)

/**
 * Selections to be used for the RulerTracklist's
 * current selections, copying, undoing, etc.
 */
typedef struct RulerTracklistSelections
{
  /** Chords acting upon */
  ZChord *                 chords[800];
  ZChord *                 transient_chords[800];
  int                      num_chords;
} RulerTracklistSelections;

static const cyaml_schema_field_t
  ruler_tracklist_selections_fields_schema[] =
{
  CYAML_FIELD_SEQUENCE_COUNT (
    "chords", CYAML_FLAG_DEFAULT,
    RulerTracklistSelections, chords, num_chords,
    &chord_schema, 0, CYAML_UNLIMITED),

	CYAML_FIELD_END
};

static const cyaml_schema_value_t
ruler_tracklist_selections_schema = {
  CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER,
    RulerTracklistSelections,
    ruler_tracklist_selections_fields_schema),
};

void
ruler_tracklist_selections_init_loaded (
  RulerTracklistSelections * ts);

/**
 * Creates transient objects for objects added
 * to selections without transients.
 */
void
ruler_tracklist_selections_create_missing_transients (
  RulerTracklistSelections * ts);

/**
 * Clone the struct for copying, undoing, etc.
 */
RulerTracklistSelections *
ruler_tracklist_selections_clone (
  RulerTracklistSelections * src);

/**
 * Returns if there are any selections.
 */
int
ruler_tracklist_selections_has_any (
  RulerTracklistSelections * ts);

/**
 * Returns the position of the leftmost object.
 *
 * If transient is 1, the transient objects are
 * checked instead.
 *
 * The return value will be stored in pos.
 */
void
ruler_tracklist_selections_get_start_pos (
  RulerTracklistSelections * ts,
  Position *           pos,
  int                  transient);

/**
 * Returns the position of the rightmost object.
 *
 * If transient is 1, the transient objects are
 * checked instead.
 *
 * The return value will be stored in pos.
 */
void
ruler_tracklist_selections_get_end_pos (
  RulerTracklistSelections * ts,
  Position *           pos,
  int                  transient);

/**
 * Gets first object's widget.
 *
 * If transient is 1, transient objects rae checked
 * instead.
 */
GtkWidget *
ruler_tracklist_selections_get_first_object (
  RulerTracklistSelections * ts,
  int                  transient);

/**
 * Gets last object's widget.
 *
 * If transient is 1, transient objects rae checked
 * instead.
 */
GtkWidget *
ruler_tracklist_selections_get_last_object (
  RulerTracklistSelections * ts,
  int                  transient);

/**
 * Gets highest track in the selections.
 *
 * If transient is 1, transient objects rae checked
 * instead.
 */
Track *
ruler_tracklist_selections_get_highest_track (
  RulerTracklistSelections * ts,
  int                  transient);

/**
 * Gets lowest track in the selections.
 *
 * If transient is 1, transient objects rae checked
 * instead.
 */
Track *
ruler_tracklist_selections_get_lowest_track (
  RulerTracklistSelections * ts,
  int                  transient);

void
ruler_tracklist_selections_paste_to_pos (
  RulerTracklistSelections * ts,
  Position *           pos);

/**
 * Only removes transients from their tracks and
 * frees them.
 */
void
ruler_tracklist_selections_remove_transients (
  RulerTracklistSelections * ts);

void
ruler_tracklist_selections_add_chord (
  RulerTracklistSelections * ts,
  ZChord *              c,
  int                  transient);


void
ruler_tracklist_selections_remove_chord (
  RulerTracklistSelections * ts,
  ZChord *              c);

/**
 * Clears selections.
 */
void
ruler_tracklist_selections_clear (
  RulerTracklistSelections * ts);

void
ruler_tracklist_selections_free (RulerTracklistSelections * self);

SERIALIZE_INC (RulerTracklistSelections, ruler_tracklist_selections)
DESERIALIZE_INC (RulerTracklistSelections, ruler_tracklist_selections)
PRINT_YAML_INC (RulerTracklistSelections, ruler_tracklist_selections)

#endif
