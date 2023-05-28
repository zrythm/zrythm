// SPDX-FileCopyrightText: © 2018-2023 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <inttypes.h>
#include <math.h>

#include "audio/engine.h"
#include "audio/position.h"
#include "audio/snap_grid.h"
#include "audio/tempo_track.h"
#include "audio/transport.h"
#include "gui/widgets/arranger.h"
#include "gui/widgets/bot_dock_edge.h"
#include "gui/widgets/center_dock.h"
#include "gui/widgets/editor_ruler.h"
#include "gui/widgets/main_window.h"
#include "gui/widgets/midi_arranger.h"
#include "gui/widgets/ruler.h"
#include "gui/widgets/timeline_arranger.h"
#include "gui/widgets/timeline_ruler.h"
#include "gui/widgets/top_bar.h"
#include "project.h"
#include "utils/algorithms.h"
#include "utils/math.h"
#include "utils/objects.h"

#include <gtk/gtk.h>

PURE static int
position_cmpfunc (const void * a, const void * b)
{
  const Position * posa = (Position const *) a;
  const Position * posb = (Position const *) b;
  return (
    int) CLAMP (position_compare_frames (posa, posb), -1, 1);
}

void
position_sort_array (Position * array, const size_t size)
{
  qsort (array, size, sizeof (Position), position_cmpfunc);
}

/**
 * Updates ticks.
 *
 * @param ticks_per_frame If zero, AudioEngine.ticks_per_frame
 *   will be used instead.
 */
void
position_update_ticks_from_frames (
  Position * self,
  double     ticks_per_frame)
{
  if (math_doubles_equal (ticks_per_frame, 0.0))
    {
      ticks_per_frame = AUDIO_ENGINE->ticks_per_frame;
    }
  g_return_if_fail (ticks_per_frame > 0);
  self->ticks = (double) self->frames * ticks_per_frame;
}

/**
 * Converts ticks to frames.
 */
signed_frame_t
position_get_frames_from_ticks (
  double ticks,
  double frames_per_tick)
{
  if (math_doubles_equal (frames_per_tick, 0.0))
    {
      frames_per_tick = AUDIO_ENGINE->frames_per_tick;
    }
  g_return_val_if_fail (frames_per_tick > 0, -1);
  return math_round_double_to_signed_frame_t (
    (ticks * frames_per_tick));
}

/**
 * Updates frames.
 *
 * @param frames_per_tick If zero, AudioEngine.frames_per_tick
 *   will be used instead.
 */
void
position_update_frames_from_ticks (
  Position * self,
  double     frames_per_tick)
{
  self->frames = position_get_frames_from_ticks (
    self->ticks, frames_per_tick);
}

/**
 * Sets position to given bar.
 */
void
position_set_to_bar (Position * self, int bar)
{
  g_return_if_fail (
    TRANSPORT->ticks_per_bar > 0
    /* don't use INT_MAX, it results in a negative
     * position */
    && bar <= POSITION_MAX_BAR);

  position_init (self);

  if (bar > 0)
    {
      bar--;
      self->ticks = TRANSPORT->ticks_per_bar * bar;
      position_from_ticks (self, self->ticks);
    }
  else if (bar < 0)
    {
      bar++;
      self->ticks = TRANSPORT->ticks_per_bar * bar;
      position_from_ticks (self, self->ticks);
    }
  else
    {
      g_return_if_reached ();
    }
}

/**
 * Adds the frames to the position and updates
 * the rest of the fields, and makes sure the
 * frames are still accurate.
 */
void
position_add_frames (Position * pos, const signed_frame_t frames)
{
  pos->frames += frames;
  position_update_ticks_from_frames (pos, 0.0);
}

/**
 * Returns the Position in milliseconds.
 */
signed_ms_t
position_to_ms (const Position * pos)
{
  if (position_to_frames (pos) == 0)
    {
      return 0;
    }
  return math_round_double_to_signed_frame_t (
    (1000.0 * (double) position_to_frames (pos))
    / ((double) AUDIO_ENGINE->sample_rate));
}

