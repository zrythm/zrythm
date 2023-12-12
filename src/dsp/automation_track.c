// SPDX-FileCopyrightText: © 2018-2023 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <math.h>

#include "dsp/automation_point.h"
#include "dsp/automation_region.h"
#include "dsp/automation_track.h"
#include "dsp/control_port.h"
#include "dsp/instrument_track.h"
#include "dsp/track.h"
#include "gui/backend/event.h"
#include "gui/backend/event_manager.h"
#include "gui/widgets/arranger.h"
#include "gui/widgets/center_dock.h"
#include "gui/widgets/custom_button.h"
#include "gui/widgets/region.h"
#include "gui/widgets/timeline_arranger.h"
#include "gui/widgets/track.h"
#include "project.h"
#include "utils/arrays.h"
#include "utils/flags.h"
#include "utils/math.h"
#include "utils/mem.h"
#include "utils/objects.h"
#include "utils/string.h"
#include "zrythm_app.h"

#include <glib/gi18n.h>

static AutomationTrack *
_at_create (void)
{
  AutomationTrack * self = object_new (AutomationTrack);

  port_identifier_init (&self->port_id);

  self->schema_version = AUTOMATION_TRACK_SCHEMA_VERSION;

  return self;
}

void
automation_track_init_loaded (AutomationTrack * self, AutomationTracklist * atl)
{
  self->atl = atl;

  /* init regions */
  self->regions_size = (size_t) self->num_regions;
  int       i;
  ZRegion * region;
  for (i = 0; i < self->num_regions; i++)
    {
      region = self->regions[i];
      arranger_object_init_loaded ((ArrangerObject *) region);
    }
}

AutomationTrack *
automation_track_new (Port * port)
{
  AutomationTrack * self = _at_create ();

  self->regions_size = 1;
  self->regions = object_new_n (self->regions_size, ZRegion *);

  self->height = TRACK_DEF_HEIGHT;

  g_return_val_if_fail (port_identifier_validate (&port->id), NULL);
  port_identifier_copy (&self->port_id, &port->id);

  port->at = self;

  self->automation_mode = AUTOMATION_MODE_READ;

#if 0
  if (port->id.flags & PORT_FLAG_MIDI_AUTOMATABLE)
    {
      self->automation_mode =
        AUTOMATION_MODE_RECORD;
      self->record_mode =
        AUTOMATION_RECORD_MODE_TOUCH;
    }
#endif

  return self;
}

NONNULL bool
automation_track_validate (AutomationTrack * self)
{
  g_return_val_if_fail (
    /*self->schema_version == AUTOMATION_TRACK_SCHEMA_VERSION*/
    port_identifier_validate (&self->port_id), false);

  unsigned int track_name_hash = self->port_id.track_name_hash;
  if (self->port_id.owner_type == PORT_OWNER_TYPE_PLUGIN)
    {
      g_return_val_if_fail (
        self->port_id.plugin_id.track_name_hash == track_name_hash, false);
    }

  /* this is expensive so only do this during tests */
  if (ZRYTHM_TESTING)
    {
      AutomationTrack * found_at =
        automation_track_find_from_port_id (&self->port_id, !ZRYTHM_TESTING);
      if (found_at != self)
        {
          g_message (
            "The automation track for the following "
            "port identifier was not found");
          port_identifier_print (&self->port_id);
          g_message ("automation tracks:");
          AutomationTracklist * atl =
            automation_track_get_automation_tracklist (self);
          automation_tracklist_print_ats (atl);
          g_return_val_if_reached (false);
        }
    }

  for (int j = 0; j < self->num_regions; j++)
    {
      ZRegion * r = self->regions[j];
      g_return_val_if_fail (
        r->id.track_name_hash == track_name_hash && r->id.at_idx == self->index
          && r->id.idx == j,
        false);
      for (int k = 0; k < r->num_aps; k++)
        {
          AutomationPoint * ap = r->aps[k];
          ArrangerObject *  obj = (ArrangerObject *) ap;
          g_return_val_if_fail (
            obj->region_id.track_name_hash == track_name_hash, false);
        }
      g_return_val_if_fail (r->num_midi_notes == 0, false);
      g_return_val_if_fail (r->num_chord_objects == 0, false);
    }

  return true;
}

/**
 * Gets the automation mode as a localized string.
 */
