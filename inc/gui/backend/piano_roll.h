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

#ifndef __AUDIO_PIANO_ROLL_H__
#define __AUDIO_PIANO_ROLL_H__

#define PIANO_ROLL (&PROJECT->piano_roll)
#define PIANO_ROLL_SELECTED_TRACK (PIANO_ROLL->track)

typedef enum MidiModifier
{
  MIDI_MODIFIER_VELOCITY,
  MIDI_MODIFIER_PITCH_WHEEL,
  MIDI_MODIFIER_MOD_WHEEL,
  MIDI_MODIFIER_AFTERTOUCH,
} MidiModifier;

typedef struct Track Track;

/**
 * Piano roll serializable backend.
 *
 * The actual widgets should reflect the information here.
 */
typedef struct PianoRoll
{
  int                    notes_zoom; ///< notes zoom level
  MidiModifier           midi_modifier; ///< selected midi modifier

  /** Track currently attached to piano_roll. */
  int                      track_id;
  Track *                  track; ///< cache
} PianoRoll;

static const cyaml_strval_t
midi_modifier_strings[] =
{
	{ "Velocity",      MIDI_MODIFIER_VELOCITY    },
	{ "Pitch Wheel",   MIDI_MODIFIER_PITCH_WHEEL   },
	{ "Mod Wheel",     MIDI_MODIFIER_MOD_WHEEL   },
	{ "Aftertouch",    MIDI_MODIFIER_AFTERTOUCH   },
};

static const cyaml_schema_field_t
piano_roll_fields_schema[] =
{
	CYAML_FIELD_INT (
			"notes_zoom", CYAML_FLAG_DEFAULT,
			PianoRoll, notes_zoom),
  CYAML_FIELD_ENUM (
			"midi_modifier", CYAML_FLAG_DEFAULT,
			PianoRoll, midi_modifier, midi_modifier_strings,
      CYAML_ARRAY_LEN (midi_modifier_strings)),
	CYAML_FIELD_INT (
			"track_id", CYAML_FLAG_DEFAULT,
			PianoRoll, track_id),

	CYAML_FIELD_END
};

static const cyaml_schema_value_t
piano_roll_schema =
{
	CYAML_VALUE_MAPPING (
    CYAML_FLAG_POINTER,
		PianoRoll, piano_roll_fields_schema),
};

void
piano_roll_init (PianoRoll * self);

/**
 * Sets the track and refreshes the piano roll widgets.
 *
 * To be called only from GTK threads.
 */
void
piano_roll_set_track (Track * track);

#endif