signed_frame_t
position_ms_to_frames (const double ms)
{
  return math_round_double_to_signed_frame_t (
    (ms / 1000.0) * (double) AUDIO_ENGINE->sample_rate);
}

double
position_ms_to_ticks (const double ms)
{
  /* FIXME simplify - this is a roundabout way not suitable
   * for realtime calculations */
  const signed_frame_t frames = position_ms_to_frames (ms);
  Position             pos;
  position_from_frames (&pos, frames);
  return pos.ticks;
}

void
position_add_ms (Position * pos, const double ms)
{
  signed_frame_t frames = position_ms_to_frames (ms);
  position_add_frames (pos, frames);
}

void
position_add_minutes (Position * pos, int mins)
{
  signed_frame_t frames =
    position_ms_to_frames (mins * 60 * 1000);
  position_add_frames (pos, frames);
}

void
position_add_seconds (Position * pos, const signed_sec_t seconds)
{
  signed_frame_t frames =
    position_ms_to_frames (seconds * 1000);
  position_add_frames (pos, frames);
}

/**
 * Sets the end position to be 1 snap point away
 * from  the start pos.
 *
 * FIXME rename to something more meaningful.
 * @param start_pos Start Position.
 * @param end_pos Position to set.
 * @param snap The SnapGrid.
 */
void
position_set_min_size (
  const Position * start_pos,
  Position *       end_pos,
  SnapGrid *       snap)
{
  position_set_to_pos (end_pos, start_pos);
  position_add_ticks (
    end_pos, snap_grid_get_default_ticks (snap));
}

/**
 * Returns closest snap point.
 *
 * @param pos Position.
 * @param p1 Snap point 1.
 * @param p2 Snap point 2.
 */
PURE static inline Position *
closest_snap_point (
  const Position * const pos,
  Position * const       p1,
  Position * const       p2)
{
  if (pos->ticks - p1->ticks <= p2->ticks - pos->ticks)
    {
      return p1;
    }
  else
    {
      return p2;
    }
}

/**
 * Gets the previous snap point.
 *
 * @param pos The position to reference. Must be
 *   positive.
 * @param track Track, used when moving things in
 *   the timeline. If keep offset is on and this is
 *   passed, the objects in the track will be taken
 *   into account. If keep offset is on and this is
 *   NULL, all applicable objects will be taken into
 *   account. Not used if keep offset is off.
 * @param region Region, used when moving
 *   things in the editor. Same behavior as @ref
 *   track.
 * @param sg SnapGrid options.
 * @param prev_snap_point The position to set.
 *
 * @return Whether a snap point was found or not.
 */
HOT static bool
get_prev_snap_point (
  const Position * pos,
  Track *          track,
  ZRegion *        region,
  const SnapGrid * sg,
  Position *       prev_sp)
{
  if (pos->frames < 0 || pos->ticks < 0)
    {
      /* negative not supported, set to same
       * position */
      position_set_to_pos (prev_sp, pos);
      return false;
    }

  bool     snapped = false;
  Position snap_point;
  position_set_to_pos (&snap_point, pos);
  if (sg->snap_to_grid)
    {
      double snap_ticks = snap_grid_get_snap_ticks (sg);
      double ticks_from_prev = fmod (pos->ticks, snap_ticks);
      position_add_ticks (&snap_point, -ticks_from_prev);
      position_set_to_pos (prev_sp, &snap_point);
      snapped = true;
    }

  if (track)
    {
      for (int i = 0; i < track->num_lanes; i++)
        {
          TrackLane * lane = track->lanes[i];
          for (int j = 0; j < lane->num_regions; j++)
            {
              ZRegion *        r = lane->regions[j];
              ArrangerObject * r_obj = (ArrangerObject *) r;
              snap_point = r_obj->pos;
              if (
                position_is_before_or_equal (&snap_point, pos)
                && position_is_after (&snap_point, prev_sp))
                {
                  position_set_to_pos (prev_sp, &snap_point);
                  snapped = true;
                }
              snap_point = r_obj->end_pos;
              if (
                position_is_before_or_equal (&snap_point, pos)
                && position_is_after (&snap_point, prev_sp))
                {
                  position_set_to_pos (prev_sp, &snap_point);
                  snapped = true;
                }
            }
        }
    }
  else if (region)
    {
      /* TODO */
    }

  /* if no point to snap to, set to same position */
  if (!snapped)
    {
      position_set_to_pos (prev_sp, pos);
    }

  return snapped;
}

