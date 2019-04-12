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
#include "audio/engine.h"
#include "audio/position.h"
#include "audio/track.h"
#include "audio/transport.h"
#include "gui/backend/events.h"
#include "gui/backend/timeline_selections.h"
#include "gui/widgets/audio_region.h"
#include "gui/widgets/midi_region.h"
#include "project.h"
#include "utils/arrays.h"
#include "utils/flags.h"
#include "utils/yaml.h"

#include <gtk/gtk.h>

void
timeline_selections_init_loaded (
  TimelineSelections * ts)
{
  int i;
  for (i = 0; i < ts->num_regions; i++)
    ts->regions[i] =
      project_get_region (ts->region_ids[i]);

  for (i = 0; i < ts->num_automation_points; i++)
    ts->automation_points[i] =
      project_get_automation_point (ts->ap_ids[i]);

  for (i = 0; i < ts->num_chords; i++)
    ts->chords[i] =
      project_get_chord (ts->chord_ids[i]);
}

/**
 * Creates transient objects for objects added
 * to selections without transients.
 */
void
timeline_selections_create_missing_transients (
  TimelineSelections * ts)
{
  Region * r = NULL, * transient = NULL;
  for (int i = 0; i < ts->num_regions; i++)
    {
      r = ts->regions[i];
      if (ts->transient_regions[i])
        g_warn_if_fail (
          GTK_IS_WIDGET (
            ts->transient_regions[i]->widget));
      else if (!ts->transient_regions[i])
        {
          /* create the transient */
          transient =
            region_clone (
              r, REGION_CLONE_COPY);
          transient->transient = 1;
          if (r->type == REGION_TYPE_MIDI)
            transient->widget =
              Z_REGION_WIDGET (
                midi_region_widget_new (transient));
          else if (r->type == REGION_TYPE_AUDIO)
            transient->widget =
              Z_REGION_WIDGET (
                audio_region_widget_new (transient));
          gtk_widget_set_visible (
            GTK_WIDGET (transient->widget),
            F_NOT_VISIBLE);

          /* add it to selections and to track */
          ts->transient_regions[i] =
            transient;
          track_add_region (
            transient->track, transient);
        }
      EVENTS_PUSH (ET_REGION_CHANGED,
                   r);
      EVENTS_PUSH (ET_REGION_CHANGED,
                   ts->transient_regions[i]);
    }
}

/**
 * Returns if there are any selections.
 */
