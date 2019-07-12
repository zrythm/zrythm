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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "audio/engine.h"
#include "audio/position.h"
#include "audio/track.h"
#include "audio/transport.h"
#include "gui/backend/midi_arranger_selections.h"
#include "gui/widgets/midi_note.h"
#include "project.h"
#include "utils/arrays.h"
#include "utils/flags.h"
#include "utils/yaml.h"

#include <gtk/gtk.h>

void
midi_arranger_selections_init_loaded (
  MidiArrangerSelections * self)
{
  int i;
  MidiNote * mn;
  for (i = 0; i < self->num_midi_notes; i++)
    {
      mn = self->midi_notes[i];
      mn->region =
        region_find_by_name (mn->region_name);
      g_warn_if_fail (mn->region);
      self->midi_notes[i] =
        midi_note_find (mn);
      midi_note_free (mn);
    }
}

/**
 * Returns if there are any selections.
 */
int
midi_arranger_selections_has_any (
  MidiArrangerSelections * mas)
{
  return mas->num_midi_notes > 0;
}

/**
 * Returns the position of the leftmost object.
 *
 * If transient is 1, the transient objects are
 * checked instead.
 *
 * The return value will be stored in pos.
 *
 * @param global Return global (timeline) Position,
 *   otherwise returns the local (from the start
 *   of the Region) Position.
 */
void
midi_arranger_selections_get_start_pos (
  MidiArrangerSelections * mas,
  Position *           pos,
  int                  transient,
  int                  global)
{
  position_set_to_bar (pos,
                       TRANSPORT->total_bars);

  GtkWidget * widget = NULL;
  (void) widget; // avoid unused warnings

  int i;

  ARRANGER_OBJ_SET_GIVEN_POS_TO (
    mas, MidiNote, midi_note, start_pos,
    transient, before, widget);

  if (global)
    position_add_ticks (
      pos,
      mas->midi_notes[0]->region->
        start_pos.total_ticks);
}

/**
 * Returns the position of the rightmost object.
 *
 * If transient is 1, the transient objects are
 * checked instead.
 *
 * The return value will be stored in pos.
 */
void
midi_arranger_selections_get_end_pos (
  MidiArrangerSelections * mas,
  Position *           pos,
  int                  transient)
{
  position_set_to_bar (pos,
                       TRANSPORT->total_bars);

  GtkWidget * widget = NULL;
  (void) widget; // avoid unused warnings

  int i;

  ARRANGER_OBJ_SET_GIVEN_POS_TO (
    mas, MidiNote, midi_note, start_pos,
    transient, after, widget);
}

/**
 * Gets first object's widget.
 *
 * If transient is 1, transient objects are checked
 * instead.
 */
GtkWidget *
midi_arranger_selections_get_first_object (
  MidiArrangerSelections * mas,
  int                  transient)
{
  Position _pos;
  Position * pos = &_pos;
  GtkWidget * widget = NULL;
  position_set_to_bar (
    pos, TRANSPORT->total_bars);
  int i;

  ARRANGER_OBJ_SET_GIVEN_POS_TO (
    mas, MidiNote, midi_note, start_pos,
    transient, before, widget);

  return widget;
}

/**
 * Gets last object's widget.
 *
 * If transient is 1, transient objects are checked
 * instead.
 */
GtkWidget *
midi_arranger_selections_get_last_object (
  MidiArrangerSelections * mas,
  int                  transient)
{
  Position _pos;
  Position * pos = &_pos;
  GtkWidget * widget = NULL;
  position_set_to_bar (
    pos, TRANSPORT->total_bars);
  int i;

  ARRANGER_OBJ_SET_GIVEN_POS_TO (
    mas, MidiNote, midi_note, start_pos,
    transient, after, widget);

  return widget;
}

/**
 * Sets the cache Position's for each object in
 * the selection.
 *
 * Used by the ArrangerWidget's.
 */
void
midi_arranger_selections_set_cache_poses (
  MidiArrangerSelections * mas)
{
  int i;

  MidiNote * mn;
  for (i = 0; i < mas->num_midi_notes; i++)
    {
      mn = mas->midi_notes[i];

      /* set start pos for midi note */
      midi_note_set_cache_start_pos (
        mn, &mn->start_pos);

      /* set end pos for midi note */
      midi_note_set_cache_end_pos (
        mn, &mn->end_pos);

      midi_note_set_cache_val (
        mn, mn->val);

      /* set cache velocity */
      velocity_set_cache_vel (
        mn->vel, mn->vel->vel);
    }
}