/**
 * Get next snap point.
 *
 * @param pos The position to reference. Must be
 *   positive.
 * @param track Track, used when moving things in
 *   the timeline. If keep offset is on and this is
 *   passed, the objects in the track will be taken
 *   into account. If keep offset is on and this is
 *   NULL, all applicable objects will be taken into
 *   account. Not used if keep offset is off.
 * @param region Region, used when moving
 *   things in the editor. Same behavior as @ref
 *   track.
 * @param sg SnapGrid options.
 * @param next_snap_point Position to set.
 *
 * @return Whether a snap point was found or not.
 */
HOT static bool
get_next_snap_point (
  const Position * pos,
  Track *          track,
  ZRegion *        region,
  const SnapGrid * sg,
  Position *       next_sp)
{
  if (pos->frames < 0 || pos->ticks < 0)
    {
      /* negative not supported, set to same
       * position */
      position_set_to_pos (next_sp, pos);
      return false;
    }

  bool     snapped = false;
  Position snap_point;
  position_set_to_pos (&snap_point, pos);
  if (sg->snap_to_grid)
    {
      double snap_ticks = snap_grid_get_snap_ticks (sg);
      double ticks_from_prev = fmod (pos->ticks, snap_ticks);
      double ticks_to_next = snap_ticks - ticks_from_prev;
      position_add_ticks (&snap_point, ticks_to_next);
      position_set_to_pos (next_sp, &snap_point);
      snapped = true;
    }

  if (track)
    {
      for (int i = 0; i < track->num_lanes; i++)
        {
          TrackLane * lane = track->lanes[i];
          for (int j = 0; j < lane->num_regions; j++)
            {
              ZRegion *        r = lane->regions[j];
              ArrangerObject * r_obj = (ArrangerObject *) r;
              snap_point = r_obj->pos;
              if (
                position_is_after (&snap_point, pos)
                && position_is_before (&snap_point, next_sp))
                {
                  position_set_to_pos (next_sp, &snap_point);
                  snapped = true;
                }
              snap_point = r_obj->end_pos;
              if (
                position_is_after (&snap_point, pos)
                && position_is_before (&snap_point, next_sp))
                {
                  position_set_to_pos (next_sp, &snap_point);
                  snapped = true;
                }
            }
        }
    }
  else if (region)
    {
      /* TODO */
    }

  /* if no point to snap to, set to same position */
  if (!snapped)
    {
      position_set_to_pos (next_sp, pos);
    }

  return snapped;
}

/**
 * Get closest snap point.
 *
 * @param pos Position to reference.
 * @param track Track, used when moving things in
 *   the timeline. If keep offset is on and this is
 *   passed, the objects in the track will be taken
 *   into account. If keep offset is on and this is
 *   NULL, all applicable objects will be taken into
 *   account. Not used if keep offset is off.
 * @param region Region, used when moving
 *   things in the editor. Same behavior as @ref
 *   track.
 * @param sg SnapGrid options.
 * @param closest_sp Position to set.
 *
 * @return Whether a snap point was found or not.
 */
