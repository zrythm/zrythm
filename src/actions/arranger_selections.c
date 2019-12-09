/*
 * Copyright (C) 2019 Alexandros Theodotou <alex at zrythm dot org>
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

#include "actions/arranger_selections.h"
#include "audio/automation_region.h"
#include "audio/automation_track.h"
#include "audio/chord_region.h"
#include "audio/chord_track.h"
#include "audio/marker_track.h"
#include "audio/track.h"
#include "gui/backend/arranger_selections.h"
#include "gui/widgets/center_dock.h"
#include "project.h"
#include "utils/flags.h"
#include "utils/objects.h"
#include "utils/string.h"

#include <glib/gi18n.h>


static ArrangerSelectionsAction *
_create_action (
  ArrangerSelections * sel)
{
	ArrangerSelectionsAction * self =
    calloc (1, sizeof (ArrangerSelectionsAction));

  self->sel = arranger_selections_clone (sel);
  self->first_run = 1;

  return self;
}

static ArrangerSelections *
get_actual_arranger_selections (
  ArrangerSelectionsAction * self)
{
  switch (self->sel->type)
    {
    case ARRANGER_SELECTIONS_TYPE_TIMELINE:
      return
        (ArrangerSelections *)
        TL_SELECTIONS;
    case ARRANGER_SELECTIONS_TYPE_MIDI:
      return
        (ArrangerSelections *)
        MA_SELECTIONS;
    case ARRANGER_SELECTIONS_TYPE_AUTOMATION:
      return
        (ArrangerSelections *)
        AUTOMATION_SELECTIONS;
    case ARRANGER_SELECTIONS_TYPE_CHORD:
      return
        (ArrangerSelections *)
        CHORD_SELECTIONS;
		default:
			g_return_val_if_reached (NULL);
			break;
    }

  g_return_val_if_reached (NULL);
}

/**
 * Creates a new action for moving or duplicating
 * objects.
 *
 * @param move 1 to move, 0 to duplicate.
 */
UndoableAction *
arranger_selections_action_new_move_or_duplicate (
  ArrangerSelections * sel,
  const int            move,
  const long           ticks,
  const int            delta_chords,
  const int            delta_pitch,
  const int            delta_tracks,
  const int            delta_lanes)
{
  ArrangerSelectionsAction * self =
    _create_action (sel);
  UndoableAction * ua = (UndoableAction *) self;
  if (move)
    ua->type = UA_MOVE_ARRANGER_SELECTIONS;
  else
    ua->type = UA_DUPLICATE_ARRANGER_SELECTIONS;

  self->ticks = ticks;
  self->delta_chords = delta_chords;
  self->delta_lanes = delta_lanes;
  self->delta_tracks = delta_tracks;
  self->delta_pitch = delta_pitch;

  return ua;
}

/**
 * Creates a new action for creating/deleting objects.
 *
 * @param create 1 to create 0 to delte.
 */
UndoableAction *
arranger_selections_action_new_create_or_delete (
  ArrangerSelections * sel,
  const int            create)
{
  ArrangerSelectionsAction * self =
    _create_action (sel);
  UndoableAction * ua = (UndoableAction *) self;
  if (create)
    ua->type = UA_CREATE_ARRANGER_SELECTIONS;
  else
    ua->type = UA_DELETE_ARRANGER_SELECTIONS;

  return ua;
}

/**
 * Creates a new action for editing properties
 * of an object.
 *
 * @param sel_before The selections before the
 *   change.
 * @param sel_after The selections after the
 *   change.
 * @param type Indication of which field has changed.
 */
UndoableAction *
arranger_selections_action_new_edit (
  ArrangerSelections *             sel_before,
  ArrangerSelections *             sel_after,
  ArrangerSelectionsActionEditType type)
{
  ArrangerSelectionsAction * self =
    _create_action (sel_before);
  UndoableAction * ua = (UndoableAction *) self;
  ua->type = UA_EDIT_ARRANGER_SELECTIONS;

  self->sel_after =
    arranger_selections_clone (sel_after);
  self->edit_type = type;

  return ua;
}

/**
 * Creates a new action for splitting
 * ArrangerObject's.
 *
 * @param pos Global position to split at.
 */