/**
 * Clears selections.
 */
void
midi_arranger_selections_clear (
  MidiArrangerSelections * mas)
{
  int i, num_midi_notes = mas->num_midi_notes;;
  MidiNote * mn;

  /* use caches because mas->* will be operated on */
  static MidiNote * midi_notes[600];
  for (i = 0; i < num_midi_notes; i++)
    {
      midi_notes[i] = mas->midi_notes[i];
    }
  for (i = 0; i < num_midi_notes; i++)
    {
      mn = midi_notes[i];
      midi_arranger_selections_remove_midi_note (
        mas, mn);
      EVENTS_PUSH (ET_MIDI_NOTE_CHANGED,
                   mn);
    }

  g_message ("cleared midi arranger selections");
}

/**
 * Returns if the MidiArrangerSelections contain
 * the given MidiNote.
 *
 * The note must be the main note (see
 * midi_note_get_main_midi_note()).
 */
int
midi_arranger_selections_contains_midi_note (
  MidiArrangerSelections * mas,
  MidiNote *               note)
{
  return array_contains (
    mas->midi_notes,
    mas->num_midi_notes,
    midi_note_get_main_midi_note (note));
}

/**
 * Adds a note to the selections.
 */
void
midi_arranger_selections_add_midi_note (
  MidiArrangerSelections * mas,
  MidiNote *               note)
{
  if (!array_contains (mas->midi_notes,
                      mas->num_midi_notes,
                      note))
    {
      array_append (mas->midi_notes,
                    mas->num_midi_notes,
                    note);

      EVENTS_PUSH (ET_MIDI_NOTE_CHANGED,
                   note);
    }
}

void
midi_arranger_selections_remove_midi_note (
  MidiArrangerSelections * mas,
  MidiNote *               note)
{
  if (!array_contains (mas->midi_notes,
                       mas->num_midi_notes,
                       note))
    {
      return;
    }

  array_delete (
    mas->midi_notes,
    mas->num_midi_notes,
    note);
}

/**
 * Clone the struct for copying, undoing, etc.
 */
MidiArrangerSelections *
midi_arranger_selections_clone (
  MidiArrangerSelections * src)
{
  MidiArrangerSelections * new_ts =
    calloc (1, sizeof (MidiArrangerSelections));

  for (int i = 0; i < src->num_midi_notes; i++)
    {
      MidiNote * r = src->midi_notes[i];
      MidiNote * new_r =
        midi_note_clone (
          r, MIDI_NOTE_CLONE_COPY);
      array_append (new_ts->midi_notes,
                    new_ts->num_midi_notes,
                    new_r);
    }
  return new_ts;
}

MidiNote *
midi_arranger_selections_get_highest_note (
  MidiArrangerSelections * mas,
  int                      transient)
{
  MidiNote * top_mn;
  if (transient)
    top_mn = mas->midi_notes[0]->obj_info.main_trans;
  else
    top_mn = mas->midi_notes[0]->obj_info.main;
  MidiNote * tmp;
  for (int i = 0;
       i < mas->num_midi_notes;
       i++)
    {
      if (transient)
        tmp = mas->midi_notes[i]->obj_info.main_trans;
      else
        tmp = mas->midi_notes[i]->obj_info.main;
      if (tmp->val >
            top_mn->val)
        {
          top_mn = tmp;
        }
    }
  return top_mn;
}

MidiNote *
midi_arranger_selections_get_lowest_note (
  MidiArrangerSelections * mas,
  int                      transient)
{

  MidiNote * bot_mn;
  if (transient)
    bot_mn = mas->midi_notes[0]->obj_info.main_trans;
  else
    bot_mn = mas->midi_notes[0]->obj_info.main;
  MidiNote * tmp;
  for (int i = 0;
       i < mas->num_midi_notes;
       i++)
    {
      if (transient)
        tmp = mas->midi_notes[i]->obj_info.main_trans;
      else
        tmp = mas->midi_notes[i]->obj_info.main;
      if (tmp->val <
            bot_mn->val)
        {
          bot_mn = tmp;
        }
    }
  return bot_mn;
}