static inline bool
get_closest_snap_point (
  const Position * pos,
  Track *          track,
  ZRegion *        region,
  const SnapGrid * sg,
  Position *       closest_sp)
{
  /* get closest snap point */
  Position prev_sp, next_sp;
  bool     prev_snapped =
    get_prev_snap_point (pos, track, region, sg, &prev_sp);
  bool next_snapped =
    get_next_snap_point (pos, track, region, sg, &next_sp);
  if (prev_snapped && next_snapped)
    {
      Position * closest_sp_ptr =
        closest_snap_point (pos, &prev_sp, &next_sp);
      *closest_sp = *closest_sp_ptr;
      return true;
    }
  else if (prev_snapped)
    {
      *closest_sp = prev_sp;
      return true;
    }
  else if (next_snapped)
    {
      *closest_sp = next_sp;
      return true;
    }
  else
    {
      *closest_sp = *pos;
      return false;
    }
}

/**
 * Snaps position using given options.
 *
 * @param start_pos The previous position (ie, the
 *   position the drag started at. This is only used
 *   when the "keep offset" setting is on.
 * @param pos Position to edit.
 * @param track Track, used when moving things in
 *   the timeline. If snap to events is on and this is
 *   passed, the objects in the track will be taken
 *   into account. If snap to events is on and this is
 *   NULL, all applicable objects will be taken into
 *   account. Not used if snap to events is off.
 * @param region Region, used when moving
 *   things in the editor. Same behavior as @ref
 *   track.
 * @param sg SnapGrid options.
 */
void
position_snap (
  const Position * start_pos,
  Position *       pos,
  Track *          track,
  ZRegion *        region,
  const SnapGrid * sg)
{
  g_return_if_fail (sg);

  /* this should only be called if snap is on.
   * the check should be done before calling */
  g_warn_if_fail (SNAP_GRID_ANY_SNAP (sg));

  /* position must be positive - only global
   * positions allowed */
  g_return_if_fail (pos->frames >= 0 && pos->ticks >= 0);

  if (!sg->snap_to_events)
    {
      region = NULL;
      track = NULL;
    }

  /* snap to grid with offset */
  if (sg->snap_to_grid_keep_offset)
    {
      /* get previous snap point from start pos */
      g_return_if_fail (start_pos);
      Position prev_sp_from_start_pos;
      position_init (&prev_sp_from_start_pos);
      get_prev_snap_point (
        start_pos, track, region, sg, &prev_sp_from_start_pos);

      /* get diff from previous snap point */
      double ticks_delta =
        start_pos->ticks - prev_sp_from_start_pos.ticks;

      /* add ticks to current pos and check the
       * closest snap point to the resulting
       * pos */
      position_add_ticks (pos, -ticks_delta);

      /* get closest snap point */
      Position closest_sp;
      bool     have_closest_sp = get_closest_snap_point (
        pos, track, region, sg, &closest_sp);
      if (have_closest_sp)
        {
          /* move to closest snap point */
          position_set_to_pos (pos, &closest_sp);
        }

      /* readd ticks */
      position_add_ticks (pos, ticks_delta);
    }
  /* else if snap to grid without offset */
  else
    {
      /* get closest snap point */
      Position closest_sp;
      get_closest_snap_point (
        pos, track, region, sg, &closest_sp);

      /* move to closest snap point */
      position_set_to_pos (pos, &closest_sp);
    }
}

/**
 * Converts seconds to position and puts the result
 * in the given Position.
 */
void
position_from_seconds (Position * position, double secs)
{
  position_from_ticks (
    position,
    (secs * (double) AUDIO_ENGINE->sample_rate)
      / (double) AUDIO_ENGINE->frames_per_tick);
}

/**
 * Sets position to the given total tick count.
 */
void
position_from_ticks (Position * pos, double ticks)
{
  pos->schema_version = POSITION_SCHEMA_VERSION;
  pos->ticks = ticks;
  position_update_frames_from_ticks (pos, 0.0);

  /* assert that no overflow occurred */
  if (ticks >= 0)
    g_return_if_fail (pos->frames >= 0);
  else
    g_return_if_fail (pos->frames <= 0);
}

void
position_from_frames (
  Position *           pos,
  const signed_frame_t frames)
{
  pos->schema_version = POSITION_SCHEMA_VERSION;
  pos->frames = frames;
  position_update_ticks_from_frames (pos, 0.0);
}

