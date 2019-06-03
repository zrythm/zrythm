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
#include "utils/audio.h"
#include "utils/flags.h"
#include "utils/objects.h"
#include "utils/yaml.h"

#include <gtk/gtk.h>

/**
 * Sets the Position pos to the earliest or latest
 * object.
 *
 * @param cc Camel case.
 * @param lc Lower case
 * @param pos_name Variable name of position to check
 * @param bef_or_aft Before or after
 * @param widg Widget to set.
 */
#define SET_POS_TO(cc,lc,pos_name,transient,bef_or_aft,widg) \
  cc * lc; \
  for (i = 0; i < ts->num_##lc##s; i++) \
    { \
      if (transient) \
        lc = ts->lc##s[i]->obj_info.main_trans; \
      else \
        lc = ts->lc##s[i]->obj_info.main; \
      if (position_is_##bef_or_aft ( \
            &lc->pos_name, pos)) \
        { \
          position_set_to_pos ( \
            pos, &lc->pos_name); \
          widget = GTK_WIDGET (lc->widget); \
        } \
    }

void
timeline_selections_init_loaded (
  TimelineSelections * ts)
{
  int i;
  for (i = 0; i < ts->num_regions; i++)
    ts->regions[i] =
      region_find (ts->regions[i]);

  for (i = 0; i < ts->num_aps; i++)
    ts->aps[i] =
      automation_point_find (ts->aps[i]);

  for (int i = 0; i < ts->num_chords; i++)
    ts->chords[i] =
      chord_find (ts->chords[i]);
}

/**
 * Returns if there are any selections.
 */
int
timeline_selections_has_any (
  TimelineSelections * ts)
{
  if (ts->num_regions > 0 ||
      ts->num_aps > 0 ||
      ts->num_chords > 0)
    return 1;
  else
    return 0;
}

/**
 * Sets the cache Position's for each object in
 * the selection.
 *
 * Used by the ArrangerWidget's.
 */
void
timeline_selections_set_cache_poses (
  TimelineSelections * ts)
{
  int i;

  Region * r;
  for (i = 0; i < ts->num_regions; i++)
    {
      r = ts->regions[i];

      /* set start poses for regions */
      region_set_cache_start_pos (
        r, &r->start_pos);

      /* set end poses for regions */
      region_set_cache_end_pos (
        r, &r->end_pos);
    }
  ZChord * c;
  for (i = 0; i < ts->num_chords; i++)
    {
      c = ts->chords[i];

      /* set start poses for chords */
      position_set_to_pos (
        &c->cache_pos, &c->pos);
    }
  AutomationPoint * ap;
  for (i = 0; i < ts->num_aps; i++)
    {
      ap = ts->aps[i];

      /* set start poses for APs */
      position_set_to_pos (
        &ap->cache_pos, &ap->pos);
    }
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
  GtkWidget * widget = NULL;

  int i;

  SET_POS_TO (Region, region, start_pos,
              transient, before, widget);
  SET_POS_TO (AutomationPoint, ap, pos,
              transient, before, widget);
  SET_POS_TO (ZChord, chord, pos,
              transient, before, widget);
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
  GtkWidget * widget = NULL;

  int i;

  SET_POS_TO (Region, region, start_pos,
              transient, after, widget);
  SET_POS_TO (AutomationPoint, ap, pos,
              transient, after, widget);
  SET_POS_TO (ZChord, chord, pos,
              transient, after, widget);
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
  Position _pos;
  Position * pos = &_pos;
  GtkWidget * widget = NULL;
  position_set_to_bar (
    pos, TRANSPORT->total_bars);
  int i;

  SET_POS_TO (Region, region, start_pos,
              transient, before, widget);
  SET_POS_TO (AutomationPoint, ap, pos,
              transient, before, widget);
  SET_POS_TO (ZChord, chord, pos,
              transient, before, widget);

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
  Position _pos;
  Position * pos = &_pos;
  GtkWidget * widget = NULL;
  position_init (pos);
  int i;

  SET_POS_TO (Region, region, start_pos,
              transient, after, widget);
  SET_POS_TO (AutomationPoint, ap, pos,
              transient, after, widget);
  SET_POS_TO (ZChord, chord, pos,
              transient, after, widget);

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
      TRACKLIST, tmp_track); \
  if (tmp_pos > track_pos) \
    { \
      track_pos = tmp_pos; \
      track = tmp_track; \
    }

  Region * region;
  for (int i = 0; i < ts->num_regions; i++)
    {
      if (transient)
        region = ts->regions[i]->obj_info.main_trans;
      else
        region = ts->regions[i]->obj_info.main;
      CHECK_POS (region->lane->track);
    }
  AutomationPoint * ap;
  for (int i = 0; i < ts->num_aps; i++)
    {
      if (transient)
        ap = ts->aps[i]->obj_info.main_trans;
      else
        ap = ts->aps[i]->obj_info.main;
      CHECK_POS (ap->at->track);
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
      TRACKLIST, tmp_track); \
  if (tmp_pos < track_pos) \
    { \
      track_pos = tmp_pos; \
      track = tmp_track; \
    }

  Region * region;
  for (int i = 0; i < ts->num_regions; i++)
    {
      if (transient)
        region = ts->regions[i]->obj_info.main_trans;
      else
        region = ts->regions[i]->obj_info.main;
      CHECK_POS (region->lane->track);
    }
  AutomationPoint * ap;
  for (int i = 0; i < ts->num_aps; i++)
    {
      if (transient)
        ap = ts->aps[i]->obj_info.main_trans;
      else
        ap = ts->aps[i]->obj_info.main;
      CHECK_POS (ap->at->track);
    }
  CHECK_POS (P_CHORD_TRACK);

  return track;
