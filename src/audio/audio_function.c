/*
 * Copyright (C) 2020 Alexandros Theodotou <alex at zrythm dot org>
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

#include "audio/engine.h"
#include "audio/audio_function.h"
#include "audio/audio_region.h"
#include "gui/backend/arranger_selections.h"
#include "gui/backend/event.h"
#include "gui/backend/event_manager.h"
#include "project.h"
#include "settings/settings.h"
#include "utils/dsp.h"
#include "utils/string.h"
#include "zrythm_app.h"

/**
 * Applies the given action to the given selections.
 *
 * This will save a file in the pool and store the
 * pool ID in the selections.
 *
 * @param sel Selections to edit.
 * @param type Function type. If invalid is passed,
 *   this will simply add the audio file in the pool
 *   for the unchanged audio material (used in
 *   audio selection actions for the selections
 *   before the change).
 */
int
audio_function_apply (
  ArrangerSelections * sel,
  AudioFunctionType    type)
{
  g_message (
    "applying %s...",
    audio_function_type_to_string (type));

  AudioSelections * audio_sel =
    (AudioSelections *) sel;

  ZRegion * r = region_find (&audio_sel->region_id);
  g_return_val_if_fail (r, -1);
  Track * tr =
    arranger_object_get_track ((ArrangerObject *) r);
  g_return_val_if_fail (tr, -1);
  AudioClip * orig_clip =  audio_region_get_clip (r);
  g_return_val_if_fail (orig_clip, -1);

  Position init_pos;
  position_init (&init_pos);
  if (position_is_before (
        &audio_sel->sel_start, &r->base.pos) ||
      position_is_after (
        &audio_sel->sel_end, &r->base.end_pos))
    {
      g_warning (
        "invalid positions - skipping function");
      position_print (&audio_sel->sel_start);
      position_print (&audio_sel->sel_end);
      return -1;
    }

  /* adjust the positions */
  Position start, end;
  position_set_to_pos (
    &start, &audio_sel->sel_start);
  position_set_to_pos (
    &end, &audio_sel->sel_end);
  position_add_frames (
    &start, - r->base.pos.frames);
  position_add_frames (
    &end, - r->base.pos.frames);

  /* create a copy of the frames to be replaced */
  size_t num_frames =
    (size_t) (end.frames - start.frames);
  g_debug ("num frames %zu", num_frames);

  /* interleaved frames */
  size_t channels = orig_clip->channels;
  float frames[num_frames * channels];
  dsp_copy (
    &frames[0],
    &orig_clip->frames[start.frames * (long) channels],
    num_frames * channels);

  switch (type)
    {
    case AUDIO_FUNCTION_INVERT:
      dsp_mul_k2 (
        &frames[0], -1.f, num_frames * channels);
      break;
    case AUDIO_FUNCTION_NORMALIZE:
      /* peak-normalize */
      {
        float abs_peak = 0.f;
        dsp_abs_max (
          &frames[0], &abs_peak,
          num_frames * channels);
        dsp_mul_k2 (
          &frames[0], 1.f / abs_peak,
          num_frames * channels);
      }
      break;
    case AUDIO_FUNCTION_REVERSE:
      for (size_t i = 0; i < num_frames; i++)
        {
          for (size_t j = 0; j < channels; j++)
            {
              frames[i * channels + j] =
                orig_clip->frames[
                  ((size_t) start.frames +
                   ((num_frames - i) - 1)) *
                    channels + j];
            }
        }
      break;
    case AUDIO_FUNCTION_INVALID:
      /* do nothing */
      break;
    default:
      g_warning ("not implemented");
      break;
    }

  char * tmp =
    g_strdup_printf (
      "%s - %s - %s",
      tr->name, r->name,
      audio_function_type_to_string (type));

  /* remove dots from name */
  char * name = string_replace (tmp, ".", "_");
  g_free (tmp);

  AudioClip * clip =
    audio_clip_new_from_float_array (
      &frames[0], (long) num_frames,
      channels, name);
  g_free (name);
  audio_pool_add_clip (AUDIO_POOL, clip);
  g_message (
    "writing %s to pool (id %d)",
    clip->name, clip->pool_id);
  audio_clip_write_to_pool (clip, false);

  audio_sel->pool_id = clip->pool_id;

  if (type != AUDIO_FUNCTION_INVALID)
    {
      /* replace the frames in the region */
      audio_region_replace_frames (
        r, frames, (size_t) start.frames,
        num_frames);
    }

  if (!ZRYTHM_TESTING)
    {
      /* set last action */
      g_settings_set_int (
        S_UI, "audio-function", type);
    }

  EVENTS_PUSH (ET_EDITOR_FUNCTION_APPLIED, NULL);

  return 0;
}