void
position_from_ms (Position * pos, const signed_ms_t ms)
{
  position_init (pos);
  position_add_ms (pos, ms);
}

void
position_from_bars (Position * pos, int bars)
{
  position_init (pos);
  position_add_bars (pos, bars);
}

void
position_add_ticks (Position * self, double ticks)
{
  position_from_ticks (self, self->ticks + ticks);
}

/**
 * Calculates the midway point between the two
 * positions and sets it on pos.
 *
 * @param pos Position to set to.
 */
inline void
position_get_midway_pos (
  Position * start_pos,
  Position * end_pos,
  Position * pos)
{
  double end_ticks, start_ticks, ticks_diff;
  start_ticks = position_to_ticks (start_pos);
  end_ticks = position_to_ticks (end_pos);
  ticks_diff = end_ticks - start_ticks;
  position_set_to_pos (pos, start_pos);
  position_add_ticks (pos, ticks_diff / 2.0);
}

/**
 * Returns the difference in ticks between the two
 * Position's, snapped based on the given SnapGrid
 * (if any).
 *
 * TODO, refactor, too confusing on what it's
 * supposed to do.
 *
 * @param end_pos End position.
 * @param start_pos Start Position.
 * @param sg SnapGrid to snap with, or NULL to not
 *   snap.
 */
double
position_get_ticks_diff (
  const Position * end_pos,
  const Position * start_pos,
  const SnapGrid * sg)
{
  double ticks_diff = end_pos->ticks - start_pos->ticks;
  int    is_negative = ticks_diff < 0.0;
  POSITION_INIT_ON_STACK (diff_pos);
  position_add_ticks (&diff_pos, fabs (ticks_diff));
  if (sg && SNAP_GRID_ANY_SNAP (sg))
    {
      position_snap (NULL, &diff_pos, NULL, NULL, sg);
    }
  ticks_diff = diff_pos.ticks;
  if (is_negative)
    ticks_diff = -ticks_diff;

  return ticks_diff;
}

/**
 * Creates a string in the form of "0.0.0.0" from
 * the given position.
 *
 * Must be free'd by caller.
 */
char *
position_to_string_alloc (const Position * pos)
{
  char buf[80];
  position_to_string (pos, buf);
  return g_strdup (buf);
}

void
position_to_string_full (
  const Position * pos,
  char *           buf,
  int              decimal_places)
{
  int    bars = position_get_bars (pos, true);
  int    beats = position_get_beats (pos, true);
  int    sixteenths = position_get_sixteenths (pos, true);
  double ticks = position_get_ticks (pos);
  g_return_if_fail (bars > -80000);
  if (ZRYTHM_TESTING)
    {
      sprintf (
        buf, "%d.%d.%d.%.*f (%" SIGNED_FRAME_FORMAT ")", bars,
        abs (beats), abs (sixteenths), decimal_places,
        fabs (ticks), pos->frames);
    }
  else
    {
      sprintf (
        buf, "%d.%d.%d.%.*f", bars, abs (beats),
        abs (sixteenths), decimal_places, fabs (ticks));
    }
}

/**
 * Creates a string in the form of "0.0.0.0" from
 * the given position.
 */
void
position_to_string (const Position * pos, char * buf)
{
  position_to_string_full (pos, buf, 4);
}

/**
 * Parses a position from the given string.
 *
 * @return Whether successful.
 */