void
automation_mode_get_localized (AutomationMode mode, char * buf)
{
  switch (mode)
    {
    case AUTOMATION_MODE_READ:
      strcpy (buf, _ ("On"));
      break;
    case AUTOMATION_MODE_RECORD:
      strcpy (buf, _ ("Rec"));
      break;
    case AUTOMATION_MODE_OFF:
      strcpy (buf, _ ("Off"));
      break;
    default:
      g_return_if_reached ();
    }
}

/**
 * Gets the automation mode as a localized string.
 */
void
automation_record_mode_get_localized (AutomationRecordMode mode, char * buf)
{
  switch (mode)
    {
    case AUTOMATION_RECORD_MODE_TOUCH:
      strcpy (buf, _ ("Touch"));
      break;
    case AUTOMATION_RECORD_MODE_LATCH:
      strcpy (buf, _ ("Latch"));
      break;
    default:
      g_return_if_reached ();
    }
}

/**
 * Adds an automation ZRegion to the AutomationTrack.
 *
 * @note This must not be used directly. Use
 *   track_add_region() instead.
 */
void
automation_track_add_region (AutomationTrack * self, ZRegion * region)
{
  automation_track_insert_region (self, region, self->num_regions);
}

/**
 * Inserts an automation ZRegion to the
 * AutomationTrack at the given index.
 *
 * @note This must not be used directly. Use
 *   track_insert_region() instead.
 */
void
automation_track_insert_region (AutomationTrack * self, ZRegion * region, int idx)
{
  g_return_if_fail (idx >= 0);
  g_return_if_fail (region->name && region->id.type == REGION_TYPE_AUTOMATION);

  array_double_size_if_full (
    self->regions, self->num_regions, self->regions_size, ZRegion *);
  for (int i = self->num_regions; i > idx; i--)
    {
      self->regions[i] = self->regions[i - 1];
      self->regions[i]->id.idx = i;
      region_update_identifier (self->regions[i]);
    }
  self->num_regions++;

  self->regions[idx] = region;
  region_set_automation_track (region, self);
  region->id.idx = idx;
  region_update_identifier (region);
}

AutomationTracklist *
automation_track_get_automation_tracklist (AutomationTrack * self)
{
  Track * track = automation_track_get_track (self);
  g_return_val_if_fail (track, NULL);
  return track_get_automation_tracklist (track);
}

/**
 * Returns the ZRegion that starts before
 * given Position, if any.
 *
 * @param ends_after Whether to only check for
 *   regions that also end after \ref pos (ie,
 *   the region surrounds \ref pos), otherwise
 *   get the region that ends last.
 */
ZRegion *
automation_track_get_region_before_pos (
  const AutomationTrack * self,
  const Position *        pos,
  bool                    ends_after,
  bool                    use_snapshots)
{
  ZRegion ** regions = use_snapshots ? self->region_snapshots : self->regions;
  int        num_regions =
    use_snapshots ? self->num_region_snapshots : self->num_regions;
  if (ends_after)
    {
      for (int i = num_regions - 1; i >= 0; i--)
        {
          ZRegion *        region = regions[i];
          ArrangerObject * r_obj = (ArrangerObject *) region;
          if (
            position_is_before_or_equal (&r_obj->pos, pos)
            && position_is_after_or_equal (&r_obj->end_pos, pos))
            return region;
        }
    }
  else
    {
      /* find latest region */
      ZRegion * latest_r = NULL;
      long      latest_distance = LONG_MIN;
      for (int i = num_regions - 1; i >= 0; i--)
        {
          ZRegion *        region = regions[i];
          ArrangerObject * r_obj = (ArrangerObject *) region;
          long distance_from_r_end = r_obj->end_pos.frames - pos->frames;
          if (
            position_is_before_or_equal (&r_obj->pos, pos)
            && distance_from_r_end > latest_distance)
            {
              latest_distance = distance_from_r_end;
              latest_r = region;
            }
        }
      return latest_r;
    }
  return NULL;
}

/**
 * Returns the automation point before the Position
 * on the timeline.
 *
 * @param ends_after Whether to only check in
 *   regions that also end after \ref pos (ie,
 *   the region surrounds \ref pos), otherwise
 *   check in the region that ends last.
 */
