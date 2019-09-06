/*
 * Copyright (C) 2018-2019 Alexandros Theodotou <alex at zrythm dot org>
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

#include "audio/channel.h"
#include "audio/midi.h"
#include "audio/midi_note.h"
#include "audio/midi_region.h"
#include "audio/region.h"
#include "audio/track.h"
#include "gui/widgets/bot_dock_edge.h"
#include "gui/widgets/center_dock.h"
#include "gui/widgets/clip_editor.h"
#include "gui/widgets/clip_editor_inner.h"
#include "gui/widgets/main_window.h"
#include "gui/widgets/midi_arranger.h"
#include "gui/widgets/midi_editor_space.h"
#include "gui/widgets/midi_region.h"
#include "gui/widgets/region.h"
#include "project.h"
#include "utils/arrays.h"
#include "utils/objects.h"
#include "utils/yaml.h"

#include "ext/midilib/src/midifile.h"

MidiRegion *
midi_region_new (
  const Position * start_pos,
  const Position * end_pos,
  int        is_main)
{
  MidiRegion * self =
    calloc (1, sizeof (MidiRegion));

  self->type = REGION_TYPE_MIDI;

  region_init (
    self, start_pos, end_pos, is_main);

   return self;
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
 */
void
midi_region_add_midi_note (
  MidiRegion * region,
  MidiNote * midi_note)
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

  array_double_size_if_full (
    region->midi_notes, region->num_midi_notes,
    region->midi_notes_size, MidiNote *)
  array_append (region->midi_notes,
                region->num_midi_notes,
                midi_note);

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
  if (MW_MIDI_ARRANGER->start_midi_note ==
        midi_note)
    MW_MIDI_ARRANGER->start_midi_note = NULL;

  array_delete (region->midi_notes,
                region->num_midi_notes,
                midi_note);
  if (free)
    free_later (midi_note, midi_note_free_all);

  if (pub_event)
    EVENTS_PUSH (ET_MIDI_NOTE_REMOVED, NULL);
}

/**
 * Exports the Region to an existing MIDI file
 * instance.
 *
 * @param add_region_start Add the region start
 *   offset to the positions.
 * @param export_full Traverse loops and export the
 *   MIDI file as it would be played inside Zrythm.
 *   If this is 0, only the original region (from
 *   true start to true end) is exported.
 */
void
midi_region_write_to_midi_file (
  const Region * self,
  MIDI_FILE *    mf,
  const int      add_region_start,
  const int      export_full)
{
  MidiEvents * events =
    midi_region_get_as_events (
      self, add_region_start, export_full);
  MidiEvent * ev;
  int i;
  Track * track = region_get_track (self);
  for (i = 0; i < events->num_events; i++)
    {
      ev = &events->events[i];

      BYTE tmp[] =
        { ev->raw_buffer[0],
          ev->raw_buffer[1],
        ev->raw_buffer[2] };
      midiTrackAddRaw (
        mf, track->pos, 3, tmp, 1,
        i == 0 ?
          ev->time :
          ev->time - events->events[i - 1].time);
    }
  midi_events_free (events);
}

/**
 * Exports the Region to a specified MIDI file.
 *
 * @param full_path Absolute path to the MIDI file.
 * @param export_full Traverse loops and export the
 *   MIDI file as it would be played inside Zrythm.
 *   If this is 0, only the original region (from
 *   true start to true end) is exported.
 */
void
midi_region_export_to_midi_file (
  const Region * self,
  const char *   full_path,
  int            midi_version,
  const int      export_full)
{
  MIDI_FILE *mf;

  int i;
	if ((mf = midiFileCreate(full_path, TRUE)))
		{
      /* Write tempo information out to track 1 */
      midiSongAddTempo(mf, 1, TRANSPORT->bpm);

      /* All data is written out to _tracks_ not
       * channels. We therefore
      ** set the current channel before writing
      data out. Channel assignments
      ** can change any number of times during the
      file, and affect all
      ** tracks messages until it is changed. */
      midiFileSetTracksDefaultChannel (
        mf, 1, MIDI_CHANNEL_1);

      midiFileSetPPQN (mf, TICKS_PER_QUARTER_NOTE);

      midiFileSetVersion (mf, midi_version);

      /* common time: 4 crochet beats, per bar */
      midiSongAddSimpleTimeSig (
        mf, 1, TRANSPORT->beats_per_bar,
        TRANSPORT->ticks_per_beat);

      midi_region_write_to_midi_file (
        self, mf, 0, export_full);

      midiFileClose(mf);
    }
}

/**
 * Returns a newly initialized MidiEvents with
 * the contents of the region converted into
 * events.
 *
 * Must be free'd with midi_events_free ().
 *
 * @param add_region_start Add the region start
 *   offset to the positions.
 * @param export_full Traverse loops and export the
 *   MIDI file as it would be played inside Zrythm.
 *   If this is 0, only the original region (from
 *   true start to true end) is exported.
 */
MidiEvents *
midi_region_get_as_events (
  const Region * self,
  const int      add_region_start,
  const int      full)
{
  MidiEvents * events =
    midi_events_new (NULL);

  long region_start = 0;
  if (add_region_start)
    region_start =
      position_to_ticks (&self->start_pos);

  MidiNote * mn;
  for (int i = 0; i < self->num_midi_notes; i++)
    {
      mn = self->midi_notes[i];
      midi_events_add_note_on (
        events, 1, mn->val, mn->vel->vel,
        position_to_ticks (&mn->start_pos) +
          region_start,
        0);
      midi_events_add_note_off (
        events, 1, mn->val,
        position_to_ticks (&mn->end_pos) +
          region_start,
        0);
    }

  midi_events_sort_by_time (
    events);

  return events;
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