bool
position_parse (Position * pos, const char * str)
{
  int   bars, beats, sixteenths;
  float ticksf;
  int   res = sscanf (
    str, "%d.%d.%d.%f", &bars, &beats, &sixteenths, &ticksf);
  if (res != 4 || res == EOF)
    return false;

  /* adjust for starting from 1 */
  if (bars > 0)
    bars--;
  else if (bars < 0)
    bars++;
  else
    return false;

  if (beats > 0)
    beats--;
  else if (beats < 0)
    beats++;
  else
    return false;

  if (sixteenths > 0)
    sixteenths--;
  else if (sixteenths < 0)
    sixteenths++;
  else
    return false;

  double ticks = (double) ticksf;

  int beats_per_bar =
    tempo_track_get_beats_per_bar (P_TEMPO_TRACK);
  int sixteenths_per_beat = TRANSPORT->sixteenths_per_beat;
  int total_sixteenths =
    bars * beats_per_bar * sixteenths_per_beat
    + beats * sixteenths_per_beat + sixteenths;
  double total_ticks =
    (double) (total_sixteenths) *TICKS_PER_SIXTEENTH_NOTE_DBL
    + ticks;
  position_from_ticks (pos, total_ticks);

  return true;
}

/**
 * Prints the Position in the "0.0.0.0" form.
 */
void
position_print (const Position * pos)
{
  char buf[140];
  position_to_string (pos, buf);
  g_message (
    "%s (%" SIGNED_FRAME_FORMAT
    " frames | "
    "%f ticks)",
    buf, pos->frames, pos->ticks);
}

void
position_print_range (
  const Position * pos,
  const Position * pos2)
{
  char buf[140];
  position_to_string (pos, buf);
  char buf2[140];
  position_to_string (pos2, buf2);
  g_message (
    "%s (%" SIGNED_FRAME_FORMAT
    ") - "
    "%s (%" SIGNED_FRAME_FORMAT
    ") "
    "<delta %" SIGNED_FRAME_FORMAT
    " frames "
    "%f ticks>",
    buf, pos->frames, buf2, pos2->frames,
    pos2->frames - pos->frames, pos2->ticks - pos->ticks);
}

/**
 * Returns the total number of beats.
 *
 * @param include_current Whether to count the
 *   current beat if it is at the beat start.
 */
int
position_get_total_bars (
  const Position * pos,
  bool             include_current)
{
  int bars = position_get_bars (pos, false);
  int cur_bars = position_get_bars (pos, true);

  if (include_current || bars == 0)
    {
      return bars;
    }

  /* if we are at the start of the bar, don't
   * count this bar */
  Position pos_at_bar;
  position_set_to_bar (&pos_at_bar, cur_bars);
  if (pos_at_bar.frames == pos->frames)
    {
      bars--;
    }

  return bars;
}

/**
 * Returns the total number of beats.
 *
 * @param include_current Whether to count the
 *   current beat if it is at the beat start.
 */
int
position_get_total_beats (
  const Position * pos,
  bool             include_current)
{
  int beats = position_get_beats (pos, false);
  int bars = position_get_bars (pos, false);

  int beats_per_bar =
    tempo_track_get_beats_per_bar (P_TEMPO_TRACK);
  int ret = beats + bars * beats_per_bar;

  if (include_current || ret == 0)
    {
      return ret;
    }

  Position tmp;
  position_from_ticks (
    &tmp, (double) ret * TRANSPORT->ticks_per_beat);
  if (tmp.frames == pos->frames)
    {
      ret--;
    }

  return ret;
}

/**
 * Returns the total number of sixteenths not
 * including the current one.
 */
int
position_get_total_sixteenths (
  const Position * pos,
  bool             include_current)
{
  int ret;
  if (pos->ticks >= 0)
    {
      ret = (int) math_round_double_to_signed_32 (
        floor (pos->ticks / TICKS_PER_SIXTEENTH_NOTE_DBL));
    }
  else
    {
      ret = (int) math_round_double_to_signed_32 (
        ceil (pos->ticks / TICKS_PER_SIXTEENTH_NOTE_DBL));
    }

  if (include_current || ret == 0)
    {
      return ret;
    }

  Position tmp;
  position_from_ticks (
    &tmp, (double) ret * TICKS_PER_SIXTEENTH_NOTE_DBL);
  if (tmp.frames == pos->frames)
    {
      ret--;
    }

  return ret;
}

/**
 * Changes the sign of the position.
 *
 * For example, 4.2.1.21 would become -4.2.1.21.
 */