int
timeline_selections_has_any (
  TimelineSelections * ts)
{
  if (ts->num_regions > 0 ||
      ts->num_automation_points > 0 ||
      ts->num_chords > 0)
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
timeline_selections_get_start_pos (
  TimelineSelections * ts,
  Position *           pos,
  int                  transient)
{
  position_set_to_bar (pos,
                       TRANSPORT->total_bars);

  for (int i = 0; i < ts->num_regions; i++)
    {
      Region * region =
        transient ?
        ts->transient_regions[i] :
        ts->regions[i];
      if (position_compare (&region->start_pos,
                            pos) < 0)
        position_set_to_pos (pos,
                             &region->start_pos);
    }
  for (int i = 0; i < ts->num_automation_points; i++)
    {
      AutomationPoint * automation_point =
        transient ?
        ts->transient_aps[i] :
        ts->automation_points[i];
      if (position_compare (&automation_point->pos,
                            pos) < 0)
        position_set_to_pos (pos,
                             &automation_point->pos);
    }
  for (int i = 0; i < ts->num_chords; i++)
    {
      ZChord * chord =
        transient ?
        ts->transient_chords :
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
timeline_selections_get_end_pos (
  TimelineSelections * ts,
  Position *           pos,
  int                  transient)
{
  position_init (pos);

  for (int i = 0; i < ts->num_regions; i++)
    {
      Region * region =
        transient ?
        ts->transient_regions[i] :
        ts->regions[i];
      if (position_compare (&region->end_pos,
                            pos) > 0)
        position_set_to_pos (pos,
                             &region->end_pos);
    }
  for (int i = 0; i < ts->num_automation_points; i++)
    {
      AutomationPoint * automation_point =
        transient ?
        ts->transient_aps[i] :
        ts->automation_points[i];
      if (position_compare (&automation_point->pos,
                            pos) > 0)
        position_set_to_pos (pos,
                             &automation_point->pos);
    }
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
timeline_selections_get_first_object (
  TimelineSelections * ts,
  int                  transient)
{
  Position pos;
  GtkWidget * widget = NULL;
  position_set_to_bar (&pos,
                       TRANSPORT->total_bars);

  for (int i = 0; i < ts->num_regions; i++)
    {
      Region * region =
        transient ?
        ts->transient_regions[i] :
        ts->regions[i];
      if (position_compare (&region->start_pos,
                            &pos) < 0)
        {
          position_set_to_pos (&pos,
                               &region->start_pos);
          widget = GTK_WIDGET (region->widget);
        }
    }
  for (int i = 0; i < ts->num_automation_points; i++)
    {
      AutomationPoint * automation_point =
        transient ?
        ts->transient_aps[i] :
        ts->automation_points[i];
      if (position_compare (&automation_point->pos,
                            &pos) < 0)
        {
          position_set_to_pos (
            &pos, &automation_point->pos);
          widget =
            GTK_WIDGET (automation_point->widget);
        }
    }
  for (int i = 0; i < ts->num_chords; i++)
    {
      ZChord * chord =
        transient ?
        ts->transient_chords :
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
timeline_selections_get_last_object (
  TimelineSelections * ts,
  int                  transient)
{
  Position pos;
  GtkWidget * widget = NULL;
  position_init (&pos);

  for (int i = 0; i < ts->num_regions; i++)
    {
      Region * region =
        transient ?
        ts->transient_regions[i] :
        ts->regions[i];
      if (position_compare (&region->end_pos,
                            &pos) > 0)
        {
          position_set_to_pos (&pos,
                               &region->end_pos);
          widget =
            GTK_WIDGET (region->widget);
        }
    }
  for (int i = 0; i < ts->num_automation_points; i++)
    {
      AutomationPoint * automation_point =
        transient ?
        ts->transient_aps[i] :
        ts->automation_points[i];
      if (position_compare (&automation_point->pos,
                            &pos) > 0)
        {
          position_set_to_pos (
            &pos, &automation_point->pos);
          widget =
            GTK_WIDGET (automation_point->widget);
        }
    }
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
timeline_selections_get_highest_track (
  TimelineSelections * ts,
  int                  transient)
{
  int track_pos = -1;
  Track * track = NULL;
  int tmp_pos;
  Track * tmp_track;

#define CHECK_POS(_track) \
  tmp_track = _track; \
  tmp_pos = \
    tracklist_get_track_pos ( \
      tmp_track); \
  if (tmp_pos > track_pos) \
    { \
      track_pos = tmp_pos; \
      track = tmp_track; \
    }

  for (int i = 0; i < ts->num_regions; i++)
    {
      Region * region =
        transient ?
        ts->transient_regions[i] :
        ts->regions[i];
      CHECK_POS (region->track);
    }
  for (int i = 0; i < ts->num_automation_points; i++)
    {
      AutomationPoint * automation_point =
        transient ?
        ts->transient_aps[i] :
        ts->automation_points[i];
      CHECK_POS (automation_point->at->track);
    }
  CHECK_POS (P_CHORD_TRACK);

  return track;
#undef CHECK_POS
}

/**
 * Gets lowest track in the selections.
 *
 * If transient is 1, transient objects rae checked
 * instead.
 */
Track *
timeline_selections_get_lowest_track (
  TimelineSelections * ts,
  int                  transient)
{
  int track_pos = 8000;
  Track * track = NULL;
  int tmp_pos;
  Track * tmp_track;

#define CHECK_POS(_track) \
  tmp_track = _track; \
  tmp_pos = \
    tracklist_get_track_pos ( \
      tmp_track); \
  if (tmp_pos < track_pos) \
    { \
      track_pos = tmp_pos; \
      track = tmp_track; \
    }

  for (int i = 0; i < ts->num_regions; i++)
    {
      Region * region =
        transient ?
        ts->transient_regions[i] :
        ts->regions[i];
      CHECK_POS (region->track);
    }
  for (int i = 0; i < ts->num_automation_points; i++)
    {
      AutomationPoint * automation_point =
        transient ?
        ts->transient_aps[i] :
        ts->automation_points[i];
      CHECK_POS (automation_point->at->track);
    }
  CHECK_POS (P_CHORD_TRACK);

  return track;
#undef CHECK_POS
}

static void
remove_transient_region (
  TimelineSelections * ts,
  int                  index)
{
  Region * transient =
    ts->transient_regions[index];

  if (!transient || !transient->widget)
    return;

  g_warn_if_fail (
    GTK_IS_WIDGET (transient->widget));

  ts->transient_regions[index] = NULL;
  track_remove_region (
    transient->track,
    transient,
    F_FREE);
}

static void
remove_transient_ap (
  TimelineSelections * ts,
  int                  index)
{
  AutomationPoint * transient =
    ts->transient_aps[index];

  if (!transient || !transient->widget)
    return;

  g_warn_if_fail (
    GTK_IS_WIDGET (transient->widget));

  ts->transient_aps[index] = NULL;
  automation_track_remove_ap (
    transient->at,
    transient);
}

static void
remove_transient_chord (
  TimelineSelections * ts,
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
timeline_selections_remove_transients (
  TimelineSelections * ts)
{
  for (int i = 0; i < ts->num_regions; i++)
    remove_transient_region (ts, i);
  for (int i = 0; i < ts->num_automation_points; i++)
    remove_transient_ap (ts, i);
  for (int i = 0; i < ts->num_chords; i++)
    remove_transient_chord (ts, i);
}

/**
 * Adds an object to the selections.
 *
 * Optionally adds a transient object (if moving/
 * copy-moving).
 */
void
timeline_selections_add_region (
  TimelineSelections * ts,
  Region *             r,
  int                  transient)
{
  if (!array_contains (ts->regions,
                      ts->num_regions,
                      r))
    {
      ts->region_ids[ts->num_regions] =
        r->id;
      array_append (ts->regions,
                    ts->num_regions,
                    r);

      EVENTS_PUSH (ET_REGION_CHANGED,
                   r);
    }

  if (transient)
    timeline_selections_create_missing_transients (ts);

}

void
timeline_selections_add_chord (
  TimelineSelections * ts,
  ZChord *             r,
  int                  transient)
{
  if (!array_contains (ts->chords,
                      ts->num_chords,
                      r))
    {
      ts->chord_ids[ts->num_chords] =
        r->id;
      array_append (ts->chords,
                    ts->num_chords,
                    r);

      EVENTS_PUSH (ET_CHORD_CHANGED,
                   r);
    }

  if (transient)
    timeline_selections_create_missing_transients (ts);
}

void
timeline_selections_add_ap (
  TimelineSelections * ts,
  AutomationPoint *    ap,
  int                  transient)
{
  if (!array_contains (ts->automation_points,
                      ts->num_automation_points,
                      ap))
    {
      ts->ap_ids[ts->num_automation_points] =
        ap->id;
      array_append (ts->automation_points,
                    ts->num_automation_points,
                    ap);

      EVENTS_PUSH (ET_AUTOMATION_POINT_CHANGED,
                   ap);
    }

  if (transient)
    timeline_selections_create_missing_transients (ts);
}

void
timeline_selections_remove_region (
  TimelineSelections * ts,
  Region *             r)
{
  if (!array_contains (ts->regions,
                       ts->num_regions,
                       r))
    {
      EVENTS_PUSH (ET_REGION_CHANGED,
                   r);
      return;
    }

  int idx;
  array_delete_return_pos (ts->regions,
                ts->num_regions,
                r,
                idx);
  int size = ts->num_regions + 1;
  array_delete (ts->region_ids,
                size,
                r->id);

  /* remove the transient */
  remove_transient_region (ts, idx);
}

void
timeline_selections_remove_chord (
  TimelineSelections * ts,
  ZChord *              c)
{
  if (!array_contains (ts->chords,
                       ts->num_chords,
                       c))
    return;

  int idx;
  array_delete_return_pos (ts->chords,
                ts->num_chords,
                c,
                idx);
  int size = ts->num_chords + 1;
  array_delete (ts->chord_ids,
                size,
                c->id);

  EVENTS_PUSH (ET_CHORD_CHANGED,
               c);

  /* remove the transient */
  remove_transient_chord (ts, idx);
}

void
timeline_selections_remove_ap (
  TimelineSelections * ts,
  AutomationPoint *    ap)
{
  if (!array_contains (ts->automation_points,
                       ts->num_automation_points,
                       ap))
    return;

  int idx;
  array_delete_return_pos (ts->automation_points,
                ts->num_automation_points,
                ap,
                idx);
  int size = ts->num_automation_points + 1;
  array_delete (ts->ap_ids,
                size,
                ap->id);

  EVENTS_PUSH (ET_AUTOMATION_POINT_CHANGED,
               ap);

  /* remove the transient */
  remove_transient_ap (ts, idx);
}

int
timeline_selections_contains_region (
  TimelineSelections * self,
  Region *             region,
  int                  check_transients)
{
  if (array_contains (self->regions,
                      self->num_regions,
                      region))
    return 1;

  if (check_transients &&
      array_contains (self->transient_regions,
                      self->num_regions,
                      region))
    return 1;

  return 0;
}

/**
 * Clears selections.
 */
void
timeline_selections_clear (
  TimelineSelections * ts)
{
  int i, num_regions, num_chords, num_aps;
  Region * r;
  ZChord * c;
  AutomationPoint * ap;

  /* use caches because ts->* will be operated on. */
  static Region * regions[600];
  static ZChord * chords[600];
  static AutomationPoint * aps[600];
  for (i = 0; i < ts->num_regions; i++)
    {
      regions[i] = ts->regions[i];
    }
  num_regions = ts->num_regions;
  for (i = 0; i < ts->num_chords; i++)
    {
      chords[i] = ts->chords[i];
    }
  num_chords = ts->num_chords;
  for (i = 0; i < ts->num_automation_points; i++)
    {
      aps[i] = ts->automation_points[i];
    }
  num_aps = ts->num_automation_points;

  for (i = 0; i < num_regions; i++)
    {
      r = regions[i];
      timeline_selections_remove_region (
        ts, r);
      EVENTS_PUSH (ET_REGION_CHANGED,
                   r);
    }
  for (i = 0; i < num_chords; i++)
    {
      c = chords[i];
      timeline_selections_remove_chord (
        ts, c);
      EVENTS_PUSH (ET_CHORD_CHANGED,
                   c);
    }
  for (i = 0; i < num_aps; i++)
    {
      ap = aps[i];
      timeline_selections_remove_ap (
        ts, ap);
      EVENTS_PUSH (ET_AUTOMATION_POINT_CHANGED,
                   ap);
    }
  g_message ("cleared timeline selections");
}

/**
 * Clone the struct for copying, undoing, etc.
 */
TimelineSelections *
timeline_selections_clone ()
{
  TimelineSelections * new_ts =
    calloc (1, sizeof (TimelineSelections));

  TimelineSelections * src = TL_SELECTIONS;

  /* FIXME only does regions */
  for (int i = 0; i < src->num_regions; i++)
    {
      Region * r = src->regions[i];
      Region * new_r =
        region_clone (r, REGION_CLONE_COPY);
      array_append (new_ts->regions,
                    new_ts->num_regions,
                    new_r);
    }

  return new_ts;
}

void
timeline_selections_paste_to_pos (
  TimelineSelections * ts,
  Position *           pos)
{
  int pos_ticks = position_to_ticks (pos);

  /* get pos of earliest object */
  Position start_pos;
  timeline_selections_get_start_pos (
    ts, &start_pos, F_NO_TRANSIENTS);
  int start_pos_ticks =
    position_to_ticks (&start_pos);

  /* subtract the start pos from every object and
   * add the given pos */
#define DIFF (curr_ticks - start_pos_ticks)
#define ADJUST_POSITION(x) \
  curr_ticks = position_to_ticks (x); \
  position_from_ticks (x, pos_ticks + DIFF)

  g_message ("[before loop]num regions %d num midi notes %d",
             ts->num_regions,
             ts->regions[0]->num_midi_notes);

  int curr_ticks, i;
  for (i = 0; i < ts->num_regions; i++)
    {
      Region * region = ts->regions[i];

      /* update positions */
      curr_ticks = position_to_ticks (&region->start_pos);
      position_from_ticks (&region->start_pos,
                           pos_ticks + DIFF);
      curr_ticks = position_to_ticks (&region->end_pos);
      position_from_ticks (&region->end_pos,
                           pos_ticks + DIFF);
      /* TODO */
      /*position_set_to_pos (&region->unit_end_pos,*/
                           /*&region->end_pos);*/
  g_message ("[in loop]num regions %d num midi notes %d",
             ts->num_regions,
             ts->regions[0]->num_midi_notes);

      /* same for midi notes */
      g_message ("region type %d", region->type);
      if (region->type == REGION_TYPE_MIDI)
        {
          MidiRegion * mr = region;
          g_message ("HELLO?");
          g_message ("num midi notes here %d",
                     mr->num_midi_notes);
          for (int j = 0; j < mr->num_midi_notes; j++)
            {
              MidiNote * mn = mr->midi_notes[j];
              g_message ("old midi start");
              /*position_print (&mn->start_pos);*/
              g_message ("bars %d",
                         mn->start_pos.bars);
              g_message ("new midi start");
              ADJUST_POSITION (&mn->start_pos);
              position_print (&mn->start_pos);
              g_message ("old midi start");
              ADJUST_POSITION (&mn->end_pos);
              position_print (&mn->end_pos);
            }
        }

      /* clone and add to track */
      Region * cp =
        region_clone (region,
                      REGION_CLONE_COPY);
      region_print (cp);
      track_add_region (cp->track,
                        cp);
    }
  for (i = 0; i < ts->num_automation_points; i++)
    {
      AutomationPoint * ap =
        ts->automation_points[i];

      curr_ticks = position_to_ticks (&ap->pos);
      position_from_ticks (&ap->pos,
                           pos_ticks + DIFF);
    }
  for (i = 0; i < ts->num_chords; i++)
    {
      ZChord * chord = ts->chords[i];

      curr_ticks = position_to_ticks (&chord->pos);
      position_from_ticks (&chord->pos,
                           pos_ticks + DIFF);
    }
#undef DIFF
}

void
timeline_selections_free (TimelineSelections * self)
{
  free (self);
}

SERIALIZE_SRC (
  TimelineSelections, timeline_selections)
DESERIALIZE_SRC (
  TimelineSelections, timeline_selections)
PRINT_YAML_SRC (
  TimelineSelections, timeline_selections)