UndoableAction *
arranger_selections_action_new_split (
  ArrangerSelections * sel,
  const Position *     pos)
{
  ArrangerSelectionsAction * self =
    _create_action (sel);
  UndoableAction * ua = (UndoableAction *) self;
  ua->type = UA_SPLIT_ARRANGER_SELECTIONS;

  self->pos = *pos;
  position_update_frames (&self->pos);

  return ua;
}

/**
 * Creates a new action for resizing
 * ArrangerObject's.
 *
 * @param ticks How many ticks to add to the resizing
 *   edge.
 */
UndoableAction *
arranger_selections_action_new_resize (
  ArrangerSelections *               sel,
  ArrangerSelectionsActionResizeType type,
  const long                         ticks)
{
  ArrangerSelectionsAction * self =
    _create_action (sel);
  UndoableAction * ua = (UndoableAction *) self;
  ua->type = UA_RESIZE_ARRANGER_SELECTIONS;

  self->resize_type = type;
  self->ticks = ticks;

  return ua;
}

/**
 * Creates a new action fro quantizing
 * ArrangerObject's.
 *
 * @param opts Quantize options.
 */
UndoableAction *
arranger_selections_action_new_quantize (
  ArrangerSelections * sel,
  QuantizeOptions *    opts)
{
  ArrangerSelectionsAction * self =
    _create_action (sel);
  UndoableAction * ua = (UndoableAction *) self;
  ua->type = UA_QUANTIZE_ARRANGER_SELECTIONS;

  self->quantized_sel =
    arranger_selections_clone (sel);
  self->opts = quantize_options_clone (opts);

  return ua;
}

/**
 * Adds the ArrangerOjbect where it belongs in the
 * Project (eg a Track).
 */
static void
add_object_to_project (
  ArrangerObject * obj)
{
  switch (obj->type)
    {
    case ARRANGER_OBJECT_TYPE_AUTOMATION_POINT:
      {
        AutomationPoint * ap =
          (AutomationPoint *) obj;

        /* find the region */
        ap->region =
          region_find_by_name (
            ap->region_name);
        g_return_if_fail (
          ap->region);

        /* add it to the region */
        automation_region_add_ap (
          ap->region, ap);
      }
      break;
    case ARRANGER_OBJECT_TYPE_CHORD_OBJECT:
      {
        ChordObject * chord =
          (ChordObject *) obj;

        /* find the region */
        chord->region =
          region_find_by_name (
            chord->region_name);
        g_return_if_fail (
          chord->region);

        /* add it to the region */
        chord_region_add_chord_object (
          chord->region, chord);
      }
      break;
    case ARRANGER_OBJECT_TYPE_MIDI_NOTE:
      {
        MidiNote * mn =
          (MidiNote *) obj;

        /* find the region */
        mn->region =
          region_find_by_name (
            mn->region_name);
        g_return_if_fail (
          mn->region);

        /* add it to the region */
        midi_region_add_midi_note (
          mn->region, mn);
      }
      break;
    case ARRANGER_OBJECT_TYPE_SCALE_OBJECT:
      {
        ScaleObject * scale =
          (ScaleObject *) obj;

        /* add it to the track */
        chord_track_add_scale (
          P_CHORD_TRACK, scale);
      }
      break;
    case ARRANGER_OBJECT_TYPE_MARKER:
      {
        Marker * marker =
          (Marker *) obj;

        /* add it to the track */
        marker_track_add_marker (
          P_MARKER_TRACK, marker);
      }
      break;
    case ARRANGER_OBJECT_TYPE_REGION:
      {
        Region * r =
          (Region *) obj;

        /* add it to track */
        Track * track =
          TRACKLIST->tracks[
            r->track_pos];
        switch (r->type)
          {
          case REGION_TYPE_AUTOMATION:
            {
              AutomationTrack * at =
                track->
                  automation_tracklist.
                    ats[r->at_index];
              track_add_region (
                track, r, at, -1,
                F_GEN_NAME,
                F_PUBLISH_EVENTS);
            }
            break;
          case REGION_TYPE_CHORD:
            track_add_region (
              P_CHORD_TRACK, r, NULL,
              -1, F_GEN_NAME,
              F_PUBLISH_EVENTS);
            break;
          default:
            track_add_region (
              track, r, NULL, r->lane_pos,
              F_GEN_NAME,
              F_PUBLISH_EVENTS);
            break;
          }
      }
      break;
    default:
      g_warn_if_reached ();
      break;
    }
}