AutomationPoint *
automation_track_get_ap_before_pos (
  const AutomationTrack * self,
  const Position *        pos,
  bool                    ends_after,
  bool                    use_snapshots)
{
  ZRegion * r = automation_track_get_region_before_pos (
    self, pos, ends_after, use_snapshots);
  ArrangerObject * r_obj = (ArrangerObject *) r;

  if (!r || arranger_object_get_muted (r_obj, true))
    {
      return NULL;
    }

  /* if region ends before pos, assume pos is the
   * region's end pos */
  signed_frame_t local_pos = (signed_frame_t) region_timeline_frames_to_local (
    r,
    !ends_after && (r_obj->end_pos.frames < pos->frames)
      ? r_obj->end_pos.frames - 1
      : pos->frames,
    F_NORMALIZE);
  /*g_debug ("local pos %ld", local_pos);*/

  for (int i = r->num_aps - 1; i >= 0; i--)
    {
      AutomationPoint * ap = r->aps[i];
      ArrangerObject *  obj = (ArrangerObject *) ap;
      if (obj->pos.frames <= local_pos)
        return ap;
    }

  return NULL;
}

/**
 * Finds the AutomationTrack associated with `port`.
 *
 * FIXME use hashtable
 *
 * @param track The track that owns the port, if
 *   known.
 */
AutomationTrack *
automation_track_find_from_port (Port * port, Track * track, bool basic_search)
{
  if (!track)
    {
      track = port_get_track (port, 1);
    }
  g_return_val_if_fail (track, NULL);

  AutomationTracklist * atl = track_get_automation_tracklist (track);
  g_return_val_if_fail (atl, NULL);
  for (int i = 0; i < atl->num_ats; i++)
    {
      AutomationTrack * at = atl->ats[i];
      if (basic_search)
        {
          PortIdentifier * src = &port->id;
          PortIdentifier * dest = &at->port_id;
          if (
            dest->owner_type == src->owner_type
            && dest->type == src->type
            && dest->flow == src->flow
            && dest->flags == src->flags
            && dest->track_name_hash == src->track_name_hash
            &&
            (dest->sym
               ? string_is_equal (dest->sym, src->sym)
               : string_is_equal (dest->label, src->label)))
            {
              if (dest->owner_type == PORT_OWNER_TYPE_PLUGIN)
                {
                  if (!plugin_identifier_is_equal (
                        &dest->plugin_id, &src->plugin_id))
                    {
                      continue;
                    }

                  Plugin * pl = port_get_plugin (port, true);
                  g_return_val_if_fail (IS_PLUGIN_AND_NONNULL (pl), NULL);

                  if (pl->setting->descr->protocol == Z_PLUGIN_PROTOCOL_LV2)
                    {
                      /* if lv2 plugin port (not
                       * standard zrythm-provided
                       * port), make sure the symbol
                       * matches (some plugins have
                       * multiple ports with the same
                       * label but different
                       * symbol) */
                      if (
                        src->flags ^ PORT_FLAG_GENERIC_PLUGIN_PORT
                        && !string_is_equal (dest->sym, src->sym))
                        {
                          continue;
                        }
                      return at;
                    }
                  /* if not lv2, also search by
                   * index */
                  else if (dest->port_index == src->port_index)
                    {
                      return at;
                    }
                }
              else
                {
                  if (dest->port_index == src->port_index)
                    {
                      return at;
                    }
                }
            }
        }
      /* not basic search */
      else
        {
          if (port_identifier_is_equal (&port->id, &at->port_id))
            {
              return at;
            }
        }
    }

  return NULL;
}

/**
 * @note This is expensive and should only be used
 *   if \ref PortIdentifier.at_idx is not set. Use
 *   port_get_automation_track() instead.
 *
 * @param basic_search If true, only basic port
 *   identifier members are checked.
 */
AutomationTrack *
automation_track_find_from_port_id (PortIdentifier * id, bool basic_search)
{
  Port * port = port_find_from_identifier (id);
  g_return_val_if_fail (port && port_identifier_is_equal (id, &port->id), NULL);

  return automation_track_find_from_port (port, NULL, basic_search);
}

void
automation_track_set_automation_mode (
  AutomationTrack * self,
  AutomationMode    mode,
  bool              fire_events)
{
  g_return_if_fail (ZRYTHM_APP_IS_GTK_THREAD);

  AutomationTracklist * atl = automation_track_get_automation_tracklist (self);
  g_return_if_fail (atl);

  /* add to atl cache if recording */
  if (mode == AUTOMATION_MODE_RECORD)
    {
      bool already_added = false;
      for (int i = 0; i < atl->num_ats_in_record_mode; i++)
        {
          AutomationTrack * cur_at = atl->ats_in_record_mode[i];
          if (self == cur_at)
            {
              already_added = true;
              break;
            }
        }

      if (!already_added)
        {
          array_append (
            atl->ats_in_record_mode, atl->num_ats_in_record_mode, self);
        }
    }

  self->automation_mode = mode;

  if (fire_events)
    {
      EVENTS_PUSH (ET_AUTOMATION_TRACK_CHANGED, self);
    }
}

