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

#include "audio/audio_region.h"
#include "audio/channel.h"
#include "audio/midi_note.h"
#include "audio/midi_region.h"
#include "audio/instrument_track.h"
#include "audio/region.h"
#include "audio/track.h"
#include "ext/audio_decoder/ad.h"
#include "gui/widgets/audio_region.h"
#include "gui/widgets/bot_dock_edge.h"
#include "gui/widgets/center_dock.h"
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
  POSITION_SET_ARRANGER_OBJ_POS_WITH_LANE ( \
    region, r, pos_name, pos, trans_only)

/**
 * Only to be used by implementing structs.
 *
 * @param is_main Is main Region. If this is 1 then
 *   arranger_object_info_init_main() is called to
 *   create 3 additional regions in obj_info.
 */
void
region_init (
  Region *   region,
  const RegionType type,
  const Position * start_pos,
  const Position * end_pos,
  const int        is_main)
{
  g_message ("creating region");
  position_set_to_pos (&region->start_pos,
                       start_pos);
  position_set_to_pos (&region->end_pos,
                       end_pos);
  position_init (&region->clip_start_pos);
  long length =
    region_get_full_length_in_ticks (region);
  position_from_ticks (&region->true_end_pos,
                       length);
  position_init (&region->loop_start_pos);
  position_set_to_pos (&region->loop_end_pos,
                       &region->true_end_pos);
  region->linked_region_name = NULL;
  region->type = type;
  if (type == REGION_TYPE_AUDIO)
    {
      region->widget = Z_REGION_WIDGET (
        audio_region_widget_new (region));
    }
  else if (type == REGION_TYPE_MIDI)
    {
      region->widget = Z_REGION_WIDGET (
        midi_region_widget_new (region));
    }

  if (is_main)
    {
      if (type == REGION_TYPE_MIDI)
        {
          /* set it as main */
          Region * main_trans =
            midi_region_new (
              start_pos, end_pos, 0);
          Region * lane =
            midi_region_new (
              start_pos, end_pos, 0);
          Region * lane_trans =
            midi_region_new (
              start_pos, end_pos, 0);
          arranger_object_info_init_main (
            region,
            main_trans,
            lane,
            lane_trans,
            AOI_TYPE_REGION);
        }
      else if (type == REGION_TYPE_AUDIO)
        {
          /* TODO */

        }
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
 * Inits freshly loaded region.
 */
void
region_init_loaded (Region * region)
{
  region->linked_region =
    region_find_by_name (
      region->linked_region_name);

  if (region->type == REGION_TYPE_AUDIO)
    {
      /* reload audio */
      struct adinfo nfo;
      SRC_DATA src_data;
      long out_buff_size;

      audio_decode (
        &nfo, &src_data,
        &region->buff, &out_buff_size,
        region->filename);

      region->buff_size =
        src_data.output_frames_gen;
      region->channels = nfo.channels;

      region->widget = Z_REGION_WIDGET (
        audio_region_widget_new (region));
    }
  else if (region->type == REGION_TYPE_MIDI)
    {
      region->widget = Z_REGION_WIDGET (
        midi_region_widget_new (region));
    }

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
  Region * region)
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
  TrackLane * lane;
  Region * r;
  for (i = 0; i < TRACKLIST->num_tracks; i++)
    {
      track = TRACKLIST->tracks[i];
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

void
region_set_cache_end_pos (
  Region * region,
  const Position * pos)
{
  SET_POS (region, cache_end_pos, pos, 0);
}

void
region_set_cache_start_pos (
  Region * region,
  Position * pos)
{
  SET_POS (region, cache_start_pos, pos, 0);
}

/**
 * Clamps position then sets it to its counterparts.
 *
 * To be used only when resizing. For moving,
 * use region_move().
 *
 * @param trans_only Only set the transient
 *   Position's.
 */
void
region_set_start_pos (
  Region * region,
  Position * pos,
  int        trans_only)
{
  if (position_to_ticks (&region->end_pos) -
      position_to_ticks (pos) >=
      TRANSPORT->lticks_per_beat)
    {
      SET_POS (region, start_pos, pos, trans_only);
    }
}

/**
 * Getter for start pos.
 */
void
region_get_start_pos (
  Region * region,
  Position * pos)
{
  position_set_to_pos (pos, &region->start_pos);
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
 * Returns the length of the loop in ticks.
 */
long
region_get_loop_length_in_ticks (
  Region * region)
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
  Region * region)
{
  return
    position_to_frames (&region->loop_end_pos) -
      position_to_frames (&region->loop_start_pos);
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

/**
 * Clamps position then sets it.
 *
 * To be used only when resizing. For moving,
 * use region_move().
 *
 * @param trans_only Only set the Position to the
 *   counterparts.
 */
void
region_set_end_pos (
  Region * region,
  Position * pos,
  int        trans_only)
{
  if (position_to_ticks (pos) -
      position_to_ticks (&region->start_pos) >=
      TRANSPORT->lticks_per_beat)
    {
      SET_POS (region, end_pos, pos, trans_only);
    }
}

void
region_set_true_end_pos (
  Region * region,
  Position * pos)
{
  SET_POS (region, true_end_pos, pos, 0);
}

void
region_set_loop_end_pos (
  Region * region,
  Position * pos)
{
  SET_POS (region, loop_end_pos, pos, 0);
}

/**
 * Checks if position is valid then sets it.
 */
void
region_set_loop_start_pos (
  Region * region,
  Position * pos)
{
  SET_POS (region, loop_start_pos, pos, 0);
}

/**
 * Checks if position is valid then sets it.
 */
void
region_set_clip_start_pos (
  Region * region,
  Position * pos)
{
  SET_POS (region, clip_start_pos, pos, 0);
}

/**
 * Returns if Region is in TimelineSelections.
 */
int
region_is_selected (Region * self)
{
  if (timeline_selections_contains_region (
        TL_SELECTIONS,
        region_get_main_region (self)))
    return 1;

  return 0;
}

/**
 * Moves the Region by the given amount of ticks.
 *
 * @param use_cached_pos Add the ticks to the cached
 *   Position instead of its current Position.
 * @param trans_only Only do transients.
 * @return Whether moved or not.
 */
int
region_move (
  Region * region,
  long     ticks,
  int      use_cached_pos,
  int      trans_only)
{
  Position tmp;
  int moved;
  POSITION_MOVE_BY_TICKS_W_LENGTH (
    tmp, use_cached_pos, region, ticks, moved,
    trans_only);

  return moved;
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
 */
int
region_is_hit (
  Region *   region,
  Position * pos) ///< global position
{
  return
    position_compare (
      &region->start_pos, pos) <= 0 &&
    position_compare (
      &region->end_pos, pos) > 0;
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
 * Shifts the Region by given number of ticks on x,
 * and delta number of visible tracks on y.
 */
void
region_shift (
  Region * self,
  long ticks,
  int  delta)
{
  if (ticks)
    {
      position_add_ticks (
        &self->start_pos,
        ticks);
      position_add_ticks (
        &self->end_pos,
        ticks);
    }
  if (delta)
    {
      // TODO
    }
}

/**
 * Resizes the region on the left side or right side
 * by given amount of ticks.
 *
 * @param left 1 to resize left side, 0 to resize right
 *   side.
 * @param ticks Number of ticks to resize.
 */
void
region_resize (
  Region * r,
  int      left,
  long     ticks)
{
  if (left)
    position_add_ticks (&r->start_pos, ticks);
  else
    position_add_ticks (&r->end_pos, ticks);
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
  int is_main = 0;
  if (flag == REGION_CLONE_COPY_MAIN)
    is_main = 1;

  Region * new_region = NULL;
  if (region->type == REGION_TYPE_MIDI)
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
          for (int i = 0;
               i < mr_orig->num_midi_notes; i++)
            {
              MidiNote * mn =
                midi_note_clone (
                  mr_orig->midi_notes[i],
                  MIDI_NOTE_CLONE_COPY_MAIN);

              midi_region_add_midi_note (mr, mn);
            }
        }

      new_region = (Region *) mr;
    }
  else if (region->type == REGION_TYPE_AUDIO)
    {
      Region * ar =
        audio_region_new (
          region->filename,
          &region->start_pos,
          is_main);

      new_region = ar;
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

  return new_region;
}

/**
 * Converts a position on the timeline (global)
 * to a local position (in the clip).
 *
 * If normalize is 1 it will only return a position
 * from 0.0.0.0 to loop_end (it will traverse the
 * loops to find the appropriate position),
 * otherwise it may exceed loop_end.
 *
 * @param timeline_pos Timeline position.
 * @param local_pos Position to fill.
 */
void
region_timeline_pos_to_local (
  Region *   region,
  Position * timeline_pos,
  Position * local_pos,
  int        normalize)
{
  long diff_ticks;

  if (normalize)
    {
      if (region)
        {
          diff_ticks =
            position_to_ticks (timeline_pos) -
            position_to_ticks (
              &region->start_pos);
          long loop_end_ticks =
            position_to_ticks (
              &region->loop_end_pos);
          long clip_start_ticks =
            position_to_ticks (
              &region->clip_start_pos);
          long loop_size =
            region_get_loop_length_in_ticks (
              region);

          diff_ticks += clip_start_ticks;

          while (diff_ticks >= loop_end_ticks)
            {
              diff_ticks -= loop_size;
            }
        }
      else
        {
          diff_ticks = 0;
        }
      position_from_ticks (local_pos, diff_ticks);
      return;
    }
  else
    {
      if (region)
        {
          diff_ticks =
            position_to_ticks (timeline_pos) -
            position_to_ticks (
              &region->start_pos);
        }
      else
        {
          diff_ticks = 0;
        }
      position_from_ticks (local_pos, diff_ticks);
      return;
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
      array_delete (
        TL_SELECTIONS->regions,
        TL_SELECTIONS->num_regions,
        self);
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
  if (self->type == REGION_TYPE_MIDI)
    midi_region_free_members (self);
  if (self->type == REGION_TYPE_AUDIO)
    audio_region_free_members (self);

  free (self);
}

SERIALIZE_SRC (Region, region)
DESERIALIZE_SRC (Region, region)
PRINT_YAML_SRC (Region, region)
