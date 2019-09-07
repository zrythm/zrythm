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

#include "audio/audio_region.h"
#include "audio/automation_region.h"
#include "audio/chord_region.h"
#include "audio/channel.h"
#include "audio/midi_note.h"
#include "audio/midi_region.h"
#include "audio/instrument_track.h"
#include "audio/region.h"
#include "audio/track.h"
#include "ext/audio_decoder/ad.h"
#include "gui/widgets/audio_region.h"
#include "gui/widgets/automation_region.h"
#include "gui/widgets/bot_dock_edge.h"
#include "gui/widgets/center_dock.h"
#include "gui/widgets/chord_region.h"
#include "gui/widgets/clip_editor.h"
#include "gui/widgets/main_window.h"
#include "gui/widgets/midi_arranger.h"
#include "gui/widgets/midi_region.h"
#include "gui/widgets/region.h"
#include "gui/widgets/timeline_arranger.h"
#include "project.h"
#include "utils/arrays.h"
#include "utils/flags.h"
#include "utils/audio.h"
#include "utils/yaml.h"

#include <sndfile.h>
#include <samplerate.h>

DEFINE_START_POS

#define SET_POS(r,pos_name,pos,trans_only) \
  ARRANGER_OBJ_SET_POS_WITH_LANE ( \
    region, r, pos_name, pos, trans_only)

ARRANGER_OBJ_DEFINE_MOVABLE_W_LENGTH (
  Region, region, timeline_selections,
  TL_SELECTIONS);

/**
 * Only to be used by implementing structs.
 *
 * @param is_main Is main Region. If this is 1 then
 *   arranger_object_info_init_main() is called to
 *   create 3 additional regions in obj_info.
 */
void
region_init (
  Region *         self,
  const Position * start_pos,
  const Position * end_pos,
  const int        is_main)
{
  position_set_to_pos (&self->start_pos,
                       start_pos);
  position_set_to_pos (&self->end_pos,
                       end_pos);
  position_init (&self->clip_start_pos);
  long length =
    region_get_full_length_in_ticks (self);
  position_from_ticks (&self->true_end_pos,
                       length);
  position_init (&self->loop_start_pos);
  position_set_to_pos (&self->loop_end_pos,
                       &self->true_end_pos);
  self->linked_region_name = NULL;

  if (is_main)
    {
      ARRANGER_OBJECT_SET_AS_MAIN (
        REGION, Region, region);
    }
}

/**
 * Sets the track lane.
 */
void
region_set_lane (
  Region * region,
  TrackLane * lane)
{
  g_return_if_fail (lane);

  Region * r;
  for (int i = 0; i < 4; i++)
    {
      if (i == AOI_COUNTERPART_MAIN)
        r = region_get_main_region (region);
      else if (i == AOI_COUNTERPART_MAIN_TRANSIENT)
        r = region_get_main_trans_region (region);
      else if (i == AOI_COUNTERPART_LANE)
        r = region_get_lane_region (region);
      else if (i == AOI_COUNTERPART_LANE_TRANSIENT)
        r = region_get_lane_trans_region (region);

      r->lane = lane;
      r->lane_pos = lane->pos;
      r->track_pos = lane->track_pos;
    }
}

/**
 * Sets the automation track.
 */
void
region_set_automation_track (
  Region * region,
  AutomationTrack * at)
{
  g_return_if_fail (at);

  Region * r;
  for (int i = 0; i < 2; i++)
    {
      if (i == AOI_COUNTERPART_MAIN)
        r = region_get_main_region (region);
      else if (i == AOI_COUNTERPART_MAIN_TRANSIENT)
        r = region_get_main_trans_region (region);

      r->at = at;
      r->at_index = at->index;
      r->track_pos = at->track->pos;
    }
}

/**
 * Inits freshly loaded region.
 */