#undef CHECK_POS
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
  Region *             r)
{
  if (!array_contains (ts->regions,
                      ts->num_regions,
                      r))
    {
      array_append (ts->regions,
                    ts->num_regions,
                    r);

      EVENTS_PUSH (ET_REGION_CHANGED,
                   r);
    }
}

void
timeline_selections_add_chord (
  TimelineSelections * ts,
  ZChord *             r)
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
}

void
timeline_selections_add_ap (
  TimelineSelections * ts,
  AutomationPoint *    ap)
{
  if (!array_contains (
        ts->aps,
        ts->num_aps,
        ap))
    {
      array_append (
        ts->aps,
        ts->num_aps,
        ap);

      EVENTS_PUSH (ET_AUTOMATION_POINT_CHANGED,
                   ap);
    }
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

  array_delete (
    ts->regions,
    ts->num_regions,
    r);
}

void
timeline_selections_remove_chord (
  TimelineSelections * ts,
  ZChord *              c)
{
  if (!array_contains (
        ts->chords,
        ts->num_chords,
        c))
    return;

  array_delete (
    ts->chords,
    ts->num_chords,
    c);

  EVENTS_PUSH (ET_CHORD_CHANGED,
               c);
}

void
timeline_selections_remove_ap (
  TimelineSelections * ts,
  AutomationPoint *    ap)
{
  if (!array_contains (
        ts->aps,
        ts->num_aps,
        ap))
    return;

  array_delete (
    ts->aps,
    ts->num_aps,
    ap);

  EVENTS_PUSH (ET_AUTOMATION_POINT_CHANGED,
               ap);
}