/**
 * Gets first (position-wise) MidiNote.
 *
 * If transient is 1, the transient notes are
 * checked instead.
 */
MidiNote *
midi_arranger_selections_get_first_midi_note (
  MidiArrangerSelections * mas,
  int                      transient)
{
	MidiNote * result = NULL;
  MidiNote * tmp = NULL;
	for (int i = 0;
		i < mas->num_midi_notes;
		i++)
    {
      if (transient)
        tmp = mas->midi_notes[i]->obj_info.main_trans;
      else
        tmp = mas->midi_notes[i]->obj_info.main;
      if (!result ||
          position_to_ticks(&result->end_pos) >
            position_to_ticks(&tmp->end_pos))
        result = tmp;
    }
	return result;
}

/**
 * Gets last (position-wise) MidiNote.
 *
 * If transient is 1, the transient notes are
 * checked instead.
 */
MidiNote *
midi_arranger_selections_get_last_midi_note (
  MidiArrangerSelections * mas,
  int                      transient)
{
	MidiNote * result = NULL;
  MidiNote * tmp = NULL;
	for (int i = 0;
		i < mas->num_midi_notes;
		i++)
    {
      if (transient)
        tmp = mas->midi_notes[i]->obj_info.main_trans;
      else
        tmp = mas->midi_notes[i]->obj_info.main;
      if (!result ||
          position_to_ticks(&result->end_pos) <
            position_to_ticks(&tmp->end_pos))
        result = tmp;
    }
	return result;
}

/**
 * Moves the MidiArrangerSelections by the given
 * amount of ticks.
 *
 * @param use_cached_pos Add the ticks to the cached
 *   Position's instead of the current Position's.
 * @param ticks Ticks to add.
 * @param update_flag ArrangerObjectUpdateFlag.
 */
void
midi_arranger_selections_add_ticks (
  MidiArrangerSelections * mas,
  long                 ticks,
  int                  use_cached_pos,
  ArrangerObjectUpdateFlag update_flag)
{
  int i;

  /* update midi note positions */
  MidiNote * mn;
  for (i = 0; i < mas->num_midi_notes; i++)
    {
      mn = mas->midi_notes[i];
      midi_note_move (
        mn, ticks, use_cached_pos, update_flag);
    }
}

void
midi_arranger_selections_paste_to_pos (
  MidiArrangerSelections * ts,
  Position *           pos)
{
  int pos_ticks = position_to_ticks (pos);

  /* get pos of earliest object */
  Position start_pos;
  midi_arranger_selections_get_start_pos (
    ts, &start_pos, 0, 1);
  int start_pos_ticks =
    position_to_ticks (&start_pos);

  /* subtract the start pos from every object and
   * add the given pos */
#define DIFF (curr_ticks - start_pos_ticks)
#define ADJUST_POSITION(x) \
  curr_ticks = position_to_ticks (x); \
  position_from_ticks (x, pos_ticks + DIFF)

  int curr_ticks, i;
  for (i = 0; i < ts->num_midi_notes; i++)
    {
      MidiNote * midi_note = ts->midi_notes[i];

      /* update positions */
      curr_ticks =
        position_to_ticks (&midi_note->start_pos);
      position_from_ticks (
        &midi_note->start_pos,
        pos_ticks + DIFF);
      curr_ticks =
        position_to_ticks (&midi_note->end_pos);
      position_from_ticks (&midi_note->end_pos,
                           pos_ticks + DIFF);

      /* clone and add to track */
      MidiNote * cp =
        midi_note_clone (
          midi_note,
          MIDI_NOTE_CLONE_COPY_MAIN);
      midi_region_add_midi_note (
        cp->region, cp, F_GEN_WIDGET);
    }
#undef DIFF
}

void
midi_arranger_selections_free (
  MidiArrangerSelections * self)
{
  free (self);
}

SERIALIZE_SRC (MidiArrangerSelections,
               midi_arranger_selections)
DESERIALIZE_SRC (MidiArrangerSelections,
                 midi_arranger_selections)
PRINT_YAML_SRC (MidiArrangerSelections,
                midi_arranger_selections)

