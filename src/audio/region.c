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
#include "gui/widgets/audio_region.h"
#include "gui/widgets/main_window.h"
#include "gui/widgets/midi_region.h"
#include "gui/widgets/region.h"
#include "project.h"
#include "utils/yaml.h"

/**
 * Only to be used by implementing structs.
 */
void
region_init (Region *   region,
             RegionType type,
             Track *    track,
             Position * start_pos,
             Position * end_pos)
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
  /*g_message ("loop start");*/
  /*position_print (&region->loop_start_pos);*/
  /*g_message ("loop end");*/
  position_set_to_pos (&region->loop_end_pos,
                       &region->true_end_pos);
  /*position_print (&region->loop_end_pos);*/
  /*g_message ("start pos");*/
  /*position_print (&region->start_pos);*/
  /*g_message ("end pos");*/
  /*position_print (&region->end_pos);*/
  region->track_id = track->id;
  region->track = track;
  Channel * chan = track_get_channel (track);
  region->name = g_strdup_printf ("%s (%d)",
                                  chan->name,
                                  region->id);
  region->linked_region_id = -1;
  region->type = type;
  if (type == REGION_TYPE_AUDIO)
    {
      AudioRegion * ar = (AudioRegion *) region;
      region->audio_region = ar;
      region->widget = Z_REGION_WIDGET (
        audio_region_widget_new (ar));
      ar->dummy = 1;
      region->midi_region =
        calloc (1,sizeof (MidiRegion));
      region->midi_region->dummy = 1;
    }
  else if (type == REGION_TYPE_MIDI)
    {
      MidiRegion * mr = (MidiRegion *) region;
      region->midi_region = mr;
      region->widget = Z_REGION_WIDGET (
        midi_region_widget_new (mr));
      mr->dummy = 1;
      region->audio_region =
        calloc (1,sizeof (AudioRegion));
      region->audio_region->dummy = 1;
    }
  project_add_region (region);
}

/**
 * Clamps position then sets it.
 */
void
region_set_start_pos (Region * region,
                      Position * pos,
                      int      moved) ///< region moved or not (to move notes as
                                          ///< well)
{
  Position prev;
  position_set_to_pos (&prev, &region->start_pos);

  position_set_to_pos (&region->start_pos,
                       pos);
  /*if (moved && region->type == REGION_TYPE_MIDI)*/
    /*{*/
      /*MidiRegion * midi_region = (MidiRegion *) region;*/
      /*int prev_frames = position_to_frames (&prev);*/
      /*int now_frames = position_to_frames (pos);*/
      /*int frames = now_frames - prev_frames;*/
      /*for (int i = 0; i < midi_region->num_midi_notes; i++)*/
        /*{*/
          /*MidiNote * note = midi_region->midi_notes[i];*/
          /*position_add_frames (&note->start_pos, frames);*/
          /*position_add_frames (&note->end_pos, frames);*/
        /*}*/
    /*}*/
  /*long length =*/
    /*region_get_full_length_in_ticks (region);*/
  /*position_from_ticks (&region->true_end_pos,*/
                       /*length);*/
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
 */
void
region_set_end_pos (Region * region,
                    Position * pos)
{
  position_set_to_pos (&region->end_pos,
                       pos);
}

/**
 * Checks if position is valid then sets it.
 */
void
region_set_true_end_pos (Region * region,
                         Position * pos)
{
  position_set_to_pos (&region->true_end_pos,
                       pos);
}

/**
 * Checks if position is valid then sets it.
 */
void
region_set_loop_end_pos (Region * region,
                         Position * pos)
{
  position_set_to_pos (&region->loop_end_pos,
                       pos);
}

/**
 * Checks if position is valid then sets it.
 */
void
region_set_loop_start_pos (Region * region,
                         Position * pos)
{
  position_set_to_pos (&region->loop_start_pos,
                       pos);
}

/**
 * Checks if position is valid then sets it.
 */
void
region_set_clip_start_pos (Region * region,
                         Position * pos)
{
  position_set_to_pos (&region->clip_start_pos,
                       pos);
}

/**
 * Returns the region at the given position in the given channel
 */
Region *
region_at_position (Track    * track, ///< the track to look in
                    Position * pos) ///< the position
{
  int num_regions = 0;
  MidiRegion ** midi_regions;
  if (track->type == TRACK_TYPE_AUDIO)
    {

    }
  else if (track->type == TRACK_TYPE_INSTRUMENT)
    {
      num_regions = ((InstrumentTrack *)track)->num_regions;
      midi_regions = ((InstrumentTrack *)track)->regions;
    }
  for (int i = 0; i < num_regions; i++)
    {
      Region * region;
      if (track->type == TRACK_TYPE_AUDIO)
        {

        }
      else if (track->type == TRACK_TYPE_INSTRUMENT)
        {
          region = (Region *)midi_regions[i];
        }

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
 * Clone region.
 *
 * Creates a new region and either links to the original or
 * copies every field.
 */
Region *
region_clone (Region *        region,
              RegionCloneFlag flag)
{
  Track * track =
    project_get_track (region->track_id);

  Region * new_region = NULL;
  if (region->type == REGION_TYPE_MIDI)
    {
      MidiRegion * mr =
        midi_region_new (track,
                         &region->start_pos,
                         &region->end_pos);
      MidiRegion * mr_orig = region->midi_region;
      if (flag == REGION_CLONE_COPY)
        {
          for (int i = 0; i < mr_orig->num_midi_notes; i++)
            {
              MidiNote * mn =
                midi_note_clone (mr_orig->midi_notes[i],
                                 mr);

              midi_region_add_midi_note (mr,
                                         mn);
            }
        }

      new_region = (Region *) mr;
    }
  else if (region->type == REGION_TYPE_AUDIO)
    {

    }

  new_region->selected = region->selected;
  new_region->cloned_from = region->id;

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
 */
void
region_timeline_pos_to_local (
  Region *   region, ///< the region
  Position * timeline_pos, ///< timeline position
  Position * local_pos, ///< position to fill
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
  Region * start_region;
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
 */
char *
region_generate_filename (Region * region)
{
  Channel * channel = track_get_channel (region->track);
  return g_strdup_printf (REGION_PRINTF_FILENAME,
                          region->id,
                          channel->name,
                          region->name);
}

void
region_free (Region * self)
{
  if (self->name)
    g_free (self->name);
  if (self->widget)
    gtk_widget_destroy (GTK_WIDGET (self->widget));
  if (self->midi_region)
    midi_region_free_members (self->midi_region);
  if (self->audio_region)
    audio_region_free_members (self->audio_region);

  free (self);
}

SERIALIZE_SRC (Region, region)
DESERIALIZE_SRC (Region, region)
PRINT_YAML_SRC (Region, region)