static void
remove_object_from_project (
  ArrangerObject * obj)
{
  switch (obj->type)
    {
    case ARRANGER_OBJECT_TYPE_AUTOMATION_POINT:
      {
        AutomationPoint * ap =
          (AutomationPoint *) obj;
        automation_region_remove_ap (
          ap->region, ap, F_FREE);
      }
      break;
    case ARRANGER_OBJECT_TYPE_CHORD_OBJECT:
      {
        ChordObject * chord =
          (ChordObject *) obj;
        chord_region_remove_chord_object (
          chord->region, chord, F_FREE);
      }
      break;
    case ARRANGER_OBJECT_TYPE_REGION:
      {
        Region * r =
          (Region *) obj;
        track_remove_region (
          arranger_object_get_track (obj),
          r, F_PUBLISH_EVENTS, F_FREE);
      }
      break;
    case ARRANGER_OBJECT_TYPE_SCALE_OBJECT:
      {
        ScaleObject * scale =
          (ScaleObject *) obj;
        chord_track_remove_scale (
          P_CHORD_TRACK, scale,
          F_FREE);
      }
      break;
    case ARRANGER_OBJECT_TYPE_MARKER:
      {
        Marker * marker =
          (Marker *) obj;
        marker_track_remove_marker (
          P_MARKER_TRACK, marker, F_FREE);
      }
      break;
    case ARRANGER_OBJECT_TYPE_MIDI_NOTE:
      {
        MidiNote * mn =
          (MidiNote *) obj;
        midi_region_remove_midi_note (
          mn->region, mn, F_FREE,
          F_NO_PUBLISH_EVENTS);
      }
      break;
    default:
      break;
    }
}

/**
 * Does or undoes the action.
 *
 * @param _do 1 to do, 0 to undo.
 */
static int
do_or_undo_move (
	ArrangerSelectionsAction * self,
  const int                  _do)
{
  int size = 0;
  ArrangerObject ** objs =
    arranger_selections_get_all_objects (
      self->sel, &size);

  long ticks =
    _do ? self->ticks : - self->ticks;
  int delta_tracks =
    _do ? self->delta_tracks : - self->delta_tracks;
  int delta_lanes =
    _do ? self->delta_lanes : - self->delta_lanes;
  int delta_chords =
    _do ? self->delta_chords : - self->delta_chords;
  int delta_pitch =
    _do ? self->delta_pitch : - self->delta_pitch;

  if (!self->first_run)
    {
      for (int i = 0; i < size; i++)
        {
          /* get the actual object from the
           * project */
          ArrangerObject * obj =
            arranger_object_find (objs[i]);
          g_return_val_if_fail (obj, -1);

          if (ticks != 0)
            {
              /* shift the actual object */
              arranger_object_move (
                obj, ticks, F_NO_CACHED);

              /* also shift the copy */
              arranger_object_move (
                objs[i], ticks, F_NO_CACHED);
            }

          if (delta_tracks != 0)
            {
              g_return_val_if_fail (
                obj->type ==
                ARRANGER_OBJECT_TYPE_REGION, -1);

              Region * r = (Region *) obj;

              Track * track_to_move_to =
                tracklist_get_visible_track_after_delta (
                  TRACKLIST,
                  arranger_object_get_track (obj),
                  delta_tracks);

              /* shift the actual object by tracks */
              region_move_to_track (
                r, track_to_move_to);

              /* FIXME check is it okay to not shift
               * the clone too? */
            }

          if (delta_pitch != 0)
            {
              g_return_val_if_fail (
                obj->type ==
                  ARRANGER_OBJECT_TYPE_MIDI_NOTE, -1);

              MidiNote * mn =
                (MidiNote *) obj;

              /* shift the actual object */
              midi_note_shift_pitch (
                mn, delta_pitch);

              /* also shift the copy so they can
               * match */
              midi_note_shift_pitch (
                (MidiNote *) objs[i],
                delta_pitch);
            }

          if (delta_lanes != 0)
            {
              g_return_val_if_fail (
                obj->type ==
                ARRANGER_OBJECT_TYPE_REGION, -1);

              Region * r = (Region *) obj;

              Track * region_track =
                arranger_object_get_track (obj);
              int new_lane_pos =
                r->lane->pos + delta_lanes;
              g_return_val_if_fail (
                new_lane_pos >= 0, -1);
              track_create_missing_lanes (
                region_track, new_lane_pos);
              TrackLane * lane_to_move_to =
                region_track->lanes[new_lane_pos];

              /* shift the actual object by lanes */
              region_move_to_lane (
                r, lane_to_move_to);
            }

          if (delta_chords != 0)
            {
              /* TODO */
            }
        }
    }
  free (objs);

  ArrangerSelections * sel =
    get_actual_arranger_selections (self);
  EVENTS_PUSH (
    ET_ARRANGER_SELECTIONS_CHANGED, sel);

  self->first_run = 0;

  return 0;
}