void
region_init_loaded (Region * self)
{
  self->linked_region =
    region_find_by_name (
      self->linked_region_name);

  int i;
  switch (self->type)
    {
    case REGION_TYPE_AUDIO:
      {
        /* reload audio */
        struct adinfo nfo;
        SRC_DATA src_data;
        long out_buff_size;

        audio_decode (
          &nfo, &src_data,
          &self->buff, &out_buff_size,
          self->filename);

        self->buff_size =
          src_data.output_frames_gen;
        self->channels = nfo.channels;
      }
      break;
    case REGION_TYPE_MIDI:
      {
        MidiNote * mn;
        for (i = 0; i < self->num_midi_notes; i++)
          {
            mn = self->midi_notes[i];
            mn->region = self;
            midi_note_init_loaded (mn);
          }
        self->midi_notes_size =
          self->num_midi_notes;
      }
      break;
    case REGION_TYPE_CHORD:
      {
        ChordObject * chord;
        for (i = 0; i < self->num_chord_objects;
             i++)
          {
            chord = self->chord_objects[i];
            chord_object_init_loaded (chord);
          }
        self->chord_objects_size =
          self->num_chord_objects;
        }
      break;
    case REGION_TYPE_AUTOMATION:
      {
        AutomationPoint * ap;
        AutomationCurve * ac;
        for (i = 0; i < self->num_aps; i++)
          {
            ap = self->aps[i];
            automation_point_init_loaded (ap);
          }
        for (i = 0; i < self->num_acs; i++)
          {
            ac = self->acs[i];
            automation_curve_init_loaded (ac);
          }
        self->aps_size =
          self->num_aps;
      }
      break;
    }

  ARRANGER_OBJECT_SET_AS_MAIN (
    REGION, Region, region);

  region_set_lane (self, self->lane);
}

/**
 * Finds the region corresponding to the given one.
 *
 * This should be called when we have a copy or a
 * clone, to get the actual region in the project.
 */
Region *
region_find (
  Region * clone)
{
  return region_find_by_name (clone->name);
}

/**
 * Returns the Track this Region is in.
 */
Track *
region_get_track (
  const Region * region)
{
  return TRACKLIST->tracks[region->track_pos];
}

/**
 * Looks for the Region under the given name.
 *
 * Warning: very expensive function.
 */
Region *
region_find_by_name (
  const char * name)
{
  int i, j, k;
  Track * track;
  AutomationTracklist * atl;
  AutomationTrack * at;
  TrackLane * lane;
  Region * r;
  for (i = 0; i < TRACKLIST->num_tracks; i++)
    {
      track = TRACKLIST->tracks[i];
      atl = &track->automation_tracklist;
      g_warn_if_fail (track);

      for (k = 0; k < track->num_lanes; k++)
        {
          lane = track->lanes[k];

          for (j = 0; j < lane->num_regions; j++)
            {
              r = lane->regions[j];
              if (!g_strcmp0 (r->name, name))
                return r;
            }
        }

      if (track->type == TRACK_TYPE_CHORD)
        {
          for (j = 0; j < track->num_chord_regions;
               j++)
            {
              r = track->chord_regions[j];
              if (!g_strcmp0 (r->name, name))
                return r;
            }
        }

      for (j = 0; j < atl->num_ats; j++)
        {
          at = atl->ats[j];

          for (k = 0; k < at->num_regions; k++)
            {
              r = at->regions[k];
              if (!g_strcmp0 (r->name, name))
                return r;
            }
        }
    }

  return NULL;
}

/**
 * Returns the MidiNote matching the properties of
 * the given MidiNote.
 *
 * Used to find the actual MidiNote in the region
 * from a cloned MidiNote (e.g. when doing/undoing).
 */
MidiNote *
region_find_midi_note (
  Region * r,
  MidiNote * clone)
{
  MidiNote * mn;
  for (int i = 0; i < r->num_midi_notes; i++)
    {
      mn = r->midi_notes[i];

      if (midi_note_is_equal (
            clone, mn))
        return mn;
    }

  return NULL;
}