int
timeline_selections_contains_region (
  TimelineSelections * self,
  Region *             region)
{
  if (array_contains (self->regions,
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
  for (i = 0; i < ts->num_aps; i++)
    {
      aps[i] = ts->aps[i];
    }
  num_aps = ts->num_aps;

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
  Region *r, * new_r;
  for (int i = 0; i < src->num_regions; i++)
    {
      r = src->regions[i];
      new_r =
        region_clone (r, REGION_CLONE_COPY);
      array_append (new_ts->regions,
                    new_ts->num_regions,
                    new_r);
    }

  return new_ts;
}

/**
 * Moves the TimelineSelections by the given
 * amount of ticks.
 *
 * @param use_cached_pos Add the ticks to the cached
 *   Position's instead of the current Position's.
 * @param ticks Ticks to add.
 */
void
timeline_selections_add_ticks (
  TimelineSelections * ts,
  long                 ticks,
  int                  use_cached_pos)
{
  Position tmp;
  long length_ticks;
  int i, j;

  /* update region positions */
  Region * r;
  for (i = 0; i <
       TL_SELECTIONS->num_regions; i++)
    {
      r =
        TL_SELECTIONS->regions[i];
      region_move (r, ticks, use_cached_pos);
    }

  /* update chord positions */
  /*ZChord * c;*/
  /*for (i = 0;*/
       /*i < TL_SELECTIONS->num_chords; i++)*/
    /*{*/
      /*for (j = 0; j < 2; j++)*/
        /*{*/
          /*if (j == 0)*/
            /*c =*/
              /*TL_SELECTIONS->chords[i]->*/
                /*obj_info.main_trans;*/
          /*else*/
            /*c =*/
              /*TL_SELECTIONS->chords[i]->*/
                /*obj_info.lane_trans;*/

          /*ARRANGER_MOVE_OBJ_BY_TICKS (*/
            /*c, chord,*/
            /*&c->cache_pos,*/
            /*ticks_diff, &tmp);*/
        /*}*/
    /*}*/

  /* update ap positions */
  AutomationPoint * ap;
  for (i = 0;
       i < TL_SELECTIONS->num_aps;
       i++)
    {
      ap =
        TL_SELECTIONS->aps[i]->
          obj_info.main_trans;

      /* get prev and next value APs */
      AutomationPoint * prev_ap =
        automation_track_get_prev_ap (ap->at, ap);
      AutomationPoint * next_ap =
        automation_track_get_next_ap (ap->at, ap);

      /* get adjusted pos for this automation point */
      Position ap_pos;
      Position * prev_pos = &ap->cache_pos;
      position_set_to_pos (&ap_pos,
                           prev_pos);
      position_add_ticks (&ap_pos, ticks);

      Position mid_pos;
      AutomationCurve * ac;

      /* update midway points */
      if (prev_ap &&
          position_is_after_or_equal (
            &ap_pos, &prev_ap->pos))
        {
          /* set prev curve point to new midway pos */
          position_get_midway_pos (
            &prev_ap->pos, &ap_pos, &mid_pos);
          ac =
            automation_track_get_next_curve_ac (
              ap->at, prev_ap);
          position_set_to_pos (&ac->pos, &mid_pos);

          /* set pos for ap */
          if (!next_ap)
            {
              position_set_to_pos (&ap->pos, &ap_pos);
            }
        }
      if (next_ap &&
          position_is_before_or_equal (
            &ap_pos, &next_ap->pos))
        {
          /* set next curve point to new midway pos */
          position_get_midway_pos (
            &ap_pos, &next_ap->pos, &mid_pos);
          ac =
            automation_track_get_next_curve_ac (
              ap->at, ap);
          position_set_to_pos (&ac->pos, &mid_pos);

          /* set pos for ap - if no prev ap exists
           * or if the position is also after the
           * prev ap */
          if ((prev_ap &&
               position_is_after_or_equal (
                &ap_pos, &prev_ap->pos)) ||
              (!prev_ap))
            {
              position_set_to_pos (&ap->pos, &ap_pos);
            }
        }
      else if (!prev_ap && !next_ap)
        {
          /* set pos for ap */
          position_set_to_pos (&ap->pos, &ap_pos);
        }
    }

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
      curr_ticks =
        position_to_ticks (&region->start_pos);
      position_from_ticks (
        &region->start_pos,
        pos_ticks + DIFF);
      curr_ticks =
        position_to_ticks (&region->end_pos);
      position_from_ticks (
        &region->end_pos,
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
        region_clone (
          region, REGION_CLONE_COPY_MAIN);
      region_print (cp);
      track_add_region (
        cp->lane->track, cp, 0, F_GEN_NAME);
    }
  for (i = 0; i < ts->num_aps; i++)
    {
      AutomationPoint * ap =
        ts->aps[i];

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