static void
move_obj_by_tracks_and_lanes (
  ArrangerObject * obj,
  const int        tracks_diff,
  const int        lanes_diff)
{
  if (tracks_diff)
    {
      g_return_if_fail (
        obj->type ==
        ARRANGER_OBJECT_TYPE_REGION);

      Region * r = (Region *) obj;

      Track * track_to_move_to =
        tracklist_get_visible_track_after_delta (
          TRACKLIST,
          arranger_object_get_track (obj),
          tracks_diff);

      /* shift the actual object by tracks */
      region_move_to_track (
        r, track_to_move_to);
    }
  if (lanes_diff)
    {
      Region * r = (Region *) obj;

      Track * region_track =
        arranger_object_get_track (obj);
      int new_lane_pos =
        r->lane_pos + lanes_diff;
      g_return_if_fail (
        new_lane_pos >= 0);
      track_create_missing_lanes (
        region_track, new_lane_pos);
      TrackLane * lane_to_move_to =
        region_track->lanes[new_lane_pos];

      /* shift the actual object by lanes */
      region_move_to_lane (
        r, lane_to_move_to);
    }
}

/**
 * Does or undoes the action.
 *
 * @param _do 1 to do, 0 to undo.
 */
static int
do_or_undo_duplicate (
	ArrangerSelectionsAction * self,
  const int                  _do)
{
  int size = 0;
  ArrangerObject ** objs =
    arranger_selections_get_all_objects (
      self->sel, &size);
  ArrangerSelections * sel =
    get_actual_arranger_selections (self);

  long ticks =
    _do ? self->ticks : - self->ticks;
  int delta_tracks =
    _do ? self->delta_tracks : - self->delta_tracks;
  int delta_lanes =
    _do ? self->delta_lanes : - self->delta_lanes;
  int delta_chords =
    _do ? self->delta_chords : - self->delta_chords;
  int delta_pitch =
    _do ? self->delta_pitch : - self->delta_pitch;

  /* clear current selections in the project */
  arranger_selections_clear (sel);

  for (int i = 0; i < size; i++)
    {
      ArrangerObject * obj;
      if (_do)
        {
          /* on first run, we need to first move
           * the original object backwards (the
           * project object too) */
          if (self->first_run)
            {
              obj =
                arranger_object_find (objs[i]);
              g_return_val_if_fail (obj, -1);

              /* ticks */
              if (self->ticks)
                {
                  arranger_object_move (
                    obj, - self->ticks, 0);
                  arranger_object_move (
                    objs[i], - self->ticks, 0);
                }

              /* tracks & lanes */
              if (delta_tracks || delta_lanes)
                {
                  move_obj_by_tracks_and_lanes (
                    obj, - delta_tracks,
                    - delta_lanes);
                  move_obj_by_tracks_and_lanes (
                    objs[i], - delta_tracks,
                    - delta_lanes);
                }

              /* chords */
              /* TODO */

              /* pitch */
              if (delta_pitch)
                {
                  midi_note_shift_pitch (
                    (MidiNote *) obj,
                    - delta_pitch);
                  midi_note_shift_pitch (
                    (MidiNote *) objs[i],
                    - delta_pitch);
                }
            }

          /* clone the clone */
          obj =
            arranger_object_clone (
              objs[i],
              ARRANGER_OBJECT_CLONE_COPY_MAIN);

          /* add to track */
          add_object_to_project (obj);

          /* remember the new name */
          if (!string_is_equal (
                arranger_object_get_name (
                  obj),
                arranger_object_get_name (
                  objs[i]), 0))
            {
              arranger_object_set_name (
                objs[i],
                arranger_object_get_name (obj));
            }
        }
      else
        {
          /* find the actual object */
          obj =
            arranger_object_find (objs[i]);
          g_return_val_if_fail (obj, -1);

          /* remove it */
          remove_object_from_project (obj);
        }

      if (ticks != 0)
        {
          if (_do)
            {
              /* shift it */
              arranger_object_move (
                obj, ticks, F_NO_CACHED);
            }

          /* also shift the copy */
          arranger_object_move (
            objs[i], ticks, F_NO_CACHED);
        }

      if (delta_tracks != 0)
        {
          if (_do)
            {
              move_obj_by_tracks_and_lanes (
                obj, delta_tracks, 0);
            }

          /* FIXME check is it okay to not shift
           * the clone too? */
        }

      if (delta_pitch != 0)
        {
          if (_do)
            {
              MidiNote * mn =
                (MidiNote *) obj;

              /* shift the actual object */
              midi_note_shift_pitch (
                mn, delta_pitch);
            }

          /* also shift the copy so they can
           * match */
          midi_note_shift_pitch (
            (MidiNote *) objs[i],
            delta_pitch);
        }

      if (delta_lanes != 0)
        {
          if (_do)
            {
              move_obj_by_tracks_and_lanes (
                obj, 0, delta_lanes);
            }

          /* FIXME ok to not shift clone too? */
        }

      if (delta_chords != 0)
        {
          /* TODO */
        }

      if (_do)
        {
          /* select it */
          arranger_object_select (
            obj, F_SELECT, F_APPEND);
        }
    }
  free (objs);

  sel =
    get_actual_arranger_selections (self);
  if (_do)
    {
      EVENTS_PUSH (
        ET_ARRANGER_SELECTIONS_CREATED, sel);
    }
  else
    {
      EVENTS_PUSH (
        ET_ARRANGER_SELECTIONS_REMOVED, sel->type);
    }

  self->first_run = 0;

  return 0;
}

