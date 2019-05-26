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

#include "audio/chord_track.h"
#include "audio/position.h"
#include "audio/track.h"
#include "audio/transport.h"
#include "gui/backend/events.h"
#include "gui/backend/ruler_tracklist_selections.h"
#include "project.h"
#include "utils/arrays.h"
#include "utils/flags.h"
#include "utils/objects.h"
#include "utils/yaml.h"

#include <gtk/gtk.h>

void
ruler_tracklist_selections_init_loaded (
  RulerTracklistSelections * ts)
{
  for (int i = 0; i < ts->num_chords; i++)
    ts->chords[i] =
      chord_find (ts->chords[i]);
}

/**
 * Creates transient objects for objects added
 * to selections without transients.
 */
void
ruler_tracklist_selections_create_missing_transients (
  RulerTracklistSelections * ts)
{
}

/**
 * Returns if there are any selections.
 */
int
ruler_tracklist_selections_has_any (
  RulerTracklistSelections * ts)
{
  if (ts->num_chords > 0)
    return 1;
  else
    return 0;
}

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
  int                  transient)
{
  position_set_to_bar (pos,
                       TRANSPORT->total_bars);

  for (int i = 0; i < ts->num_chords; i++)
    {
      ZChord * chord =
        transient ?
        ts->transient_chords[i] :
        ts->chords[i];
      if (position_compare (&chord->pos,
                            pos) < 0)
        position_set_to_pos (pos,
                             &chord->pos);
    }
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
ruler_tracklist_selections_get_end_pos (
  RulerTracklistSelections * ts,
  Position *           pos,
  int                  transient)
{
  position_init (pos);

  for (int i = 0; i < ts->num_chords; i++)
    {
      ZChord * chord =
        transient ?
        ts->transient_chords[i] :
        ts->chords[i];
      /* FIXME take into account fixed size */
      if (position_compare (&chord->pos,
                            pos) > 0)
        position_set_to_pos (pos,
                             &chord->pos);
    }
}

/**
 * Gets first object's widget.
 *
 * If transient is 1, transient objects rae checked
 * instead.
 */
GtkWidget *
ruler_tracklist_selections_get_first_object (
  RulerTracklistSelections * ts,
  int                  transient)
{
  Position pos;
  GtkWidget * widget = NULL;
  position_set_to_bar (&pos,
                       TRANSPORT->total_bars);

  for (int i = 0; i < ts->num_chords; i++)
    {
      ZChord * chord =
        transient ?
        ts->transient_chords[i] :
        ts->chords[i];
      if (position_compare (&chord->pos,
                            &pos) < 0)
        {
          position_set_to_pos (&pos,
                               &chord->pos);
          widget =
            GTK_WIDGET (chord->widget);
        }
    }
  return widget;
}

/**
 * Gets last object's widget.
 *
 * If transient is 1, transient objects rae checked
 * instead.
 */
GtkWidget *
ruler_tracklist_selections_get_last_object (
  RulerTracklistSelections * ts,
  int                  transient)
{
  Position pos;
  GtkWidget * widget = NULL;
  position_init (&pos);

  for (int i = 0; i < ts->num_chords; i++)
    {
      ZChord * chord =
        transient ?
        ts->transient_chords[i] :
        ts->chords[i];
      /* FIXME take into account fixed size */
      if (position_compare (&chord->pos,
                            &pos) > 0)
        {
          position_set_to_pos (
            &pos, &chord->pos);
          widget = GTK_WIDGET (chord->widget);
        }
    }
  return widget;
}

/**
 * Gets highest track in the selections.
 *
 * If transient is 1, transient objects rae checked
 * instead.
 */
Track *
ruler_tracklist_selections_get_highest_track (
  RulerTracklistSelections * ts,
  int                  transient)
{
  /* TODO */
  return NULL;
}

/**
 * Gets lowest track in the selections.
 *
 * If transient is 1, transient objects rae checked
 * instead.
 */
Track *
ruler_tracklist_selections_get_lowest_track (
  RulerTracklistSelections * ts,
  int                  transient)
{
  /* TODO */
  return NULL;
}

static void
remove_transient_chord (
  RulerTracklistSelections * ts,
  int                  index)
{
  ZChord * transient =
    ts->transient_chords[index];

  if (!transient || !transient->widget)
    return;

  g_warn_if_fail (
    GTK_IS_WIDGET (transient->widget));

  ts->transient_chords[index] = NULL;
  chord_track_remove_chord (
    P_CHORD_TRACK,
    transient);
}

/**
 * Only removes transients from their tracks and
 * frees them.
 */
void
ruler_tracklist_selections_remove_transients (
  RulerTracklistSelections * ts)
{
  for (int i = 0; i < ts->num_chords; i++)
    remove_transient_chord (ts, i);
}


void
ruler_tracklist_selections_add_chord (
  RulerTracklistSelections * ts,
  ZChord *             r,
  int                  transient)
{
  if (!array_contains (ts->chords,
                      ts->num_chords,
                      r))
    {
      array_append (ts->chords,
                    ts->num_chords,
                    r);

      EVENTS_PUSH (ET_CHORD_CHANGED,
                   r);
    }

  if (transient)
    ruler_tracklist_selections_create_missing_transients (
      ts);
}

void
ruler_tracklist_selections_remove_chord (
  RulerTracklistSelections * ts,
  ZChord *              c)
{
  if (!array_contains (
        ts->chords,
        ts->num_chords,
        c))
    return;

  int idx = -1;
  array_delete_return_pos (
    ts->chords,
    ts->num_chords,
    c,
    idx);

  EVENTS_PUSH (ET_CHORD_CHANGED,
               c);

  /* remove the transient */
  remove_transient_chord (ts, idx);
}

/**
 * Clears selections.
 */
void
ruler_tracklist_selections_clear (
  RulerTracklistSelections * ts)
{
  int i, num_chords;
  ZChord * c;

  /* use caches because ts->* will be operated on. */
  static ZChord * chords[600];
  for (i = 0; i < ts->num_chords; i++)
    {
      chords[i] = ts->chords[i];
    }
  num_chords = ts->num_chords;

  for (i = 0; i < num_chords; i++)
    {
      c = chords[i];
      ruler_tracklist_selections_remove_chord (
        ts, c);
      EVENTS_PUSH (ET_CHORD_CHANGED,
                   c);
    }
  g_message ("cleared ruler_tracklist selections");
}

/**
 * Clone the struct for copying, undoing, etc.
 */
RulerTracklistSelections *
ruler_tracklist_selections_clone (
  RulerTracklistSelections * src)
{
  RulerTracklistSelections * new_ts =
    calloc (1, sizeof (RulerTracklistSelections));

  /* TODO */

  return new_ts;
}

void
ruler_tracklist_selections_paste_to_pos (
  RulerTracklistSelections * ts,
  Position *           pos)
{
  int pos_ticks = position_to_ticks (pos);

  /* get pos of earliest object */
  Position start_pos;
  ruler_tracklist_selections_get_start_pos (
    ts, &start_pos, F_NO_TRANSIENTS);
  int start_pos_ticks =
    position_to_ticks (&start_pos);

  /* subtract the start pos from every object and
   * add the given pos */
#define DIFF (curr_ticks - start_pos_ticks)
#define ADJUST_POSITION(x) \
  curr_ticks = position_to_ticks (x); \
  position_from_ticks (x, pos_ticks + DIFF)

  long curr_ticks;
  for (int i = 0; i < ts->num_chords; i++)
    {
      ZChord * chord = ts->chords[i];

      curr_ticks = position_to_ticks (&chord->pos);
      position_from_ticks (&chord->pos,
                           pos_ticks + DIFF);
    }
#undef DIFF
}

void
ruler_tracklist_selections_free (
  RulerTracklistSelections * self)
{
  free (self);
}

SERIALIZE_SRC (
  RulerTracklistSelections,
  ruler_tracklist_selections)
DESERIALIZE_SRC (
  RulerTracklistSelections,
  ruler_tracklist_selections)
PRINT_YAML_SRC (
  RulerTracklistSelections,
  ruler_tracklist_selections)

