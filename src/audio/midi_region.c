/*
 * Copyright (C) 2018-2019 Alexandros Theodotou <alex at zrythm dot org>
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

#include "audio/channel.h"
#include "audio/midi_note.h"
#include "audio/midi_region.h"
#include "audio/region.h"
#include "audio/track.h"
#include "gui/widgets/bot_dock_edge.h"
#include "gui/widgets/center_dock.h"
#include "gui/widgets/clip_editor.h"
#include "gui/widgets/main_window.h"
#include "gui/widgets/midi_arranger.h"
#include "gui/widgets/midi_region.h"
#include "gui/widgets/region.h"
#include "project.h"
#include "utils/arrays.h"
#include "utils/yaml.h"

MidiRegion *
midi_region_new (Track *    track,
                 Position * start_pos,
                 Position * end_pos)
{
  MidiRegion * midi_region =
    calloc (1, sizeof (MidiRegion));

  region_init ((Region *) midi_region,
               REGION_TYPE_MIDI,
               track,
               start_pos,
               end_pos);

  return midi_region;
}

/**
 * Adds midi note to region
 */
void
midi_region_add_midi_note (MidiRegion * region,
                      MidiNote * midi_note)
{
  array_append (region->midi_notes,
                region->num_midi_notes,
                midi_note);
  region->midi_note_ids[
    region->num_midi_notes - 1] =
      region->midi_notes[
        region->num_midi_notes - 1]->id;
  midi_note->region = region;

  EVENTS_PUSH (ET_MIDI_NOTE_CREATED,
               midi_note);
}

/**
 * Returns the midi note with the given pitch from the
 * unended notes.
 *
 * Used when recording.
 */
MidiNote *
midi_region_find_unended_note (MidiRegion * self,
                               int          pitch)
{
  for (int i = 0; i < self->num_unended_notes; i++)
    {
      MidiNote * mn = self->unended_notes[i];
      if (mn->val == pitch)
        return mn;
    }
  g_warn_if_reached ();
  return NULL;
}

/**
 * this method takes a midi_note clone as parameter to
 * change the value  * of it's origin midi_note
 * on the given midi_region to the clone value. the
 * midi_note is matched by id.
 */
void
midi_region_update_midi_note_val (
	MidiRegion * region,
	MidiNote * midi_note)
{
	for (int i = 0;
		i < region->num_midi_notes;
		i++)
	{
		if (region->midi_notes[i]->id == midi_note->id)
		{
			region->midi_notes[i]->val =
				midi_note->val;
		}
	}
}

/**
 * Gets first midi note
 */
MidiNote *
midi_region_get_first_midi_note (MidiRegion * region)
{
	MidiNote * result = 0;
	for (int i = 0;
		i < region->num_midi_notes;
		i++)
	{
		if (result == 0
			|| position_to_ticks(&result->end_pos) >
		position_to_ticks(&region->midi_notes[i]->end_pos))
		{
			result = region->midi_notes[i];
		}
	}
	return result;
}
/**
 * Gets last midi note
 */
MidiNote *
midi_region_get_last_midi_note (MidiRegion * region)
{
	MidiNote * result = 0;
	for (int i = 0;
		i < region->num_midi_notes;
		i++)
	{
		if (result == 0
			|| position_to_ticks(&result->end_pos) <
			position_to_ticks(&region->midi_notes[i]->end_pos))
		{
			result = region->midi_notes[i];
		}
	}
	return result;
}

/**
 * Gets highest midi note
 */
MidiNote *
midi_region_get_highest_midi_note (MidiRegion * region)
{
	MidiNote * result = 0;
	for (int i = 0;
		i < region->num_midi_notes;
		i++)
	{
		if (result == 0
			|| result->val < region->midi_notes[i]->val)
		{
			result = region->midi_notes[i];
		}
	}
	return result;
}

/**
 * Gets lowest midi note
 */
MidiNote *
midi_region_get_lowest_midi_note (MidiRegion * region)
{
	MidiNote * result = 0;
	for (int i = 0;
		i < region->num_midi_notes;
		i++)
	{
		if (result == 0
			|| result->val > region->midi_notes[i]->val)
		{
			result = region->midi_notes[i];
		}
	}

	return result;
}
/**
 * Removes the MIDI note and its component
 * completely.
 */
void
midi_region_add_midi_note_if_not_present (
  MidiRegion * region,
  MidiNote * midi_note)
{
  for (int i = 0; i < region->num_midi_notes; i++)
  {
    if (region->midi_notes[i]->id == midi_note->id)
    {
      return;
    }
  }
  midi_region_add_midi_note (region, midi_note);
}

/**
 * Removes the MIDI note and its components
 * completely.
 */
void
midi_region_remove_midi_note (
  Region *   region,
  MidiNote * midi_note)
{
  if (MIDI_ARRANGER_SELECTIONS)
    {
      midi_arranger_selections_remove_note (
        MIDI_ARRANGER_SELECTIONS,
        midi_note);
    }
  if (MIDI_ARRANGER->start_midi_note ==
        midi_note)
    MIDI_ARRANGER->start_midi_note = NULL;

  array_delete (region->midi_note_ids,
                region->num_midi_notes,
                midi_note->id);
  int size = region->num_midi_notes + 1;
  array_delete (region->midi_notes,
                size,
                midi_note);

  project_remove_midi_note (midi_note);
  midi_note_free (midi_note);
}

/**
 * Frees members only but not the midi region itself.
 *
 * Regions should be free'd using region_free.
 */
void
midi_region_free_members (MidiRegion * self)
{
  for (int i = 0; i < self->num_midi_notes; i++)
    {
      midi_note_free (self->midi_notes[i]);
    }
  for (int i = 0; i < self->num_unended_notes; i++)
    {
      midi_note_free (self->unended_notes[i]);
    }
}