/**
 * Does or undoes the action.
 *
 * @param _do 1 to do, 0 to undo.
 * @param create 1 to create, 0 to delete. This just
 *   reverses the actions done by undo/redo.
 */
static int
do_or_undo_create_or_delete (
	ArrangerSelectionsAction * self,
  const int                  _do,
  const int                  create)
{
  int size = 0;
  ArrangerObject ** objs =
    arranger_selections_get_all_objects (
      self->sel, &size);
  ArrangerSelections * sel =
    get_actual_arranger_selections (self);

  if (!self->first_run || !create)
    {
      /* clear current selections in the project */
      arranger_selections_clear (sel);

      for (int i = 0; i < size; i++)
        {

          /* if doing in a create action or undoing
           * in a delete action */
          if ((_do && create) ||
              (!_do && !create))
            {
              /* clone the clone */
              ArrangerObject * obj =
                arranger_object_clone (
                  objs[i],
                  ARRANGER_OBJECT_CLONE_COPY_MAIN);

              /* add it to the project */
              add_object_to_project (obj);

              /* select it */
              arranger_object_select (
                obj, F_SELECT, F_APPEND);

              /* remember new info */
              switch (obj->type)
                {
                case ARRANGER_OBJECT_TYPE_AUTOMATION_POINT:
                  {
                    AutomationPoint * ap =
                      (AutomationPoint *) obj;
                    automation_point_set_region_and_index (
                      (AutomationPoint *) objs[i],
                      ap->region, ap->index);
                  }
                  break;
                case ARRANGER_OBJECT_TYPE_REGION:
                  {
                    /* remember name */
                    Region * r =
                      (Region *) obj;
                    g_free (
                      ((Region *) objs[i])->name);
                    ((Region *) objs[i])->name =
                      g_strdup (r->name);
                  }
                  break;
                default:
                  break;
                }
            }

          /* if undoing */
          else
            {
              /* get the actual object from the
               * project */
              ArrangerObject * obj =
                arranger_object_find (objs[i]);
              g_return_val_if_fail (obj, -1);

              /* remove it */
              remove_object_from_project (obj);
            }
        }
    }
  free (objs);

  if ((_do && create) || (!_do && !create))
    {
      EVENTS_PUSH (
        ET_ARRANGER_SELECTIONS_CREATED, sel);
    }
  else
    {
      EVENTS_PUSH (
        ET_ARRANGER_SELECTIONS_REMOVED, sel->type);
    }

  self->first_run = 0;

  return 0;
}