/**
 * Returns whether the automation in the automation
 * track should be read.
 *
 * @param cur_time Current time from
 *   g_get_monotonic_time() passed here to ensure
 *   the same timestamp is used in sequential calls.
 */
bool
automation_track_should_read_automation (AutomationTrack * at, gint64 cur_time)
{
  if (at->automation_mode == AUTOMATION_MODE_OFF)
    return false;

  /* TODO last argument should be true but doesnt
   * work properly atm */
  if (automation_track_should_be_recording (at, cur_time, false))
    return false;

  return true;
}

/**
 * Returns if the automation track should currently
 * be recording data.
 *
 * Returns false if in touch mode after the release
 * time has passed.
 *
 * This function assumes that the transport will
 * be rolling.
 *
 * @param cur_time Current time from
 *   g_get_monotonic_time() passed here to ensure
 *   the same timestamp is used in sequential calls.
 * @param record_aps If set to true, this function
 *   will return whether we should be recording
 *   automation point data. If set to false, this
 *   function will return whether we should be
 *   recording a region (eg, if an automation point
 *   was already created before and we are still
 *   recording inside a region regardless of whether
 *   we should create/edit automation points or not.
 */
bool
automation_track_should_be_recording (
  const AutomationTrack * const at,
  const gint64                  cur_time,
  const bool                    record_aps)
{
  if (G_LIKELY (at->automation_mode != AUTOMATION_MODE_RECORD))
    return false;

  if (at->record_mode == AUTOMATION_RECORD_MODE_LATCH)
    {
      /* in latch mode, we are always recording,
       * even if the value doesn't change
       * (an automation point will be created
       * as soon as latch mode is armed) and
       * then only when changes are made) */
      return true;
    }
  else if (at->record_mode == AUTOMATION_RECORD_MODE_TOUCH)
    {
      Port * port = at->port;
      g_return_val_if_fail (IS_PORT_AND_NONNULL (port), false);
      gint64 diff = cur_time - port->last_change;
      if (diff < AUTOMATION_RECORDING_TOUCH_REL_MS * 1000)
        {
          /* still recording */
          return true;
        }
      else if (!record_aps)
        return at->recording_started;
    }

  return false;
}

/**
 * Unselects all arranger objects.
 */
void
automation_track_unselect_all (AutomationTrack * self)
{
  for (int i = 0; i < self->num_regions; i++)
    {
      ZRegion * region = self->regions[i];
      arranger_object_select (
        (ArrangerObject *) region, false, false, F_NO_PUBLISH_EVENTS);
    }
}

/**
 * Removes a region from the automation track.
 */
void
automation_track_remove_region (AutomationTrack * self, ZRegion * region)
{
  g_return_if_fail (IS_REGION (region));

  ZRegion * clip_editor_region =
    CLIP_EDITOR ? clip_editor_get_region (CLIP_EDITOR) : NULL;

  array_delete (self->regions, self->num_regions, region);

  for (int i = region->id.idx; i < self->num_regions; i++)
    {
      ZRegion * r = self->regions[i];

      r->id.idx = i;
      region_update_identifier (r);
      if (r == clip_editor_region)
        {
          clip_editor_set_region (CLIP_EDITOR, r, F_NO_PUBLISH_EVENTS);
        }
    }
}

/**
 * Removes and frees all arranger objects
 * recursively.
 */
void
automation_track_clear (AutomationTrack * self)
{
  for (int i = self->num_regions - 1; i >= 0; i--)
    {
      ZRegion * region = self->regions[i];
      Track *   track = automation_track_get_track (self);
      g_return_if_fail (IS_TRACK_AND_NONNULL (track));
      track_remove_region (track, region, F_NO_PUBLISH_EVENTS, F_FREE);
    }
  self->num_regions = 0;
}

Track *
automation_track_get_track (AutomationTrack * self)
{
  Track * track = tracklist_find_track_by_name_hash (
    TRACKLIST, self->port_id.track_name_hash);
  g_return_val_if_fail (track, NULL);

  return track;
}