/**
 * Generate a RegionWidget for the Region and all
 * its counterparts.
 */
void
region_gen_widget (
  Region * region)
{
  Region * r = region;
  for (int i = 0; i < 4; i++)
    {
      if (i == 0)
        r = region_get_main_region (r);
      else if (i == 1)
        r = region_get_main_trans_region (r);
      else if (i == 2)
        r = region_get_lane_region (r);
      else if (i == 3)
        r = region_get_lane_trans_region (r);

#define GEN_W(type,sc) \
  case REGION_TYPE_##type: \
    r->widget = \
      Z_REGION_WIDGET ( \
        sc##_region_widget_new (r)); \
    break

      switch (r->type)
        {
        GEN_W (MIDI, midi);
        GEN_W (AUDIO, audio);
        GEN_W (AUTOMATION, automation);
        GEN_W (CHORD, chord);
        }

#undef GEN_W
    }
}

ARRANGER_OBJ_DEFINE_POS_GETTER (
  Region, region, clip_start_pos);
ARRANGER_OBJ_DEFINE_POS_GETTER (
  Region, region, loop_start_pos);
ARRANGER_OBJ_DEFINE_POS_GETTER (
  Region, region, loop_end_pos);

/**
 * Used when a setter is needed.
 */
void
region_start_pos_setter (
  Region *         region,
  const Position * pos)
{
  region_set_start_pos (
    region, pos, AO_UPDATE_ALL);
}

/**
 * Returns the full length as it appears on the
 * timeline in ticks.
 */
long
region_get_full_length_in_ticks (
  Region * region)
{
  return position_to_ticks (&region->end_pos) -
    position_to_ticks (&region->start_pos);
}

/**
 * Returns the full length as it appears on the
 * timeline in frames.
 */
long
region_get_full_length_in_frames (
  const Region * region)
{
  return
    region->end_pos.frames -
    region->start_pos.frames;
}

/**
 * Updates the frames of each position in each child
 * of the region recursively.
 */
void
region_update_frames (
  Region * self)
{
  int i;

  position_update_frames (
    &self->start_pos);
  position_update_frames (
    &self->end_pos);
  position_update_frames (
    &self->true_end_pos);
  position_update_frames (
    &self->clip_start_pos);
  position_update_frames (
    &self->loop_start_pos);
  position_update_frames (
    &self->loop_end_pos);
  position_update_frames (
    &self->fade_in_pos);
  position_update_frames (
    &self->fade_out_pos);

  MidiNote * mn;
  for (i = 0; i < self->num_midi_notes; i++)
    {
      mn = self->midi_notes[i];
      midi_note_update_frames (mn);
    }
  for (i = 0; i < self->num_unended_notes; i++)
    {
      mn = self->unended_notes[i];
      midi_note_update_frames (mn);
    }

  AutomationPoint * ap;
  for (i = 0; i < self->num_aps; i++)
    {
      ap = self->aps[i];
      automation_point_update_frames (ap);
    }

  AutomationCurve * ac;
  for (i = 0; i < self->num_acs; i++)
    {
      ac = self->acs[i];
      automation_curve_update_frames (ac);
    }

  ChordObject * co;
  for (i = 0; i < self->num_chord_objects; i++)
    {
      co = self->chord_objects[i];
      chord_object_update_frames (co);
    }

}

ARRANGER_OBJ_DECLARE_RESET_COUNTERPART (
  Region, region)
{
  Region * src =
    reset_trans ?
      region_get_main_region (region) :
      region_get_main_trans_region (region);
  Region * dest =
    reset_trans ?
      region_get_main_trans_region (region) :
      region_get_main_region (region);

  position_set_to_pos (
    &dest->start_pos, &src->start_pos);
  position_set_to_pos (
    &dest->end_pos, &src->end_pos);
}

/**
 * Returns the length of the loop in ticks.
 */
long
region_get_loop_length_in_ticks (
  const Region * region)
{
  return
    position_to_ticks (&region->loop_end_pos) -
      position_to_ticks (&region->loop_start_pos);
}

/**
 * Returns the length of the loop in frames.
 */
long
region_get_loop_length_in_frames (
  const Region * region)
{
  return
    region->loop_end_pos.frames -
      region->loop_start_pos.frames;
}

/**
 * Returns the true length as it appears on the
 * piano roll (not taking into account any looping)
 * in ticks.
 */
long
region_get_true_length_in_ticks (
  Region * region)
{
  return position_to_ticks (&region->true_end_pos);
}

ARRANGER_OBJ_DECLARE_VALIDATE_POS (
  Region, region, end_pos)
{
  return
    position_is_after (
      pos, &region->start_pos);
}

/**
 * Sets the Track position to the Region and all its
 * members recursively.
 */
void
region_set_track_pos (
  Region *  region,
  const int pos)
{
  ARRANGER_OBJ_SET_PRIMITIVE_VAL (
    Region, region, track_pos, pos, AO_UPDATE_ALL);
}

/**
 * Print region info for debugging.
 */
void
region_print (
  const Region * self)
{
  char * str =
    g_strdup_printf (
      "%s [%s] - track pos %d - lane pos %d",
      self->name,
      region_type_bitvals[self->type].name,
      self->track_pos,
      self->lane_pos);
  g_message ("%s", str);
  g_free (str);
}

/**
 * Setter.
 */
void
region_loop_start_pos_setter (
  Region *   region,
  const Position * pos)
{
  region_set_loop_start_pos (
    region, pos, AO_UPDATE_ALL);
}

/**
 * Setter.
 */
void
region_loop_end_pos_setter (
  Region *   region,
  const Position * pos)
{
  region_set_loop_end_pos (
    region, pos, AO_UPDATE_ALL);
}

/**
 * Setter.
 */
void
region_clip_start_pos_setter (
  Region *   region,
  const Position * pos)
{
  region_set_clip_start_pos (
    region, pos, AO_UPDATE_ALL);
}

/**
 * Setter.
 */
void
region_end_pos_setter (
  Region *   region,
  const Position * pos)
{
  region_set_end_pos (
    region, pos, AO_UPDATE_ALL);
}

void
region_set_true_end_pos (
  Region * region,
  const Position * pos,
  ArrangerObjectUpdateFlag update_flag)
{
  SET_POS (region, true_end_pos, pos, update_flag);
}

void
region_set_loop_end_pos (
  Region * region,
  const Position * pos,
  ArrangerObjectUpdateFlag update_flag)
{
  /* validate */
  if (position_is_after (
        pos, &region->loop_start_pos))
    {
      SET_POS (region, loop_end_pos, pos,
               update_flag);
    }
}

/**
 * Checks if position is valid then sets it.
 */
void
region_set_loop_start_pos (
  Region * region,
  const Position * pos,
  ArrangerObjectUpdateFlag update_flag)
{
  /* validate */
  if (position_is_before (
        pos, &region->loop_end_pos) &&
      position_is_after_or_equal (
        pos, START_POS))
    {
      SET_POS (region, loop_start_pos, pos,
               update_flag);
    }
}

/**
 * Checks if position is valid then sets it.
 */
void
region_set_clip_start_pos (
  Region * region,
  const Position * pos,
  ArrangerObjectUpdateFlag update_flag)
{
  /* validate */
  if (position_is_before (
        pos, &region->loop_end_pos) &&
      position_is_after_or_equal (
        pos, START_POS))
    {
      SET_POS (region, clip_start_pos, pos,
               update_flag);
    }
}

/**
 * Splits the given Region at the given Position,
 * deletes the original Region and adds 2 new
 * Regions in the same parent (Track or
 * AutomationTrack).
 *
 * The given region must be the main region, as this
 * will create 2 new main regions.
 *
 * @param region The Region to split. This Region
 *   will be deleted.
 * @param pos The Position to split at.
 * @param pos_is_local If the position is local (1)
 *   or global (0).
 * @param r1 Address to hold the pointer to the
 *   newly created region 1.
 * @param r2 Address to hold the pointer to the
 *   newly created region 2.
 */
void
region_split (
  Region *         region,
  const Position * pos,
  const int        pos_is_local,
  Region **        r1,
  Region **        r2)
{
  /* create the new regions */
  *r1 =
    region_clone (region, REGION_CLONE_COPY_MAIN);
  *r2 =
    region_clone (region, REGION_CLONE_COPY_MAIN);

  /* get global/local positions (the local pos
   * is after traversing the loops) */
  Position globalp, localp;
  if (pos_is_local)
    {
      position_set_to_pos (&globalp, pos);
      position_add_ticks (
        &globalp, region->start_pos.total_ticks);
      position_set_to_pos (&localp, pos);
    }
  else
    {
      position_set_to_pos (&globalp, pos);
      long localp_frames =
        region_timeline_frames_to_local (
          region, globalp.frames, 1);
      position_from_frames (
        &localp, localp_frames);
    }

  /* for first region just set the end pos */
  region_set_end_pos (
    *r1, &globalp, AO_UPDATE_ALL);

  region_set_clip_start_pos (
    *r2, &localp, AO_UPDATE_ALL);
  region_set_start_pos (
    *r2, &globalp, AO_UPDATE_ALL);

  /* add them to the track */
  track_add_region (
    region_get_track (region),
    *r1, region->at, region->lane_pos, 1);
  track_add_region (
    region_get_track (region),
    *r2, region->at, region->lane_pos, 1);

  /* generate widgets so update visibility in the
   * arranger can work */
  region_gen_widget (*r1);
  region_gen_widget (*r2);

  /* select them */
  timeline_selections_clear (TL_SELECTIONS);
  timeline_selections_add_region (
    TL_SELECTIONS, *r1);
  timeline_selections_add_region (
    TL_SELECTIONS, *r2);

  /* change to r1 if the original region was the
   * clip editor region */
  if (CLIP_EDITOR->region == region)
    {
      clip_editor_set_region (
        CLIP_EDITOR, *r1);
    }

  /* remove and free the original region */
  track_remove_region (
    region_get_track (region),
    region, F_FREE);

  EVENTS_PUSH (ET_REGION_CREATED, *r1);
  EVENTS_PUSH (ET_REGION_CREATED, *r2);
}

/**
 * Undoes what region_split() did.
 *
 * @param r1 The first region.
 * @param r2 The second Region.
 * @param region An address to store the newly
 *   created Region.
 */
void
region_unsplit (
  Region *         r1,
  Region *         r2,
  Region **        region)
{
  /* create the new region */
  *region =
    region_clone (r1, REGION_CLONE_COPY_MAIN);

  /* set the end pos to the end pos of r2 */
  region_set_end_pos (
    *region, &r2->end_pos, AO_UPDATE_ALL);

  /* add it to the track */
  track_add_region (
    region_get_track (r1),
    *region, r1->at, r1->lane_pos, 1);

  /* generate widgets so update visibility in the
   * arranger can work */
  region_gen_widget (*region);

  /* select it */
  timeline_selections_clear (TL_SELECTIONS);
  timeline_selections_add_region (
    TL_SELECTIONS, *region);

  /* change to r1 if the original region was the
   * clip editor region */
  if (CLIP_EDITOR->region == r1)
    {
      clip_editor_set_region (
        CLIP_EDITOR, *region);
    }

  /* remove and free the original regions */
  track_remove_region (
    region_get_track (r1),
    r1, F_FREE);
  track_remove_region (
    region_get_track (r2),
    r2, F_FREE);

  EVENTS_PUSH (ET_REGION_CREATED, *region);
}

/**
 * Sets Region name (without appending anything to
 * it) to all associated regions.
 */
void
region_set_name (
  Region * region,
  char *   name)
{
  region_get_main_region (region)->name =
    g_strdup (name);
  region_get_lane_region (region)->name =
    g_strdup (name);
  region_get_main_trans_region (region)->name =
    g_strdup (name);
  region_get_lane_trans_region (region)->name =
    g_strdup (name);
}

ARRANGER_OBJ_DECLARE_VALIDATE_POS (
  Region, region, start_pos)
{
  return
    position_is_before (
      pos, &region->end_pos) &&
    position_is_after_or_equal (
      pos, START_POS);
}

/**
 * Returns the region at the given position in the
 * given Track.
 *
 * @param track The track to look in.
 * @param pos The position.
 */
Region *
region_at_position (
  Track    * track,
  Position * pos)
{
  int num_regions = 0;
  MidiRegion ** midi_regions;
  if (track->type == TRACK_TYPE_AUDIO)
    {

    }
  else if (track->type == TRACK_TYPE_INSTRUMENT)
    {
      /* FIXME using lane 0 */
      num_regions = track->lanes[0]->num_regions;
      midi_regions = track->lanes[0]->regions;
    }
  for (int i = 0; i < num_regions; i++)
    {
      Region * region = NULL;
      if (track->type == TRACK_TYPE_AUDIO)
        {

        }
      else if (track->type == TRACK_TYPE_INSTRUMENT)
        {
          region = midi_regions[i];
        }

      g_warn_if_fail (region);

      if (position_compare (pos,
                            &region->start_pos) >= 0 &&
          position_compare (pos,
                            &region->end_pos) <= 0)
        {
          return region;
        }
    }
  return NULL;
}

/**
 * Returns if the position is inside the region
 * or not.
 *
 * @param gframes Global position in frames.
 */
int
region_is_hit (
  const Region * region,
  const long     gframes)
{
  return
    region->start_pos.frames <=
      gframes &&
    region->end_pos.frames >
      gframes;
}

/**
 * Returns if any part of the Region is inside the
 * given range, inclusive.
 */
int
region_is_hit_by_range (
  const Region * region,
  const long     gframes_start,
  const long     gframes_end)
{
  /* 4 cases:
   * - region start is inside range
   * - region end is inside range
   * - start is inside region
   * - end is inside region
   */
  return
    (gframes_start <= region->start_pos.frames &&
     gframes_end > region->start_pos.frames) ||
    (gframes_start <= region->end_pos.frames &&
     gframes_end > region->end_pos.frames) ||
    region_is_hit (region, gframes_start) ||
    region_is_hit (region, gframes_end);
}

/**
 * Returns the number of loops in the region,
 * optionally including incomplete ones.
 */
int
region_get_num_loops (
  Region * region,
  int      count_incomplete_loops)
{
  int i = 0;
  long loop_size =
    region_get_loop_length_in_ticks (region);
  g_warn_if_fail (loop_size > 0);
  long full_size =
    region_get_full_length_in_ticks (region);
  long loop_start =
    position_to_ticks (&region->loop_start_pos) -
    position_to_ticks (&region->clip_start_pos);
  long curr_ticks = loop_start;

  while (curr_ticks < full_size)
    {
      i++;
      curr_ticks += loop_size;
    }

  if (!count_incomplete_loops)
    i--;

  /*if (count_incomplete_loops &&*/
      /*(curr_ticks - loop_start) % loop_size != 0)*/
    /*i++;*/

  return i;
}

/**
 * Copies the data from src to dest.
 *
 * Used when doing/undoing changes in name,
 * clip start point, loop start point, etc.
 */
void
region_copy (
  Region * src,
  Region * dest)
{
  g_free (dest->name);
  dest->name = g_strdup (src->name);

  region_set_clip_start_pos (
    dest, &src->clip_start_pos, AO_UPDATE_THIS);
  region_set_loop_start_pos (
    dest, &src->loop_start_pos, AO_UPDATE_THIS);
  region_set_loop_end_pos (
    dest, &src->loop_end_pos, AO_UPDATE_THIS);
}


/**
 * Clone region.
 *
 * Creates a new region and either links to the
 * original or copies every field.
 */
Region *
region_clone (
  Region *        region,
  RegionCloneFlag flag)
{
  int is_main = 0, i, j;
  if (flag == REGION_CLONE_COPY_MAIN)
    is_main = 1;

  Region * new_region = NULL;
  switch (region->type)
    {
    case REGION_TYPE_MIDI:
      {
        MidiRegion * mr =
          midi_region_new (
            &region->start_pos,
            &region->end_pos,
            is_main);
        MidiRegion * mr_orig = region;
        if (flag == REGION_CLONE_COPY ||
            flag == REGION_CLONE_COPY_MAIN)
          {
            for (i = 0;
                 i < mr_orig->num_midi_notes; i++)
              {
                MidiNote * mn =
                  midi_note_clone (
                    mr_orig->midi_notes[i],
                    MIDI_NOTE_CLONE_COPY_MAIN);

                midi_region_add_midi_note (
                  mr, mn);
              }
          }

        new_region = (Region *) mr;
      }
    break;
    case REGION_TYPE_AUDIO:
      {
        Region * ar =
          audio_region_new (
            region->filename,
            &region->start_pos,
            is_main);

        new_region = ar;
      }
    break;
    case REGION_TYPE_AUTOMATION:
      {
        Region * ar  =
          automation_region_new (
            &region->start_pos,
            &region->end_pos,
            is_main);
        Region * ar_orig = region;

        AutomationPoint * src_ap, * dest_ap;
        AutomationCurve * src_ac, * dest_ac;

        /* add automation points */
        for (j = 0; j < ar_orig->num_aps; j++)
          {
            src_ap = ar_orig->aps[j];
            dest_ap =
              automation_point_new_float (
                src_ap->fvalue,
                &src_ap->pos, F_MAIN);
            automation_region_add_ap (
              ar, dest_ap, 0);
          }

        /* add automation curves */
        for (j = 0; j < ar_orig->num_acs; j++)
          {
            src_ac = ar_orig->acs[j];
            dest_ac =
              automation_curve_new (
                ar_orig->at->automatable->type,
                &src_ac->pos);
            automation_region_add_ac (
              ar, dest_ac);
          }

        new_region = ar;
      }
      break;
    case REGION_TYPE_CHORD:
      {
        Region * cr =
          chord_region_new (
            &region->start_pos,
            &region->end_pos,
            is_main);
        Region * cr_orig = region;
        if (flag == REGION_CLONE_COPY ||
            flag == REGION_CLONE_COPY_MAIN)
          {
            ChordObject * co;
            for (i = 0;
                 i < cr_orig->num_chord_objects;
                 i++)
              {
                co =
                  chord_object_clone (
                    cr_orig->chord_objects[i],
                    CHORD_OBJECT_CLONE_COPY_MAIN);

                chord_region_add_chord_object (
                  cr, co);
              }
          }

        new_region = cr;
      }
      break;
    }

  /* set caches */
  position_set_to_pos (
    &new_region->cache_start_pos,
    &region->start_pos);
  position_set_to_pos (
    &new_region->cache_end_pos,
    &region->end_pos);

  /* set loop points */
  position_set_to_pos (
    &new_region->clip_start_pos,
    &region->clip_start_pos);
  position_set_to_pos (
    &new_region->loop_start_pos,
    &region->loop_start_pos);
  position_set_to_pos (
    &new_region->loop_end_pos,
    &region->loop_end_pos);

  /* clone name */
  new_region->name = g_strdup (region->name);

  /* set track to NULL and remember track pos */
  new_region->lane = NULL;
  new_region->track_pos = -1;
  new_region->lane_pos = region->lane_pos;
  if (region->lane)
    {
      new_region->track_pos =
        region->lane->track_pos;
    }
  else
    {
      new_region->track_pos = region->track_pos;
    }
  new_region->at_index = region->at_index;

  return new_region;
}

/**
 * Converts frames on the timeline (global)
 * to local frames (in the clip).
 *
 * If normalize is 1 it will only return a position
 * from 0 to loop_end (it will traverse the
 * loops to find the appropriate position),
 * otherwise it may exceed loop_end.
 *
 * @param timeline_frames Timeline position in
 *   frames.
 *
 * @return The local frames.
 */
long
region_timeline_frames_to_local (
  const Region * region,
  const long     timeline_frames,
  const int      normalize)
{
  long diff_frames;

  if (normalize)
    {
      if (region)
        {
          diff_frames =
            timeline_frames -
            region->start_pos.frames;
          long loop_end_frames =
            region->loop_end_pos.frames;
          long clip_start_frames =
            region->clip_start_pos.frames;
          long loop_size =
            region_get_loop_length_in_frames (
              region);

          diff_frames += clip_start_frames;

          while (diff_frames >= loop_end_frames)
            {
              diff_frames -= loop_size;
            }
        }
      else
        {
          diff_frames = 0;
        }

      return diff_frames;
    }
  else
    {
      if (region)
        {
          diff_frames =
            timeline_frames -
            region->start_pos.frames;
        }
      else
        {
          diff_frames = 0;
        }

      return diff_frames;
    }
}

/**
 * Returns the region with the earliest start point.
 */
Region *
region_get_start_region (Region ** regions,
                         int       num_regions)
{
  Position pos;
  Region * start_region = NULL;
  position_set_bar (&pos, TRANSPORT->total_bars + 1);
  for (int i = 0; i < num_regions; i++)
    {
      Region * r = regions[i];
      if (position_compare (&r->start_pos,
                            &pos) <= 0)
        {
          position_set_to_pos (&pos,
                               &r->start_pos);
          start_region = r;
        }
    }
  return start_region;
}

/**
 * Generates the filename for this region.
 *
 * MUST be free'd.
 *
 * FIXME logic needs changing
 */
char *
region_generate_filename (Region * region)
{
  return g_strdup_printf (REGION_PRINTF_FILENAME,
                          region->lane->track->name,
                          region->name);
}

/**
 * Disconnects the region and anything using it.
 *
 * Does not free the Region or its children's
 * resources.
 */
void
region_disconnect (
  Region * self)
{
  if (CLIP_EDITOR->region == self)
    {
      CLIP_EDITOR->region = NULL;
      EVENTS_PUSH (ET_CLIP_EDITOR_REGION_CHANGED,
                   NULL);
    }
  if (TL_SELECTIONS)
    {
      timeline_selections_remove_region (
        TL_SELECTIONS, self);
    }
  if (MW_TIMELINE->start_region == self)
    MW_TIMELINE->start_region = NULL;
}

void
region_free_all (Region * self)
{
  region_free (
    region_get_main_trans_region (self));
  region_free (
    region_get_lane_region (self));
  region_free (
    region_get_lane_trans_region (self));
  region_free (self);
}

void
region_free (Region * self)
{
  if (self->name)
    g_free (self->name);
  if (GTK_IS_WIDGET (self->widget))
    {
      RegionWidget * widget = self->widget;
      self->widget = NULL;
      region_widget_delete (widget);
    }

#define FREE_R(type,sc) \
  case REGION_TYPE_##type: \
    sc##_region_free_members (self); \
  break

  switch (self->type)
    {
      FREE_R (MIDI, midi);
      FREE_R (AUDIO, audio);
      FREE_R (CHORD, chord);
      FREE_R (AUTOMATION, automation);
    }

#undef FREE_R

  free (self);
}

SERIALIZE_SRC (Region, region)
DESERIALIZE_SRC (Region, region)
PRINT_YAML_SRC (Region, region)