/**
 * Does or undoes the action.
 *
 * @param _do 1 to do, 0 to undo.
 * @param create 1 to create, 0 to delete. This just
 *   reverses the actions done by undo/redo.
 */
static int
do_or_undo_edit (
	ArrangerSelectionsAction * self,
  const int                  _do)
{
  int size = 0;
  ArrangerObject ** objs_before =
    arranger_selections_get_all_objects (
      self->sel, &size);
  ArrangerObject ** objs_after =
    arranger_selections_get_all_objects (
      self->sel_after, &size);

  ArrangerObject ** src_objs =
    _do ? objs_before : objs_after;
  ArrangerObject ** dest_objs =
    _do ? objs_after : objs_before;

  if (!self->first_run)
    {
      for (int i = 0; i < size; i++)
        {
          /* find the actual object */
          ArrangerObject * obj =
            arranger_object_find (src_objs[i]);
          g_return_val_if_fail (obj, -1);

          /* change the parameter */
          switch (self->edit_type)
            {
            case ARRANGER_SELECTIONS_ACTION_EDIT_NAME:
              {
                switch (obj->type)
                  {
                  case ARRANGER_OBJECT_TYPE_REGION:
                    {
                      Region * r =
                        (Region *) obj;
                      region_set_name (
                        r,
                        ((Region *) dest_objs[i])->
                          name);
                    }
                    break;
                  case ARRANGER_OBJECT_TYPE_MARKER:
                    {
                      Marker * m =
                        (Marker *) obj;
                      marker_set_name (
                        m,
                        ((Marker *) dest_objs[i])->
                          name);
                    }
                    break;
                  default:
                    break;
                  }
              }
              break;
            case ARRANGER_SELECTIONS_ACTION_EDIT_POS:
              obj->pos =
                dest_objs[i]->pos;
              obj->end_pos =
                dest_objs[i]->end_pos;
              obj->clip_start_pos =
                dest_objs[i]->clip_start_pos;
              obj->loop_start_pos =
                dest_objs[i]->loop_start_pos;
              obj->loop_end_pos =
                dest_objs[i]->loop_end_pos;
              break;
            case ARRANGER_SELECTIONS_ACTION_EDIT_PRIMITIVE:
#define SET_PRIMITIVE(cc,member) \
    ((cc *) obj)->member = ((cc *) dest_objs[i])->member

              switch (obj->type)
                {
                case ARRANGER_OBJECT_TYPE_REGION:
                  {
                    SET_PRIMITIVE (Region, muted);
                    SET_PRIMITIVE (Region, color);
                  }
                  break;
                case ARRANGER_OBJECT_TYPE_MIDI_NOTE:
                  {
                    SET_PRIMITIVE (MidiNote, muted);

                    /* set velocity and cache vel */
                    MidiNote * mn =
                      (MidiNote *) obj;
                    MidiNote * dest_mn =
                      (MidiNote *) dest_objs[i];
                    velocity_set_val (
                      mn->vel, dest_mn->vel->vel);
                    mn->vel->cache_vel =
                      dest_mn->vel->vel;
                  }
                  break;
                default:
                  break;
                }
#undef SET_PRIMITIVE
              break;
            case ARRANGER_SELECTIONS_ACTION_EDIT_SCALE:
              {
                ScaleObject * scale =
                  (ScaleObject *) obj;
                ScaleObject * dest_scale =
                  (ScaleObject *) dest_objs[i];

                /* set the new scale */
                MusicalScale * old = scale->scale;
                scale->scale =
                  musical_scale_clone (
                    dest_scale->scale);
                free_later (old, musical_scale_free);
              }
              break;
            default:
              break;
            }
        }
    }
  free (src_objs);
  free (dest_objs);

  ArrangerSelections * sel =
    get_actual_arranger_selections (self);
  EVENTS_PUSH (
    ET_ARRANGER_SELECTIONS_CHANGED, sel);

  self->first_run = 0;

  return 0;
}

/**
 * Does or undoes the action.
 *
 * @param _do 1 to do, 0 to undo.
 */