void
position_change_sign (Position * pos)
{
  pos->ticks = -pos->ticks;
  pos->frames = -pos->frames;
}

/**
 * Gets the bars of the position.
 *
 * Ie, if the position is equivalent to 4.1.2.42,
 * this will return 4.
 *
 * @param start_at_one Start at 1 or -1 instead of
 *   0.
 */
int
position_get_bars (const Position * pos, bool start_at_one)
{
  g_return_val_if_fail (
    ZRYTHM && PROJECT && TRANSPORT
      && TRANSPORT->ticks_per_bar > 0,
    -1);

  double total_bars = pos->ticks / TRANSPORT->ticks_per_bar;
  if (total_bars >= 0.0)
    {
      int ret = (int) floor (total_bars);
      if (start_at_one)
        {
          ret++;
        }
      return ret;
    }
  else
    {
      int ret = (int) ceil (total_bars);
      if (start_at_one)
        {
          ret--;
        }
      return ret;
    }
}

/**
 * Gets the beats of the position.
 *
 * Ie, if the position is equivalent to 4.1.2.42,
 * this will return 1.
 *
 * @param start_at_one Start at 1 or -1 instead of
 *   0.
 */
int
position_get_beats (const Position * pos, bool start_at_one)
{
  g_return_val_if_fail (
    ZRYTHM && PROJECT && TRANSPORT
      && TRANSPORT->ticks_per_bar > 0
      && TRANSPORT->ticks_per_beat > 0 && P_TEMPO_TRACK,
    -1);

  int beats_per_bar =
    tempo_track_get_beats_per_bar (P_TEMPO_TRACK);
  g_return_val_if_fail (beats_per_bar > 0, -1);

  double total_bars = (double) position_get_bars (pos, false);
  double total_beats = pos->ticks / TRANSPORT->ticks_per_beat;
  total_beats -= (double) (total_bars * beats_per_bar);
  if (total_beats >= 0.0)
    {
      int ret = (int) floor (total_beats);
      if (start_at_one)
        {
          ret++;
        }
      return ret;
    }
  else
    {
      int ret = (int) ceil (total_beats);
      if (start_at_one)
        {
          ret--;
        }
      return ret;
    }
}

/**
 * Gets the sixteenths of the position.
 *
 * Ie, if the position is equivalent to 4.1.2.42,
 * this will return 2.
 *
 * @param start_at_one Start at 1 or -1 instead of
 *   0.
 */
int
position_get_sixteenths (const Position * pos, bool start_at_one)
{
  g_return_val_if_fail (
    ZRYTHM && PROJECT && TRANSPORT
      && TRANSPORT->sixteenths_per_beat > 0,
    -1);

  double total_beats =
    (double) position_get_total_beats (pos, true);
  /*g_message ("total beats %f", total_beats);*/
  double total_sixteenths =
    pos->ticks / TICKS_PER_SIXTEENTH_NOTE_DBL;
  /*g_message ("total sixteenths %f",*/
  /*total_sixteenths);*/
  total_sixteenths -=
    (double) (total_beats * TRANSPORT->sixteenths_per_beat);
  if (total_sixteenths >= 0.0)
    {
      int ret = (int) floor (total_sixteenths);
      if (start_at_one)
        {
          ret++;
        }
      return ret;
    }
  else
    {
      int ret = (int) ceil (total_sixteenths);
      if (start_at_one)
        {
          ret--;
        }
      return ret;
    }
}

/**
 * Gets the ticks of the position.
 *
 * Ie, if the position is equivalent to
 * 4.1.2.42.40124, this will return 42.40124.
 */
double
position_get_ticks (const Position * pos)
{
  double total_sixteenths =
    position_get_total_sixteenths (pos, true);
  /*g_debug ("total sixteenths %f", total_sixteenths);*/
  return pos->ticks
         - (total_sixteenths * TICKS_PER_SIXTEENTH_NOTE_DBL);
}

bool
position_validate (const Position * pos)
{
  if (!pos->schema_version)
    {
      return false;
    }

  return true;
}
