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
  for (i = 0; i < self->num_midi_notes; i++)
    self->midi_notes[i] =
      project_get_midi_note (
        self->midi_note_ids[i]);
}

/**
 * Returns the position of the leftmost object.
 */
static void
get_start_pos (
  MidiArrangerSelections * ts,
  Position *           pos) ///< position to fill in
{
  position_set_to_bar (pos,
                       TRANSPORT->total_bars);

  for (int i = 0; i < ts->num_midi_notes; i++)
    {
      MidiNote * midi_note = ts->midi_notes[i];
      if (position_compare (&midi_note->start_pos,
                            pos) < 0)
        position_set_to_pos (pos,
                             &midi_note->start_pos);
    }
}

/**
 * Clears selections.
 */
void
midi_arranger_selections_clear (
  MidiArrangerSelections * mas)
{
  MidiNote * mn;
  for (int i = 0; i < mas->num_midi_notes; i++)
    {
      mn = mas->midi_notes[i];
      midi_arranger_selections_remove_note (
        mas, mn);
      EVENTS_PUSH (ET_MIDI_NOTE_CHANGED,
                   mn);
    }
  /*g_message ("cleared midi arranger selections");*/
}

static void
remove_transient (
  MidiArrangerSelections * mas,
  int                      index)
{
  MidiNote * transient =
    mas->transient_notes[index];

  if (!transient || !transient->widget)
    return;

  g_warn_if_fail (
    GTK_IS_WIDGET (transient->widget));

  g_message ("removing transient %p at %d",
             transient->widget, index);
  mas->transient_notes[index] = NULL;
  midi_region_remove_midi_note (
    transient->region,
    transient);
}

/**
 * Only removes transients from their regions and
 * frees them.
 */
void
midi_arranger_selections_remove_transients (
  MidiArrangerSelections * mas)
{
  for (int i = 0; i < mas->num_midi_notes; i++)
    {
      remove_transient (mas, i);
    }
}

/**
 * Adds a note to the selections.
 *
 * Optionally adds the missing transient notes
 * (if moving / copy-moving).
 */
void
midi_arranger_selections_add_note (
  MidiArrangerSelections * mas,
  MidiNote *               note,
  int                      transient)
{
  if (!array_contains (mas->midi_notes,
                      mas->num_midi_notes,
                      note))
    {
      mas->midi_note_ids[mas->num_midi_notes] =
        note->id;
      array_append (mas->midi_notes,
                    mas->num_midi_notes,
                    note);

      EVENTS_PUSH (ET_MIDI_NOTE_CHANGED,
                   note);
    }

  if (transient)
    midi_arranger_selections_create_missing_transients (mas);
}

/**
 * Creates transient notes for notes added
 * to selections without transients.
 */
void
midi_arranger_selections_create_missing_transients (
  MidiArrangerSelections * mas)
{
  g_message ("creating missing transients");
  MidiNote * note = NULL, * transient = NULL;
  for (int i = 0; i < mas->num_midi_notes; i++)
    {
      note = mas->midi_notes[i];
      if (mas->transient_notes[i])
        g_warn_if_fail (
          GTK_IS_WIDGET (
            mas->transient_notes[i]->widget));
      else if (!mas->transient_notes[i])
        {
          /* create the transient */
          transient =
            midi_note_clone (
              note, note->region);
          /*transient->visible = 0;*/
          transient->transient = 1;
          transient->widget =
            midi_note_widget_new (transient);
          gtk_widget_set_visible (
            GTK_WIDGET (transient->widget),
            F_NOT_VISIBLE);

          /* add it to selections and to region */
          mas->transient_notes[i] =
            transient;
          midi_region_add_midi_note (
            transient->region, transient);
          g_message ("created %p transient",
                     transient->widget);
        }
      EVENTS_PUSH (ET_MIDI_NOTE_CHANGED,
                   note);
      EVENTS_PUSH (ET_MIDI_NOTE_CHANGED,
                   mas->transient_notes[i]);
    }
  g_message ("created missing transients");
}

void
midi_arranger_selections_remove_note (
  MidiArrangerSelections * mas,
  MidiNote *               note)
{
  if (!array_contains (mas->midi_notes,
                       mas->num_midi_notes,
                       note))
    return;

  int idx;
  array_delete_return_pos (mas->midi_notes,
                mas->num_midi_notes,
                note,
                idx);
  int size = mas->num_midi_notes + 1;
  array_delete (mas->midi_note_ids,
                size,
                note->id);

  /* remove the transient */
  /*g_message ("idx %d", idx);*/
  remove_transient (mas, idx);
}

/**
 * Clone the struct for copying, undoing, etc.
 */
MidiArrangerSelections *
midi_arranger_selections_clone ()
{
  MidiArrangerSelections * new_ts =
    calloc (1, sizeof (MidiArrangerSelections));

  MidiArrangerSelections * src =
    MIDI_ARRANGER_SELECTIONS;
  for (int i = 0; i < src->num_midi_notes; i++)
    {
      MidiNote * r = src->midi_notes[i];
      MidiNote * new_r =
        midi_note_clone (r, r->region);
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
  MidiNote * top_mn =
    transient ?
    mas->transient_notes[0] :
    mas->midi_notes[0];
  MidiNote * tmp;
  for (int i = 0;
       i < mas->num_midi_notes;
       i++)
    {
      tmp =
        transient ?
        mas->transient_notes[i] :
        mas->midi_notes[i];
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

  MidiNote * bot_mn =
    transient ?
    mas->transient_notes[0] :
    mas->midi_notes[0];
  MidiNote * tmp;
  for (int i = 0;
       i < mas->num_midi_notes;
       i++)
    {
      tmp =
        transient ?
        mas->transient_notes[i] :
        mas->midi_notes[i];
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
    tmp =
      transient ?
      mas->transient_notes[i] :
      mas->midi_notes[i];
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
      tmp =
        transient ?
        mas->transient_notes[i] :
        mas->midi_notes[i];
      if (!result ||
          position_to_ticks(&result->end_pos) <
            position_to_ticks(&tmp->end_pos))
        result = tmp;
    }
	return result;
}

void
midi_arranger_selections_paste_to_pos (
  MidiArrangerSelections * ts,
  Position *           pos)
{
  int pos_ticks = position_to_ticks (pos);

  /* get pos of earliest object */
  Position start_pos;
  get_start_pos (ts,
                 &start_pos);
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
        midi_note_clone (midi_note,
                      MIDI_NOTE_CLONE_COPY);
      /*midi_note_print (cp);*/
      midi_region_add_midi_note (
        cp->region,
        cp);
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