static int
do_or_undo_split (
	ArrangerSelectionsAction * self,
  const int                  _do)
{
  int size = 0;
  ArrangerObject ** objs =
    arranger_selections_get_all_objects (
      self->sel, &size);

  for (int i = 0; i < size; i++)
    {
      ArrangerObject * obj, * r1, * r2;
      if (_do)
        {
          /* find the actual object */
          obj =
            arranger_object_find (objs[i]);
          g_return_val_if_fail (obj, -1);

          /* split */
          arranger_object_split (
            obj, &self->pos, 0, &self->r1[i],
            &self->r2[i]);

          /* r1 and r2 are now inside the project,
           * clone them to keep copies */
          self->r1[i] =
            arranger_object_clone (
              self->r1[i],
              ARRANGER_OBJECT_CLONE_COPY);
          self->r2[i] =
            arranger_object_clone (
              self->r2[i],
              ARRANGER_OBJECT_CLONE_COPY);
        }
      else
        {
          /* find the actual objects */
          r1 =
            arranger_object_find (self->r1[i]);
          r2 =
            arranger_object_find (self->r2[i]);
          g_return_val_if_fail (r1 && r2, -1);

          /* unsplit */
          arranger_object_unsplit (
            r1, r2, &obj);
          if (obj->type ==
                ARRANGER_OBJECT_TYPE_REGION)
            {
              region_set_name (
                (Region *) obj,
                ((Region *) objs[i])->name);
            }

          /* free the copies created in _do */
          arranger_object_free (
            self->r1[i]);
          arranger_object_free (
            self->r2[i]);
        }
    }
  free (objs);

  ArrangerSelections * sel =
    get_actual_arranger_selections (self);
  EVENTS_PUSH (
    ET_ARRANGER_SELECTIONS_CHANGED, sel);

  self->first_run = 0;

  return 0;
}

/**
 * Does or undoes the action.
 *
 * @param _do 1 to do, 0 to undo.
 */
static int
do_or_undo_resize (
	ArrangerSelectionsAction * self,
  const int                  _do)
{
  int size = 0;
  ArrangerObject ** objs =
    arranger_selections_get_all_objects (
      self->sel, &size);

  long ticks =
    _do ? self->ticks : - self->ticks;

  if (!self->first_run)
    {
      for (int i = 0; i < size; i++)
        {
          /* find the actual object */
          ArrangerObject * obj =
            arranger_object_find (objs[i]);
          g_return_val_if_fail (obj, -1);

          /* resize */
          arranger_object_resize (
            obj,
            self->resize_type ==
              ARRANGER_SELECTIONS_ACTION_RESIZE_L ||
            self->resize_type ==
              ARRANGER_SELECTIONS_ACTION_RESIZE_L_LOOP ?
              1 : 0,
            self->resize_type ==
              ARRANGER_SELECTIONS_ACTION_RESIZE_L_LOOP ||
            self->resize_type ==
              ARRANGER_SELECTIONS_ACTION_RESIZE_R_LOOP ?
              F_LOOP : F_NO_LOOP,
            ticks);

          /* also resize the clone so we can find the
           * actual object next time */
          arranger_object_resize (
            objs[i],
            self->resize_type ==
              ARRANGER_SELECTIONS_ACTION_RESIZE_L ||
            self->resize_type ==
              ARRANGER_SELECTIONS_ACTION_RESIZE_L_LOOP ?
              1 : 0,
            self->resize_type ==
              ARRANGER_SELECTIONS_ACTION_RESIZE_L_LOOP ||
            self->resize_type ==
              ARRANGER_SELECTIONS_ACTION_RESIZE_R_LOOP ?
              F_LOOP : F_NO_LOOP,
            ticks);
        }
    }
  free (objs);

  ArrangerSelections * sel =
    get_actual_arranger_selections (self);
  EVENTS_PUSH (
    ET_ARRANGER_SELECTIONS_CHANGED, sel);

  self->first_run = 0;

  return 0;
}

/**
 * Does or undoes the action.
 *
 * @param _do 1 to do, 0 to undo.
 */
