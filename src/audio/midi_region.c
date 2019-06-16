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
#include "utils/objects.h"
#include "utils/yaml.h"

MidiRegion *
midi_region_new (
  const Position * start_pos,
  const Position * end_pos,
  int        is_main)
{
  MidiRegion * midi_region =
    calloc (1, sizeof (MidiRegion));

  region_init ((Region *) midi_region,
               REGION_TYPE_MIDI,
               start_pos,
               end_pos,
               is_main);

  return midi_region;
}

/**
 * Prints the MidiNotes in the Region.
 *
 * Used for debugging.
 */
void
midi_region_print_midi_notes (
  Region * self)
{
  MidiNote * mn;
  for (int i = 0; i < self->num_midi_notes; i++)
    {
      mn = self->midi_notes[i];

      g_message ("Note at %d",
                 i);
      midi_note_print (mn);
    }
}


/**
 * Adds the MidiNote to the given Region.
 *
 * @param gen_widget Generate a MidiNoteWidget.
 */
void
midi_region_add_midi_note (
  MidiRegion * region,
  MidiNote * midi_note,
  int        gen_widget)
{
  g_warn_if_fail (
    midi_note->obj_info.counterpart ==
    AOI_COUNTERPART_MAIN);
  g_warn_if_fail (
    midi_note->obj_info.main &&
    midi_note->obj_info.main_trans &&
    midi_note->obj_info.lane &&
    midi_note->obj_info.lane_trans);

  midi_note_set_region (midi_note, region);

  array_append (region->midi_notes,
                region->num_midi_notes,
                midi_note);

  if (gen_widget)
    midi_note_gen_widget (midi_note);

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
 * Removes the MIDI note from the Region.
 *
 * @param free Also free the MidiNote.
 * @param pub_event Publish an event.
 */
void
midi_region_remove_midi_note (
  Region *   region,
  MidiNote * midi_note,
  int        free,
  int        pub_event)
{
  if (MA_SELECTIONS)
    {
      midi_arranger_selections_remove_midi_note (
        MA_SELECTIONS,
        midi_note);
    }
  if (MIDI_ARRANGER->start_midi_note ==
        midi_note)
    MIDI_ARRANGER->start_midi_note = NULL;

  array_delete (region->midi_notes,
                region->num_midi_notes,
                midi_note);
  if (free)
    free_later (midi_note, midi_note_free_all);

  if (pub_event)
    EVENTS_PUSH (ET_MIDI_NOTE_REMOVED, NULL);
}

/**
 * Frees members only but not the MidiRegion
 * itself.
 *
 * Regions should be free'd using region_free.
 */
void
midi_region_free_members (MidiRegion * self)
{
  for (int i = 0; i < self->num_midi_notes; i++)
    {
      midi_note_free_all (self->midi_notes[i]);
    }
  for (int i = 0; i < self->num_unended_notes; i++)
    {
      midi_note_free_all (self->unended_notes[i]);
    }
}