/**
 * Sets the index of the AutomationTrack in the
 * AutomationTracklist.
 */
void
automation_track_set_index (AutomationTrack * self, int index)
{
  self->index = index;

  for (int i = 0; i < self->num_regions; i++)
    {
      ZRegion * region = self->regions[i];
      g_return_if_fail (region);
      region->id.at_idx = index;
      region_update_identifier (region);
    }
}

/**
 * Returns the actual parameter value at the given position.
 *
 * If there is no automation point/curve during the position,
 * it returns the current value of the parameter it is
 * automating.
 *
 * @param normalized Whether to return the value normalized.
 * @param ends_after Whether to only check in regions that also
 *   end after \ref pos (ie, the region surrounds \ref pos),
 *   otherwise check in the region that ends last.
 * @param use_snapshots Whether to get the value from the
 *   snapshotted (cached) regions. This should be set to true
 *   when called during dsp playback. TODO unimplemented
 */
float
automation_track_get_val_at_pos (
  AutomationTrack * self,
  Position *        pos,
  bool              normalized,
  bool              ends_after,
  bool              use_snapshots)
{
  AutomationPoint * ap =
    automation_track_get_ap_before_pos (self, pos, ends_after, use_snapshots);
  ArrangerObject * ap_obj = (ArrangerObject *) ap;

  Port * port = port_find_from_identifier (&self->port_id);
  g_return_val_if_fail (port, 0.f);

  /* no automation points yet, return negative
   * (no change) */
  if (!ap)
    {
      return port_get_control_value (port, normalized);
    }

  /*ZRegion * region = arranger_object_get_region (ap_obj);*/
  ZRegion * region = automation_track_get_region_before_pos (
    self, pos, ends_after, use_snapshots);
  g_return_val_if_fail (IS_REGION_AND_NONNULL (region), 0.f);
  ArrangerObject * r_obj = (ArrangerObject *) region;

  /* if region ends before pos, assume pos is the
   * region's end pos */
  signed_frame_t localp = (signed_frame_t) region_timeline_frames_to_local (
    region,
    !ends_after && (r_obj->end_pos.frames < pos->frames)
      ? r_obj->end_pos.frames - 1
      : pos->frames,
    F_NORMALIZE);
  /*g_debug ("local pos %ld", localp);*/

  AutomationPoint * next_ap =
    automation_region_get_next_ap (region, ap, false, false);
  ArrangerObject * next_ap_obj = (ArrangerObject *) next_ap;

  /* return value at last ap */
  if (!next_ap)
    {
      if (normalized)
        {
          return ap->normalized_val;
        }
      else
        {
          return ap->fvalue;
        }
    }

  int   prev_ap_lower = ap->normalized_val <= next_ap->normalized_val;
  float cur_next_diff =
    (float) fabsf (ap->normalized_val - next_ap->normalized_val);

  /* ratio of how far in we are in the curve */
  signed_frame_t ap_frames = position_to_frames (&ap_obj->pos);
  signed_frame_t next_ap_frames = position_to_frames (&next_ap_obj->pos);
  double         ratio;
  signed_frame_t numerator = localp - ap_frames;
  signed_frame_t denominator = next_ap_frames - ap_frames;
  if (numerator == 0)
    {
      ratio = 0.0;
    }
  else if (G_UNLIKELY (denominator == 0))
    {
      g_warning ("denominator is 0. this should never happen");
      ratio = 1.0;
    }
  else
    {
      ratio = (double) numerator / (double) denominator;
    }
  g_return_val_if_fail (ratio >= 0, 0.f);

  float result =
    (float) automation_point_get_normalized_value_in_curve (ap, region, ratio);
  result = result * cur_next_diff;
  if (prev_ap_lower)
    result += ap->normalized_val;
  else
    result += next_ap->normalized_val;

  if (normalized)
    {
      return result;
    }
  else
    {
      return control_port_normalized_val_to_real (port, result);
    }
}

/**
 * Updates each position in each child of the
 * automation track recursively.
 *
 * @param from_ticks Whether to update the
 *   positions based on ticks (true) or frames
 *   (false).
 */
void
automation_track_update_positions (
  AutomationTrack * self,
  bool              from_ticks,
  bool              bpm_change)
{
  for (int i = 0; i < self->num_regions; i++)
    {
      arranger_object_update_positions (
        (ArrangerObject *) self->regions[i], from_ticks, bpm_change, NULL);
    }
}