static int
do_or_undo_quantize (
	ArrangerSelectionsAction * self,
  const int                  _do)
{
  int size = 0;
  ArrangerObject ** objs =
    arranger_selections_get_all_objects (
      self->sel, &size);
  ArrangerObject ** quantized_objs =
    arranger_selections_get_all_objects (
      self->quantized_sel, &size);

  for (int i = 0; i < size; i++)
    {
      /* find the actual object */
      ArrangerObject * obj;
      if (_do)
        {
          obj =
            arranger_object_find (objs[i]);
        }
      else
        {
          obj =
            arranger_object_find (quantized_objs[i]);
        }
      g_return_val_if_fail (obj, -1);

      if (_do)
        {
          /* quantize it */
          if (self->opts->adj_start)
            {
              long ticks =
                quantize_options_quantize_position (
                  self->opts, &obj->pos);
              position_add_ticks (
                &obj->end_pos, ticks);
            }
          if (self->opts->adj_end)
            {
              quantize_options_quantize_position (
                self->opts, &obj->end_pos);
            }
          arranger_object_pos_setter (
            obj, &obj->pos);
          arranger_object_end_pos_setter (
            obj, &obj->end_pos);

          /* remember the quantized position so we
           * can find the object when undoing */
          position_set_to_pos (
            &quantized_objs[i]->pos, &obj->pos);
          position_set_to_pos (
            &quantized_objs[i]->end_pos,
            &obj->end_pos);
        }
      else
        {
          /* unquantize it */
          arranger_object_pos_setter (
            obj, &objs[i]->pos);
          arranger_object_end_pos_setter (
            obj, &objs[i]->end_pos);
        }
    }
  free (objs);

  ArrangerSelections * sel =
    get_actual_arranger_selections (self);
  EVENTS_PUSH (
    ET_ARRANGER_SELECTIONS_CHANGED, sel);

  self->first_run = 0;

  return 0;
}

int
arranger_selections_action_do (
  ArrangerSelectionsAction * self)
{
  UndoableAction * ua =
    (UndoableAction *) self;

  switch (ua->type)
    {
    case UA_CREATE_ARRANGER_SELECTIONS:
      return do_or_undo_create_or_delete (self, 1, 1);
      break;
    case UA_DELETE_ARRANGER_SELECTIONS:
      return do_or_undo_create_or_delete (self, 1, 0);
      break;
    case UA_DUPLICATE_ARRANGER_SELECTIONS:
      return do_or_undo_duplicate (self, 1);
      break;
    case UA_MOVE_ARRANGER_SELECTIONS:
      return do_or_undo_move (self, 1);
      break;
    case UA_EDIT_ARRANGER_SELECTIONS:
      return do_or_undo_edit (self, 1);
      break;
    case UA_SPLIT_ARRANGER_SELECTIONS:
      return do_or_undo_split (self, 1);
      break;
    case UA_RESIZE_ARRANGER_SELECTIONS:
      return do_or_undo_resize (self, 1);
      break;
    case UA_QUANTIZE_ARRANGER_SELECTIONS:
      return do_or_undo_quantize (self, 1);
      break;
    default:
      g_return_val_if_reached (-1);
      break;
    }

  g_return_val_if_reached (-1);
}

int
arranger_selections_action_undo (
  ArrangerSelectionsAction * self)
{
  UndoableAction * ua =
    (UndoableAction *) self;

  switch (ua->type)
    {
    case UA_CREATE_ARRANGER_SELECTIONS:
      return do_or_undo_create_or_delete (self, 0, 1);
      break;
    case UA_DELETE_ARRANGER_SELECTIONS:
      return do_or_undo_create_or_delete (self, 0, 0);
      break;
    case UA_DUPLICATE_ARRANGER_SELECTIONS:
      return do_or_undo_duplicate (self, 0);
      break;
    case UA_MOVE_ARRANGER_SELECTIONS:
      return do_or_undo_move (self, 0);
      break;
    case UA_EDIT_ARRANGER_SELECTIONS:
      return do_or_undo_edit (self, 0);
      break;
    case UA_SPLIT_ARRANGER_SELECTIONS:
      return do_or_undo_split (self, 0);
      break;
    case UA_RESIZE_ARRANGER_SELECTIONS:
      return do_or_undo_resize (self, 0);
      break;
    case UA_QUANTIZE_ARRANGER_SELECTIONS:
      return do_or_undo_quantize (self, 0);
      break;
    default:
      g_return_val_if_reached (-1);
      break;
    }

  g_return_val_if_reached (-1);
}

char *
arranger_selections_action_stringize (
	ArrangerSelectionsAction * self)
{
  return g_strdup (_("Edit arranger object(s)"));
}

void
arranger_selections_action_free (
	ArrangerSelectionsAction * self)
{
  arranger_selections_free_full (self->sel);

  free (self);
}
