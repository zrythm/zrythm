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

#include <stdlib.h>

#include "audio/chord.h"
#include "audio/chord_track.h"
#include "audio/position.h"
#include "gui/widgets/chord.h"
#include "project.h"

NOTE_LABELS;

DEFINE_START_POS

#define SET_POS(_c,_pos) \
  POSITION_SET_ARRANGER_OBJ_POS ( \
    chord, _c, pos, _pos)

void
chord_init_loaded (ZChord * self)
{
  self->widget = chord_widget_new (self);
}

void
chord_set_pos (ZChord *    self,
               Position * pos)
{
  position_set_to_pos (&self->pos, pos);
}

/**
 * Creates a chord.
 */
ZChord *
chord_new (MusicalNote            root,
           uint8_t                has_bass,
           MusicalNote            bass,
           ChordType              type,
           int                    inversion)
{
  ZChord * self = calloc (1, sizeof (ZChord));

  self->root_note = root;
  self->has_bass = has_bass;
  if (has_bass)
    self->bass_note = bass;
  self->type = type;
  self->inversion = inversion;

  /* add bass note */
  if (has_bass)
    {
      self->notes[bass] = 1;
    }

  /* add root note */
  self->notes[12 + root] = 1;

  /* add rest of chord notes */
  switch (type)
    {
    case CHORD_TYPE_MAJ:
      self->notes[12 + root + 4] = 1;
      self->notes[12 + root + 4 + 3] = 1;
      break;
    case CHORD_TYPE_MIN:
      self->notes[12 + root + 3] = 1;
      self->notes[12 + root + 3 + 4] = 1;
      break;
    case CHORD_TYPE_DIM:
      self->notes[12 + root + 3] = 1;
      self->notes[12 + root + 3 + 3] = 1;
      break;
    case CHORD_TYPE_AUG:
      self->notes[12 + root + 4] = 1;
      self->notes[12 + root + 4 + 4] = 1;
      break;
    case CHORD_TYPE_SUS2:
      self->notes[12 + root + 2] = 1;
      self->notes[12 + root + 2 + 5] = 1;
      break;
    case CHORD_TYPE_SUS4:
      self->notes[12 + root + 5] = 1;
      self->notes[12 + root + 5 + 2] = 1;
      break;
    default:
      g_warning ("chord unimplemented");
      break;
    }

  /* TODO invert */

  self->widget = chord_widget_new (self);

  self->visible = 1;

  return self;
}

/**
 * Finds the chord in the project corresponding to the
 * given one.
 */
ZChord *
chord_find (
  ZChord * clone)
{
  for (int i = 0;
       i < P_CHORD_TRACK->num_chords; i++)
    {
      if (chord_is_equal (
            P_CHORD_TRACK->chords[i],
            clone))
        return P_CHORD_TRACK->chords[i];
    }
  return NULL;
}

/**
 * Moves the ZChord by the given amount of ticks.
 *
 * @param use_cached_pos Add the ticks to the cached
 *   Position instead of its current Position.
 * @return Whether moved or not.
 */
int
chord_move (
  ZChord * chord,
  long     ticks,
  int      use_cached_pos)
{
  Position tmp;
  if (use_cached_pos)
    position_set_to_pos (
      &tmp, &chord->cache_pos);
  else
    position_set_to_pos (
      &tmp, &chord->pos);
  position_add_ticks (
    &tmp, ticks);
  if (position_is_before (
        &tmp, START_POS))
    return 0;

  SET_POS (chord, &tmp);

  return 1;
}

/**
 * Returns the Track this ZChord is in.
 */
Track *
chord_get_track (
  ZChord * self)
{
  return TRACKLIST->tracks[self->track_pos];
}

/**
 * Returns the musical note as a string (eg. "C3").
 */
const char *
chord_note_to_string (MusicalNote note)
{
  (void) note_labels;
  return note_labels[note];
}

void
chord_free (ZChord * self)
{
  free (self);
}