CONST
static int
get_y_px_from_height_and_normalized_val (
  const float height,
  const float normalized_val)
{
  return (int) (height - normalized_val * height);
}

/**
 * Returns the y pixels from the value based on the
 * allocation of the automation track.
 */
int
automation_track_get_y_px_from_normalized_val (
  AutomationTrack * self,
  float             normalized_val)
{
  return get_y_px_from_height_and_normalized_val (
    (float) self->height, normalized_val);
}

/**
 * Gets the last ZRegion in the AutomationTrack.
 *
 * FIXME cache.
 */
ZRegion *
automation_track_get_last_region (AutomationTrack * self)
{
  Position pos;
  position_init (&pos);
  ZRegion *        region, *last_region = NULL;
  ArrangerObject * r_obj;
  for (int i = 0; i < self->num_regions; i++)
    {
      region = self->regions[i];
      r_obj = (ArrangerObject *) region;
      if (position_is_after (&r_obj->end_pos, &pos))
        {
          last_region = region;
          position_set_to_pos (&pos, &r_obj->end_pos);
        }
    }
  return last_region;
}

bool
automation_track_verify (AutomationTrack * self)
{
  for (int i = 0; i < self->num_regions; i++)
    {
      ZRegion * r = self->regions[i];

      for (int j = 0; j < r->num_aps; j++)
        {
          AutomationPoint * ap = r->aps[j];

          if (ZRYTHM_TESTING)
            {
              if (
                !math_assert_nonnann (ap->fvalue)
                || !math_assert_nonnann (ap->normalized_val))
                {
                  return false;
                }
            }
        }
    }
  return true;
}

void
automation_track_set_caches (AutomationTrack * self, CacheTypes types)
{
  if (types & CACHE_TYPE_PLAYBACK_SNAPSHOTS)
    {
      for (int i = 0; i < self->num_region_snapshots; i++)
        {
          arranger_object_free ((ArrangerObject *) self->region_snapshots[i]);
        }
      self->num_region_snapshots = 0;
      self->region_snapshots = g_realloc_n (
        self->region_snapshots, (size_t) self->num_regions, sizeof (ZRegion *));
      for (int i = 0; i < self->num_regions; i++)
        {
          self->region_snapshots[i] = (ZRegion *) arranger_object_clone (
            (ArrangerObject *) self->regions[i]);
          self->num_region_snapshots++;
        }
    }

  if (types & CACHE_TYPE_AUTOMATION_LANE_PORTS)
    {
      self->port = port_find_from_identifier (&self->port_id);
    }
}

bool
automation_track_contains_automation (const AutomationTrack * self)
{
  return self->num_regions > 0;
}

/**
 * Clones the AutomationTrack.
 */
AutomationTrack *
automation_track_clone (AutomationTrack * src)
{
  AutomationTrack * dest = _at_create ();

  dest->regions_size = (size_t) src->num_regions;
  dest->num_regions = src->num_regions;
  dest->regions = object_new_n (dest->regions_size, ZRegion *);
  dest->visible = src->visible;
  dest->created = src->created;
  dest->index = src->index;
  dest->y = src->y;
  dest->automation_mode = src->automation_mode;
  dest->record_mode = src->record_mode;
  dest->height = src->height;
  g_warn_if_fail (dest->height >= TRACK_MIN_HEIGHT);

  port_identifier_copy (&dest->port_id, &src->port_id);

  ZRegion * src_region;
  for (int j = 0; j < src->num_regions; j++)
    {
      src_region = src->regions[j];
      dest->regions[j] =
        (ZRegion *) arranger_object_clone ((ArrangerObject *) src_region);
    }

  return dest;
}

void
automation_track_free (AutomationTrack * self)
{
  for (int i = 0; i < self->num_regions; i++)
    {
      object_free_w_func_and_null_cast (
        arranger_object_free, ArrangerObject *, self->regions[i]);
    }
  object_zero_and_free (self->regions);
  for (int i = 0; i < self->num_region_snapshots; i++)
    {
      object_free_w_func_and_null_cast (
        arranger_object_free, ArrangerObject *, self->region_snapshots[i]);
    }
  object_zero_and_free (self->region_snapshots);

  port_identifier_free_members (&self->port_id);

  object_zero_and_free (self);
}
