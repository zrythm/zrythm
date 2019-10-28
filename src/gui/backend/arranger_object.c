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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "audio/audio_region.h"
#include "audio/automation_point.h"
#include "audio/automation_region.h"
#include "audio/chord_region.h"
#include "audio/chord_track.h"
#include "audio/marker_track.h"
#include "audio/midi_region.h"
#include "gui/backend/arranger_object.h"
#include "gui/backend/automation_selections.h"
#include "gui/backend/chord_selections.h"
#include "gui/backend/timeline_selections.h"
#include "gui/widgets/audio_region.h"
#include "gui/widgets/automation_arranger.h"
#include "gui/widgets/automation_curve.h"
#include "gui/widgets/automation_editor_space.h"
#include "gui/widgets/automation_point.h"
#include "gui/widgets/automation_region.h"
#include "gui/widgets/bot_dock_edge.h"
#include "gui/widgets/center_dock.h"
#include "gui/widgets/chord_arranger.h"
#include "gui/widgets/chord_editor_space.h"
#include "gui/widgets/chord_object.h"
#include "gui/widgets/chord_region.h"
#include "gui/widgets/clip_editor.h"
#include "gui/widgets/clip_editor_inner.h"
#include "gui/widgets/marker.h"
#include "gui/widgets/midi_arranger.h"
#include "gui/widgets/midi_editor_space.h"
#include "gui/widgets/midi_modifier_arranger.h"
#include "gui/widgets/midi_note.h"
#include "gui/widgets/midi_region.h"
#include "gui/widgets/region.h"
#include "gui/widgets/scale_object.h"
#include "gui/widgets/timeline_arranger.h"
#include "gui/widgets/timeline_panel.h"
#include "gui/widgets/velocity.h"
#include "project.h"
#include "utils/flags.h"

#define TYPE(x) \
  ARRANGER_OBJECT_TYPE_##x

#define POSITION_TYPE(x) \
  ARRANGER_OBJECT_POSITION_TYPE_##x

#define FOREACH_TYPE(func) \
  func (REGION, Region, region) \
  func (SCALE_OBJECT, ScaleObject, scale_object) \
  func (MARKER, Marker, marker) \
  func (MIDI_NOTE, MidiNote, midi_note) \
  func (VELOCITY, Velocity, velocity) \
  func (CHORD_OBJECT, ChordObject, chord_object) \
  func (AUTOMATION_POINT, AutomationPoint, \
        automation_point)

/**
 * Returns the ArrangerSelections corresponding
 * to the given object type.
 */
ArrangerSelections *
arranger_object_get_selections_for_type (
  ArrangerObjectType type)
{
  switch (type)
    {
    case TYPE (REGION):
    case TYPE (SCALE_OBJECT):
    case TYPE (MARKER):
      return (ArrangerSelections *) TL_SELECTIONS;
    case TYPE (MIDI_NOTE):
    case TYPE (VELOCITY):
      return (ArrangerSelections *) MA_SELECTIONS;
    case TYPE (CHORD_OBJECT):
      return (ArrangerSelections *) CHORD_SELECTIONS;
    case TYPE (AUTOMATION_POINT):
      return
        (ArrangerSelections *) AUTOMATION_SELECTIONS;
    default:
      return NULL;
    }
}

/**
 * Selects the object by adding it to its
 * corresponding selections or making it the
 * only selection.
 *
 * @param select 1 to select, 0 to deselect.
 * @param append 1 to append, 0 to make it the only
 *   selection.
 */
void
arranger_object_select (
  ArrangerObject * self,
  const int        select,
  const int        append)
{
  ArrangerSelections * selections =
    arranger_object_get_selections_for_type (
      self->type);

  if (select)
    {
      if (!append)
        {
          arranger_selections_clear (
            selections);
        }
      arranger_selections_add_object (
        selections, self);
    }
  else
    {
      arranger_selections_remove_object (
        selections, self);
    }
}

/**
 * Returns the widget type for the given
 * ArrangerObjectType.
 */
GType
arranger_object_get_widget_type_for_type (
  ArrangerObjectType type)
{
  switch (type)
    {
    case ARRANGER_OBJECT_TYPE_REGION:
      return REGION_WIDGET_TYPE;
    case ARRANGER_OBJECT_TYPE_MIDI_NOTE:
      return MIDI_NOTE_WIDGET_TYPE;
    case ARRANGER_OBJECT_TYPE_AUTOMATION_POINT:
      return AUTOMATION_POINT_WIDGET_TYPE;
    case ARRANGER_OBJECT_TYPE_AUTOMATION_CURVE:
      return AUTOMATION_CURVE_WIDGET_TYPE;
    case ARRANGER_OBJECT_TYPE_VELOCITY:
      return VELOCITY_WIDGET_TYPE;
    case ARRANGER_OBJECT_TYPE_CHORD_OBJECT:
      return CHORD_OBJECT_WIDGET_TYPE;
    case ARRANGER_OBJECT_TYPE_SCALE_OBJECT:
      return SCALE_OBJECT_WIDGET_TYPE;
    case ARRANGER_OBJECT_TYPE_MARKER:
      return MARKER_WIDGET_TYPE;
    default:
      g_return_val_if_reached (0);
    }
  g_return_val_if_reached (0);
}

/**
 * Returns the number of loops in the ArrangerObject,
 * optionally including incomplete ones.
 */
int
arranger_object_get_num_loops (
  ArrangerObject * self,
  const int        count_incomplete)
{
  int i = 0;
  long loop_size =
    arranger_object_get_loop_length_in_frames (
      self);
  g_return_val_if_fail (loop_size > 0, 0);
  long full_size =
    arranger_object_get_length_in_frames (self);
  long loop_start =
    self->loop_start_pos.frames -
    self->clip_start_pos.frames;
  long curr_ticks = loop_start;

  while (curr_ticks < full_size)
    {
      i++;
      curr_ticks += loop_size;
    }

  if (!count_incomplete)
    i--;

  return i;
}

static void
reset_region_counterparts (
  Region * src,
  Region * dest)
{
  g_return_if_fail (src && dest);
  dest->tmp_lane = src->tmp_lane;
}

static void
reset_midi_note_counterparts (
  MidiNote * src,
  MidiNote * dest)
{
  g_return_if_fail (
    dest && dest->vel && src && src->vel);
  dest->vel->vel = src->vel->vel;
  dest->val = src->val;
}

static void
reset_counterparts (
  ArrangerObject * src,
  ArrangerObject * dest)
{
  g_return_if_fail (src && dest);

  /* reset positions */
  dest->pos = src->pos;
  if (src->has_length)
    {
      dest->end_pos = src->end_pos;
    }
  if (src->can_loop)
    {
      dest->clip_start_pos = src->clip_start_pos;
      dest->loop_start_pos = src->loop_start_pos;
      dest->loop_end_pos = src->loop_end_pos;
    }
  if (src->can_fade)
    {
      dest->fade_in_pos = src->fade_in_pos;
      dest->fade_out_pos = src->fade_out_pos;
    }

  /* reset other members */
  switch (src->type)
    {
    case TYPE (REGION):
      reset_region_counterparts (
        (Region *) src, (Region *) dest);
      break;
    case TYPE (MIDI_NOTE):
      reset_midi_note_counterparts (
        (MidiNote *) src, (MidiNote *) dest);
      break;
    case TYPE (CHORD_OBJECT):
      {
        ChordObject * dest_co =
          (ChordObject *) dest;
        ChordObject * src_co =
          (ChordObject *) src;
        dest_co->index = src_co->index;
      }
      break;
    case TYPE (AUTOMATION_POINT):
      {
        AutomationPoint * dest_ap =
          (AutomationPoint *) dest;
        AutomationPoint * src_ap =
          (AutomationPoint *) src;
        dest_ap->fvalue = src_ap->fvalue;
      }
      break;
    default:
      break;
    }
}

/**
 * Sets the transient's values to the main
 * object's values.
 *
 * @param reset_trans 1 to reset the transient from
 *   main, 0 to reset main from transient.
 */
void
arranger_object_reset_counterparts (
  ArrangerObject * self,
  const int        reset_trans)
{
  ArrangerObject * src =
    reset_trans ?
      arranger_object_get_main (self) :
      arranger_object_get_main_trans (self);
  ArrangerObject * dest =
    reset_trans ?
      arranger_object_get_main_trans (self) :
      arranger_object_get_main (self);

  reset_counterparts (src, dest);

  if (self->can_have_lanes)
    {
      src =
        reset_trans ?
          arranger_object_get_lane (self) :
          arranger_object_get_lane_trans (self);
      dest =
        reset_trans ?
          arranger_object_get_lane_trans (self) :
          arranger_object_get_lane (self);

      reset_counterparts (src, dest);
    }
}

/**
 * Returns if the object is in the selections.
 */
int
arranger_object_is_selected (
  ArrangerObject * self)
{
  ArrangerSelections * selections =
    arranger_object_get_selections_for_type (
      self->type);

  return
    arranger_selections_contains_object (
      selections, self);
}

/**
 * If the object is part of a Region, returns it,
 * otherwise returns NULL.
 */
Region *
arranger_object_get_region (
  ArrangerObject * self)
{
  switch (self->type)
    {
    case ARRANGER_OBJECT_TYPE_AUTOMATION_POINT:
      {
        AutomationPoint * ap =
          (AutomationPoint *) self;
        return ap->region;
      }
      break;
    case ARRANGER_OBJECT_TYPE_AUTOMATION_CURVE:
      {
        AutomationCurve * ac =
          (AutomationCurve *) self;
        return ac->region;
      }
      break;
    case ARRANGER_OBJECT_TYPE_MIDI_NOTE:
      {
        MidiNote * mn =
          (MidiNote *) self;
        return mn->region;
      }
      break;
    case ARRANGER_OBJECT_TYPE_VELOCITY:
      {
        Velocity * vel =
          (Velocity *) self;
        return vel->midi_note->region;
      }
      break;
    case ARRANGER_OBJECT_TYPE_CHORD_OBJECT:
      {
        ChordObject * co =
          (ChordObject *) self;
        return co->region;
      }
      break;
    default:
      break;
    }
  return NULL;
}

/**
 * Gets a pointer to the Position in the
 * ArrangerObject matching the given arguments.
 *
 * @param cache 1 to return the cached Position
 *   instead of the main one.
 */
static Position *
get_position_ptr (
  ArrangerObject *           self,
  ArrangerObjectPositionType pos_type,
  int                        cached)
{
  switch (pos_type)
    {
    case ARRANGER_OBJECT_POSITION_TYPE_START:
      if (cached)
        return &self->cache_pos;
      else
        return &self->pos;
    case ARRANGER_OBJECT_POSITION_TYPE_END:
      if (cached)
        return &self->cache_end_pos;
      else
        return &self->end_pos;
    case ARRANGER_OBJECT_POSITION_TYPE_CLIP_START:
      if (cached)
        return &self->cache_clip_start_pos;
      else
        return &self->clip_start_pos;
    case ARRANGER_OBJECT_POSITION_TYPE_LOOP_START:
      if (cached)
        return &self->cache_loop_start_pos;
      else
        return &self->loop_start_pos;
    case ARRANGER_OBJECT_POSITION_TYPE_LOOP_END:
      if (cached)
        return &self->cache_loop_end_pos;
      else
        return &self->loop_end_pos;
    }
  g_return_val_if_reached (NULL);
}

/**
 * Returns if the given Position is valid.
 *
 * @param pos The position to set to.
 * @param pos_type The type of Position to set in the
 *   ArrangerObject.
 * @param cached Set to 1 to validate based on the
 *   cached positions instead of the main ones.
 * @param validate Validate the Position before
 *   setting it.
 */
int
arranger_object_is_position_valid (
  ArrangerObject *           self,
  const Position *           pos,
  ArrangerObjectPositionType pos_type,
  const int                  cached)
{
  int is_valid = 0;
  switch (pos_type)
    {
    case ARRANGER_OBJECT_POSITION_TYPE_START:
      if (self->has_length)
        {
          Position * end_pos =
            cached ?
              &self->cache_end_pos :
              &self->end_pos;
          is_valid =
            position_is_before (
              pos, end_pos) &&
            position_is_after_or_equal (
              pos, &POSITION_START);
        }
      else if (self->is_pos_global)
        {
          is_valid =
            position_is_after_or_equal (
              pos, &POSITION_START);
        }
      else
        {
          is_valid = 1;
        }
      break;
    case ARRANGER_OBJECT_POSITION_TYPE_LOOP_START:
      {
        Position * loop_end_pos =
          cached ?
            &self->cache_loop_end_pos :
            &self->loop_end_pos;
        is_valid =
          position_is_before (
            pos, loop_end_pos) &&
          position_is_after_or_equal (
            pos, &POSITION_START);
      }
      break;
    case ARRANGER_OBJECT_POSITION_TYPE_LOOP_END:
      {
        /* TODO */
        is_valid = 1;
      }
      break;
    case ARRANGER_OBJECT_POSITION_TYPE_CLIP_START:
      {
        Position * loop_end_pos =
          cached ?
            &self->cache_loop_end_pos :
            &self->loop_end_pos;
        is_valid =
          position_is_before (
            pos, loop_end_pos) &&
          position_is_after_or_equal (
            pos, &POSITION_START);
      }
      break;
    case ARRANGER_OBJECT_POSITION_TYPE_END:
      {
        /* TODO */
        is_valid = 1;
      }
      break;
    default:
      break;
    }

  return is_valid;
}

/**
 * Sets the Position  all of the object's linked
 * objects (see ArrangerObjectInfo)
 *
 * @param pos The position to set to.
 * @param pos_type The type of Position to set in the
 *   ArrangerObject.
 * @param cached Set to 1 to set the cached positions
 *   instead of the main ones.
 * @param validate Validate the Position before
 *   setting it.
 * @param update_flag Flag to indicate which
 *   counterparts to move.
 */
void
arranger_object_set_position (
  ArrangerObject *           self,
  const Position *           pos,
  ArrangerObjectPositionType pos_type,
  const int                  cached,
  const int                  validate,
  ArrangerObjectUpdateFlag   update_flag)
{
  /* return if validate is on and position is
   * invalid */
  if (validate &&
      !arranger_object_is_position_valid (
        self, pos, pos_type, cached))
    return;

  switch (update_flag)
  {
    case AO_UPDATE_THIS:
      position_set_to_pos (
        get_position_ptr (
          self, pos_type, cached),
        pos);
      break;
    case AO_UPDATE_TRANS:
      position_set_to_pos (
        get_position_ptr (
          arranger_object_get_main_trans (self),
          pos_type, cached),
        pos);
      break;
    case AO_UPDATE_NON_TRANS:
      position_set_to_pos (
        get_position_ptr (
          arranger_object_get_main (self),
          pos_type, cached),
        pos);
      break;
    case AO_UPDATE_ALL:
      position_set_to_pos (
        get_position_ptr (
          arranger_object_get_main_trans (self),
          pos_type, cached),
        pos);
      position_set_to_pos (
        get_position_ptr (
          arranger_object_get_main (self),
          pos_type, cached),
        pos);
      break;
    }

  if (self->can_have_lanes)
    {
      switch (update_flag)
      {
        case AO_UPDATE_THIS:
          position_set_to_pos (
            get_position_ptr (
              self, pos_type, cached),
            pos);
          break;
        case AO_UPDATE_TRANS:
          position_set_to_pos (
            get_position_ptr (
              arranger_object_get_lane_trans (self),
              pos_type, cached),
            pos);
          break;
        case AO_UPDATE_NON_TRANS:
          position_set_to_pos (
            get_position_ptr (
              arranger_object_get_lane (self),
              pos_type, cached),
            pos);
          break;
        case AO_UPDATE_ALL:
          position_set_to_pos (
            get_position_ptr (
              arranger_object_get_lane_trans (self),
              pos_type, cached),
            pos);
          position_set_to_pos (
            get_position_ptr (
              arranger_object_get_lane (self),
              pos_type, cached),
            pos);
          break;
        }
    }
}

/**
 * Moves the object by the given amount of
 * ticks.
 *
 * @param use_cached_pos Add the ticks to the cached
 *   Position instead of its current Position.
 * @param update_flag Flag to indicate which
 *   counterparts to move.
 */
void
arranger_object_move (
  ArrangerObject *         self,
  const long               ticks,
  const int                use_cached_pos,
  ArrangerObjectUpdateFlag update_flag)
{
  if (arranger_object_type_has_length (self->type))
    {
      /* start pos */
      Position tmp;
      if (use_cached_pos)
        position_set_to_pos (
          &tmp, &self->cache_pos);
      else
        position_set_to_pos (
          &tmp, &self->pos);
      position_add_ticks (
        &tmp, ticks);
      arranger_object_set_position (
        self, &tmp,
        ARRANGER_OBJECT_POSITION_TYPE_START,
        F_NO_CACHED, F_NO_VALIDATE, update_flag);

      /* end pos */
      if (use_cached_pos)
        position_set_to_pos (
          &tmp, &self->cache_end_pos);
      else
        position_set_to_pos (
          &tmp, &self->end_pos);
      position_add_ticks (
        &tmp, ticks);
      arranger_object_set_position (
        self, &tmp,
        ARRANGER_OBJECT_POSITION_TYPE_END,
        F_NO_CACHED, F_NO_VALIDATE, update_flag);
    }
  else
    {
      Position tmp;
      if (use_cached_pos)
        position_set_to_pos (
          &tmp, &self->cache_pos);
      else
        position_set_to_pos (
          &tmp, &self->pos);
      position_add_ticks (
        &tmp, ticks);
      arranger_object_set_position (
        self, &tmp,
        ARRANGER_OBJECT_POSITION_TYPE_START,
        F_NO_CACHED, F_NO_VALIDATE, update_flag);
    }
}

/**
 * Getter.
 */
void
arranger_object_get_pos (
  const ArrangerObject * self,
  Position *             pos)
{
  position_set_to_pos (
    pos, &self->pos);
}

/**
 * Getter.
 */
void
arranger_object_get_end_pos (
  const ArrangerObject * self,
  Position *             pos)

{
  position_set_to_pos (
    pos, &self->end_pos);
}

/**
 * Getter.
 */
void
arranger_object_get_clip_start_pos (
  const ArrangerObject * self,
  Position *             pos)

{
  position_set_to_pos (
    pos, &self->clip_start_pos);
}

/**
 * Getter.
 */
void
arranger_object_get_loop_start_pos (
  const ArrangerObject * self,
  Position *             pos)

{
  position_set_to_pos (
    pos, &self->loop_start_pos);
}

/**
 * Getter.
 */
void
arranger_object_get_loop_end_pos (
  const ArrangerObject * self,
  Position *             pos)

{
  position_set_to_pos (
    pos, &self->loop_end_pos);
}

/**
 * Sets the given object as the main object and
 * clones it into its other counterparts.
 *
 * The type must be set before calling this function.
 */
void
arranger_object_set_as_main (
  ArrangerObject *   self)
{
  g_return_if_fail (self->type > TYPE (NONE));
  ArrangerObject * main_trans =
    arranger_object_clone (
      self, ARRANGER_OBJECT_CLONE_COPY);
  ArrangerObject * lane =
    arranger_object_clone (
      self, ARRANGER_OBJECT_CLONE_COPY);
  ArrangerObject * lane_trans =
    arranger_object_clone (
      self, ARRANGER_OBJECT_CLONE_COPY);
  arranger_object_info_init_main (
    self, main_trans, lane, lane_trans);
}

static void
init_loaded_midi_note (
  MidiNote * self)
{
  midi_note_set_region (self, self->region);
  self->vel->midi_note = self;
  arranger_object_init_loaded (
    (ArrangerObject *) self->vel);
}

static void
init_loaded_region (
  Region * self)
{
  self->linked_region =
    region_find_by_name (
      self->linked_region_name);

  int i;
  switch (self->type)
    {
    case REGION_TYPE_AUDIO:
      {
      }
      break;
    case REGION_TYPE_MIDI:
      {
        MidiNote * mn;
        for (i = 0; i < self->num_midi_notes; i++)
          {
            mn = self->midi_notes[i];
            mn->region = self;
            arranger_object_init_loaded (
              (ArrangerObject *) mn);
          }
        self->midi_notes_size =
          (size_t) self->num_midi_notes;
      }
      break;
    case REGION_TYPE_CHORD:
      {
        ChordObject * chord;
        for (i = 0; i < self->num_chord_objects;
             i++)
          {
            chord = self->chord_objects[i];
            chord->region = self;
            arranger_object_init_loaded (
              (ArrangerObject *) chord);
          }
        self->chord_objects_size =
          (size_t) self->num_chord_objects;
        }
      break;
    case REGION_TYPE_AUTOMATION:
      {
        AutomationPoint * ap;
        AutomationCurve * ac;
        for (i = 0; i < self->num_aps; i++)
          {
            ap = self->aps[i];
            ap->region = self;
            arranger_object_init_loaded (
              (ArrangerObject *) ap);
          }
        for (i = 0; i < self->num_acs; i++)
          {
            ac = self->acs[i];
            ac->region = self;
            arranger_object_init_loaded (
              (ArrangerObject *) ac);
          }
        self->aps_size =
          (size_t) self->num_aps;
      }
      break;
    }

  if (region_type_has_lane (self->type))
    {
      region_set_lane (self, self->lane);
    }
  else if (self->type == REGION_TYPE_AUTOMATION)
    {
      region_set_automation_track (
        self, self->at);
    }
}

/**
 * Initializes the object after loading a Project.
 */
void
arranger_object_init_loaded (
  ArrangerObject * self)
{
  g_return_if_fail (self->type > TYPE (NONE));

  arranger_object_set_as_main (self);

  /* init positions */
  self->cache_pos = self->pos;
  self->cache_end_pos = self->end_pos;

  switch (self->type)
    {
    case TYPE (REGION):
      init_loaded_region (
        (Region *) self);
      break;
    case TYPE (MIDI_NOTE):
      init_loaded_midi_note (
        (MidiNote *) self);
      break;
    case TYPE (AUTOMATION_POINT):
      {
        AutomationPoint * ap =
          (AutomationPoint *) self;
        automation_point_set_region_and_index (
          ap, ap->region, ap->index);
      }
      break;
    case TYPE (VELOCITY):
      {
        Velocity * vel =
          (Velocity *) self;
        velocity_set_midi_note (
          vel, vel->midi_note);
      }
      break;
    default:
      break;
    }
}

/**
 * Returns the length of the ArrangerObject (if
 * it has length) in ticks.
 *
 * (End Position - start Position).
 */
long
arranger_object_get_length_in_ticks (
  ArrangerObject * self)
{
  g_return_val_if_fail (self->has_length, 0);
  return
    self->end_pos.total_ticks -
    self->pos.total_ticks;
}

/**
 * Returns the length of the ArrangerObject (if
 * it has length) in frames.
 *
 * (End Position - start Position).
 */
long
arranger_object_get_length_in_frames (
  ArrangerObject * self)
{
  g_return_val_if_fail (self->has_length, 0);
  return
    self->end_pos.frames -
    self->pos.frames;
}

/**
 * Returns the length of the loop in ticks.
 */
long
arranger_object_get_loop_length_in_ticks (
  ArrangerObject * self)
{
  g_return_val_if_fail (self->has_length, 0);
  return
    self->loop_end_pos.total_ticks -
    self->loop_start_pos.total_ticks;
}

/**
 * Returns the length of the loop in ticks.
 */
long
arranger_object_get_loop_length_in_frames (
  ArrangerObject * self)
{
  g_return_val_if_fail (self->has_length, 0);
  return
    self->loop_end_pos.frames -
    self->loop_start_pos.frames;
}

/**
 * Updates the frames of each position in each child
 * recursively.
 */
void
arranger_object_update_frames (
  ArrangerObject * self)
{
  position_update_frames (
    &self->pos);
  if (self->has_length)
    {
      position_update_frames (
        &self->end_pos);
    }
  if (self->can_loop)
    {
      position_update_frames (
        &self->clip_start_pos);
      position_update_frames (
        &self->loop_start_pos);
      position_update_frames (
        &self->loop_end_pos);
    }
  if (self->can_fade)
    {
      position_update_frames (
        &self->fade_in_pos);
      position_update_frames (
        &self->fade_out_pos);
    }

  int i;
  Region * r;
  switch (self->type)
    {
    case TYPE (REGION):
      r = (Region *) self;
      for (i = 0; i < r->num_midi_notes; i++)
        {
          arranger_object_update_frames (
            (ArrangerObject *) r->midi_notes[i]);
        }
      for (i = 0; i < r->num_unended_notes; i++)
        {
          arranger_object_update_frames (
            (ArrangerObject *) r->unended_notes[i]);
        }

      for (i = 0; i < r->num_aps; i++)
        {
          arranger_object_update_frames (
            (ArrangerObject *) r->aps[i]);
        }

      for (i = 0; i < r->num_acs; i++)
        {
          arranger_object_update_frames (
            (ArrangerObject *) r->acs[i]);
        }

      for (i = 0; i < r->num_chord_objects; i++)
        {
          arranger_object_update_frames (
            (ArrangerObject *) r->chord_objects[i]);
        }
      break;
    default:
      break;
    }
}

static GtkWidget *
create_widget (
  ArrangerObject * self)
{
  Region * r;
  switch (self->type)
    {
    case TYPE (REGION):
      r = (Region *) self;
      switch (r->type)
        {
        case REGION_TYPE_MIDI:
          return
            (GtkWidget *)
            midi_region_widget_new (r);
        case REGION_TYPE_AUDIO:
          return
            (GtkWidget *)
            audio_region_widget_new (r);
        case REGION_TYPE_AUTOMATION:
          return
            (GtkWidget *)
            automation_region_widget_new (r);
        case REGION_TYPE_CHORD:
          return
            (GtkWidget *)
            chord_region_widget_new (r);
        }
      break;
    case TYPE (MIDI_NOTE):
      return
        (GtkWidget *)
        midi_note_widget_new ((MidiNote *) self);
    case TYPE (AUTOMATION_POINT):
      return
        (GtkWidget *)
        automation_point_widget_new (
          (AutomationPoint *) self);
    case TYPE (MARKER):
      return
        (GtkWidget *)
        marker_widget_new ((Marker *) self);
    case TYPE (CHORD_OBJECT):
      return
        (GtkWidget *)
        chord_object_widget_new (
          (ChordObject *) self);
    case TYPE (SCALE_OBJECT):
      return
        (GtkWidget *)
        scale_object_widget_new (
          (ScaleObject *) self);
    default:
      g_return_val_if_reached (NULL);
      break;
    }
  g_return_val_if_reached (NULL);
}

/**
 * Generates a widget for each of the object's
 * counterparts.
 */
void
arranger_object_gen_widget (
  ArrangerObject * self)
{
  ArrangerObject * tmp;
  for (int i = AOI_COUNTERPART_MAIN;
       i <= AOI_COUNTERPART_MAIN_TRANSIENT; i++)
    {
      if (i == AOI_COUNTERPART_MAIN)
        tmp = arranger_object_get_main (self);
      else if (i == AOI_COUNTERPART_MAIN_TRANSIENT)
        tmp = arranger_object_get_main_trans (self);

      g_return_if_fail (tmp);
      tmp->widget = create_widget (tmp);
    }
  if (self->can_have_lanes)
    {
      for (int i = AOI_COUNTERPART_LANE;
           i <= AOI_COUNTERPART_LANE_TRANSIENT; i++)
        {
          if (i == AOI_COUNTERPART_LANE)
            tmp = arranger_object_get_lane (self);
          else if (i == AOI_COUNTERPART_LANE_TRANSIENT)
            tmp =
              arranger_object_get_lane_trans (self);

          g_return_if_fail (tmp);
          tmp->widget = create_widget (tmp);
        }
    }
}

/**
 * Frees each object stored in info.
 */
void
arranger_object_free_all (
  ArrangerObject * self)
{
  arranger_object_free (self->info.main);
  arranger_object_free (self->info.main_trans);
  arranger_object_free (self->info.lane);
  arranger_object_free (self->info.lane_trans);
}

static void
free_region (
  Region * self)
{
  if (self->name)
    g_free (self->name);

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

static void
free_midi_note (
  MidiNote * self)
{
  arranger_object_free_all (
    (ArrangerObject *) self->vel);

  if (self->region_name)
    g_free (self->region_name);

  free (self);
}

/**
 * Frees only this object.
 */
void
arranger_object_free (
  ArrangerObject * self)
{
  if (GTK_IS_WIDGET (self->widget))
    {
      g_object_unref (self->widget);
    }

  switch (self->type)
    {
    case TYPE (REGION):
      free_region ((Region *) self);
      return;
    case TYPE (MIDI_NOTE):
      free_midi_note ((MidiNote *) self);
      return;
    case TYPE (MARKER):
      {
        Marker * marker = (Marker *) self;
        g_free (marker->name);
        free (marker);
      }
      return;
    case TYPE (CHORD_OBJECT):
      {
        ChordObject * co = (ChordObject *) self;
        if (co->region_name)
          g_free (co->region_name);
        free (co);
      }
      return;
    case TYPE (SCALE_OBJECT):
      {
        ScaleObject * scale = (ScaleObject *) self;
        musical_scale_free (scale->scale);
        free (scale);
      }
      return;
    case TYPE (AUTOMATION_POINT):
      {
        AutomationPoint * ap =
          (AutomationPoint *) self;
        if (ap->region_name)
          g_free (ap->region_name);
        free (ap);
      }
      return;
    case TYPE (AUTOMATION_CURVE):
      {
        AutomationCurve * ac =
          (AutomationCurve *) self;
        free (ac);
      }
      return;
    case TYPE (VELOCITY):
      {
        Velocity * vel =
          (Velocity *) self;
        free (vel);
      }
      return;
    default:
      g_return_if_reached ();
    }
  g_return_if_reached ();
}

/**
 * Returns the visible counterpart (ie, the
 * transient or the non transient) of the object.
 *
 * Used for example when moving a Region to
 * allocate the MidiNote's based on the transient
 * Region's position instead of the main Region.
 *
 * Only checks the main/main-trans.
 */
ArrangerObject *
arranger_object_get_visible_counterpart (
  ArrangerObject * self)
{
  self = arranger_object_get_main (self);
  if (!arranger_object_should_be_visible (self))
    self = arranger_object_get_main_trans (self);
  return self;
}

static void
add_ticks_to_region_children (
  Region *   self,
  const long ticks)
{
  switch (self->type)
    {
    case REGION_TYPE_MIDI:
      for (int i = 0; i < self->num_midi_notes; i++)
        {
          arranger_object_move (
            (ArrangerObject *) self->midi_notes[i],
            ticks, F_NO_CACHED, AO_UPDATE_ALL);
        }
      break;
    case REGION_TYPE_AUDIO:
      break;
    case REGION_TYPE_AUTOMATION:
      break;
    case REGION_TYPE_CHORD:
      break;
    }
}

/**
 * Adds the given ticks to each included object.
 */
void
arranger_object_add_ticks_to_children (
  ArrangerObject * self,
  const long       ticks)
{
  if (self->type == TYPE (REGION))
    {
      add_ticks_to_region_children (
        (Region *) self, ticks);
    }
}

/**
 * Resizes the object on the left side or right
 * side by given amount of ticks, for objects that
 * do not have loops (currently none? keep it as
 * reference).
 *
 * @param left 1 to resize left side, 0 to resize
 *   right side.
 * @param ticks Number of ticks to resize.
 * @param loop Whether this is a loop-resize (1) or
 *   a normal resize (0). Only used if the object
 *   can have loops.
 * @param update_flag ArrangerObjectUpdateFlag.
 */
void
arranger_object_resize (
  ArrangerObject * self,
  const int        left,
  const int        loop,
  const long       ticks,
  ArrangerObjectUpdateFlag update_flag)
{
  Position tmp;
  if (left)
    {
      tmp = self->pos;
      position_add_ticks (
        &tmp, ticks);
      arranger_object_set_position (
        self, &tmp,
        ARRANGER_OBJECT_POSITION_TYPE_START,
        F_NO_CACHED, F_NO_VALIDATE, update_flag);

      if (self->can_loop)
        {
          tmp = self->loop_end_pos;
          position_add_ticks (
            &tmp, - ticks);
          arranger_object_set_position (
            self, &tmp,
            ARRANGER_OBJECT_POSITION_TYPE_LOOP_END,
            F_NO_CACHED, F_NO_VALIDATE, update_flag);
        }

      /* move containing items */
      arranger_object_add_ticks_to_children (
        arranger_object_get_main (self),
        - ticks);
      if (loop && self->can_loop)
        {
          tmp = self->loop_start_pos;
          position_add_ticks (
            &tmp, - ticks);
          arranger_object_set_position (
            self, &tmp,
            ARRANGER_OBJECT_POSITION_TYPE_LOOP_START,
            F_NO_CACHED, F_NO_VALIDATE, update_flag);
        }
    }
  else
    {
      tmp = self->end_pos;
      position_add_ticks (
        &tmp, ticks);
      arranger_object_set_position (
        self, &tmp,
        ARRANGER_OBJECT_POSITION_TYPE_END,
        F_NO_CACHED, F_NO_VALIDATE, update_flag);

      if (!loop && self->can_loop)
        {
          tmp = self->loop_end_pos;
          position_add_ticks (
            &tmp, ticks);
          arranger_object_set_position (
            self, &tmp,
            ARRANGER_OBJECT_POSITION_TYPE_LOOP_END,
            F_NO_CACHED, F_NO_VALIDATE, update_flag);
        }
    }
}

static void
post_deserialize_children (
  ArrangerObject * self)
{
  if (self->type == TYPE (REGION))
    {
      Region * r = (Region *) self;
      MidiNote * mn;
      for (int i = 0; i < r->num_midi_notes; i++)
        {
          mn = r->midi_notes[i];
          mn->region = r;
          arranger_object_post_deserialize (
            (ArrangerObject *) mn);
        }
    }
}

void
arranger_object_post_deserialize (
  ArrangerObject * self)
{
  g_return_if_fail (self);
  self->info.main = self;

  post_deserialize_children (self);
}

/**
 * The setter is for use in e.g. the digital meters
 * whereas the set_pos func is used during
 * arranger actions.
 */
void
arranger_object_pos_setter (
  ArrangerObject * self,
  const Position * pos)
{
  arranger_object_set_position (
    self, pos,
    ARRANGER_OBJECT_POSITION_TYPE_START,
    F_NO_CACHED, F_VALIDATE, AO_UPDATE_ALL);
}

/**
 * The setter is for use in e.g. the digital meters
 * whereas the set_pos func is used during
 * arranger actions.
 */
void
arranger_object_end_pos_setter (
  ArrangerObject * self,
  const Position * pos)
{
  arranger_object_set_position (
    self, pos,
    ARRANGER_OBJECT_POSITION_TYPE_END,
    F_NO_CACHED, F_VALIDATE, AO_UPDATE_ALL);
}

/**
 * The setter is for use in e.g. the digital meters
 * whereas the set_pos func is used during
 * arranger actions.
 */
void
arranger_object_clip_start_pos_setter (
  ArrangerObject * self,
  const Position * pos)
{
  arranger_object_set_position (
    self, pos,
    ARRANGER_OBJECT_POSITION_TYPE_CLIP_START,
    F_NO_CACHED, F_VALIDATE, AO_UPDATE_ALL);
}

/**
 * The setter is for use in e.g. the digital meters
 * whereas the set_pos func is used during
 * arranger actions.
 */
void
arranger_object_loop_start_pos_setter (
  ArrangerObject * self,
  const Position * pos)
{
  arranger_object_set_position (
    self, pos,
    ARRANGER_OBJECT_POSITION_TYPE_LOOP_START,
    F_NO_CACHED, F_VALIDATE, AO_UPDATE_ALL);
}

/**
 * The setter is for use in e.g. the digital meters
 * whereas the set_pos func is used during
 * arranger actions.
 */
void
arranger_object_loop_end_pos_setter (
  ArrangerObject * self,
  const Position * pos)
{
  arranger_object_set_position (
    self, pos,
    ARRANGER_OBJECT_POSITION_TYPE_LOOP_END,
    F_NO_CACHED, F_VALIDATE, AO_UPDATE_ALL);
}

/**
 * Validates the given Position.
 *
 * @return 1 if valid, 0 otherwise.
 */
int
arranger_object_validate_pos (
  ArrangerObject *           self,
  const Position *           pos,
  ArrangerObjectPositionType type)
{
  switch (self->type)
    {
    case TYPE (REGION):
      switch (type)
        {
        case POSITION_TYPE (START):
          return
            position_is_before (
              pos, &self->end_pos) &&
            position_is_after_or_equal (
              pos, &POSITION_START);
          break;
        default:
          break;
        }
      break;
    default:
      break;
    }

  return 1;
}

/**
 * Returns whether the object is transient or not.
 *
 * Transient objects are objects that are used
 * during moving operations.
 */
int
arranger_object_is_transient (
  ArrangerObject * self)
{
  return (
    self->info.counterpart ==
      AOI_COUNTERPART_MAIN_TRANSIENT ||
    self->info.counterpart ==
      AOI_COUNTERPART_LANE_TRANSIENT);
}

/**
 * Returns whether the object is a lane object or not
 * (only applies to TimelineArrangerWidget objects.
 */
int
arranger_object_is_lane (
  ArrangerObject * self)
{
  return (
    self->info.counterpart ==
      AOI_COUNTERPART_LANE ||
    self->info.counterpart ==
      AOI_COUNTERPART_LANE_TRANSIENT);
}

/**
 * Returns whether the object is a main object or not
 * (only applies to TimelineArrangerWidget objects.
 */
int
arranger_object_is_main (
  ArrangerObject * self)
{
  return (
    self->info.counterpart ==
      AOI_COUNTERPART_MAIN ||
    self->info.counterpart ==
      AOI_COUNTERPART_MAIN_TRANSIENT);
}

/**
 * Returns the Track this ArrangerObject is in.
 */
Track *
arranger_object_get_track (
  ArrangerObject * self)
{
  g_return_val_if_fail (self, NULL);

  Track * track = NULL;

  switch (self->type)
    {
    case TYPE (REGION):
      {
        Region * r = (Region *) self;
        track =
          TRACKLIST->tracks[r->track_pos];
      }
      break;
    case TYPE (CHORD_OBJECT):
      {
        ChordObject * chord = (ChordObject *) self;
        Region * r = chord->region;
        g_return_val_if_fail (r, NULL);
        track =
          TRACKLIST->tracks[r->track_pos];
      }
      break;
    case TYPE (SCALE_OBJECT):
      {
        ScaleObject * scale = (ScaleObject *) self;
        track =
          TRACKLIST->tracks[scale->track_pos];
      }
      break;
    case TYPE (MARKER):
      {
        Marker * marker = (Marker *) self;
        track =
          TRACKLIST->tracks[marker->track_pos];
      }
      break;
    case TYPE (AUTOMATION_POINT):
      {
        AutomationPoint * ap =
          (AutomationPoint *) self;
        g_return_val_if_fail (
          ap->region && ap->region->at &&
          ap->region->at->track, NULL);
        track = ap->region->at->track;
      }
      break;
    case TYPE (MIDI_NOTE):
      {
        MidiNote * mn = (MidiNote *) self;
        g_return_val_if_fail (
          mn->region, NULL);
        track =
          TRACKLIST->tracks[mn->region->track_pos];
      }
      break;
    case TYPE (VELOCITY):
      {
        Velocity * vel = (Velocity *) self;
        g_return_val_if_fail (
          vel->midi_note && vel->midi_note->region,
          NULL);
        track =
          TRACKLIST->tracks[
            vel->midi_note->region->track_pos];
      }
      break;
    default:
      g_warn_if_reached ();
      break;
    }

  g_return_val_if_fail (track, NULL);

  return track;
}

/**
 * Gets the arranger for this arranger object.
 */
ArrangerWidget *
arranger_object_get_arranger (
  ArrangerObject * self)
{
  Track * track =
    arranger_object_get_track (self);

  ArrangerWidget * arranger = NULL;
  switch (self->type)
    {
    case TYPE (REGION):
      {
        if (track->pinned)
          {
            arranger =
              (ArrangerWidget *) (MW_PINNED_TIMELINE);
          }
        else
          {
            arranger =
              (ArrangerWidget *) (MW_TIMELINE);
          }
      }
      break;
    case TYPE (CHORD_OBJECT):
      arranger =
        (ArrangerWidget *) (MW_CHORD_ARRANGER);
      break;
    case TYPE (SCALE_OBJECT):
      {
        if (track->pinned)
          {
            arranger =
              (ArrangerWidget *) (MW_PINNED_TIMELINE);
          }
        else
          {
            arranger =
              (ArrangerWidget *) (MW_TIMELINE);
          }
      }
      break;
    case TYPE (MARKER):
      {
        if (track->pinned)
          {
            arranger =
              (ArrangerWidget *) (MW_PINNED_TIMELINE);
          }
        else
          {
            arranger =
              (ArrangerWidget *) (MW_TIMELINE);
          }
      }
      break;
    case TYPE (AUTOMATION_POINT):
      arranger =
        (ArrangerWidget *) (MW_AUTOMATION_ARRANGER);
      break;
    case TYPE (MIDI_NOTE):
      arranger =
        (ArrangerWidget *) (MW_MIDI_ARRANGER);
      break;
    case TYPE (VELOCITY):
      arranger =
        (ArrangerWidget *) (MW_MIDI_MODIFIER_ARRANGER);
      break;
    default:
      g_return_val_if_reached (NULL);
    }

  g_warn_if_fail (arranger);
  return arranger;
}

/**
 * Returns if the object represented by the
 * ArrrangerObjectInfo should be visible.
 *
 * This is counterpart-specific, ie. it checks
 * if the transient should be visible or lane
 * counterpart should be visible, etc., based on
 * what is given.
 */
int
arranger_object_should_be_visible (
  ArrangerObject * self)
{
  ArrangerWidget * arranger =
    arranger_object_get_arranger (self);

  int trans_visible = 0;
  int non_trans_visible = 0;
  int lane_visible;

  /* check trans/non-trans visiblity */
  if (ARRANGER_WIDGET_GET_ACTION (
        arranger, MOVING) ||
      ARRANGER_WIDGET_GET_ACTION (
        arranger, CREATING_MOVING))
    {
      trans_visible = 0;
      non_trans_visible = 1;
    }
  else if (
    ARRANGER_WIDGET_GET_ACTION (
      arranger, MOVING_COPY) ||
    ARRANGER_WIDGET_GET_ACTION (
      arranger, MOVING_LINK))
    {
      trans_visible = 1;
      non_trans_visible = 1;
    }
  else
    {
      trans_visible = 0;
      non_trans_visible = 1;
    }

  /* check visibility at all */
  if (self->type == TYPE (AUTOMATION_POINT))
    {
      ArrangerObject * ap =
        arranger_object_get_object (self);
      if (!(arranger_object_get_track (ap)->
              bot_paned_visible))
        {
          non_trans_visible = 0;
          trans_visible = 0;
        }
    }

  /* check lane visibility */
  if (self->type == TYPE (REGION))
    {
      lane_visible =
        arranger_object_get_track (self)->
          lanes_visible;
    }
  else
    {
      lane_visible = 0;
    }


  /* return visibility */
  switch (self->info.counterpart)
    {
    case AOI_COUNTERPART_MAIN:
      return non_trans_visible;
    case AOI_COUNTERPART_MAIN_TRANSIENT:
      return trans_visible;
    case AOI_COUNTERPART_LANE:
      return lane_visible && non_trans_visible;
    case AOI_COUNTERPART_LANE_TRANSIENT:
      return lane_visible && trans_visible;
    }

  g_return_val_if_reached (-1);
}

/**
 * Gets the object the ArrangerObjectInfo
 * represents.
 */
ArrangerObject *
arranger_object_get_object (
  ArrangerObject * self)
{
  switch (self->info.counterpart)
    {
    case AOI_COUNTERPART_MAIN:
      return self->info.main;
    case AOI_COUNTERPART_MAIN_TRANSIENT:
      return self->info.main_trans;
    case AOI_COUNTERPART_LANE:
      return self->info.lane;
    case AOI_COUNTERPART_LANE_TRANSIENT:
      return self->info.lane_trans;
    }

  g_return_val_if_reached (NULL);
}

/**
 * Gets the given counterpart.
 */
ArrangerObject *
arranger_object_get_counterpart (
  ArrangerObject *              self,
  ArrangerObjectInfoCounterpart counterpart)
{
  switch (counterpart)
    {
    case AOI_COUNTERPART_MAIN:
      return self->info.main;
    case AOI_COUNTERPART_MAIN_TRANSIENT:
      return self->info.main_trans;
    case AOI_COUNTERPART_LANE:
      return self->info.lane;
    case AOI_COUNTERPART_LANE_TRANSIENT:
      return self->info.lane_trans;
    }
  g_return_val_if_reached (NULL);
}

static void
set_widget_selection_state_flags (
  ArrangerObject * self)
{
  if (arranger_object_is_selected (self))
    {
      gtk_widget_set_state_flags (
        GTK_WIDGET (self->widget),
        GTK_STATE_FLAG_SELECTED, 0);
    }
  else
    {
      gtk_widget_unset_state_flags (
        GTK_WIDGET (self->widget),
        GTK_STATE_FLAG_SELECTED);
    }
  arranger_object_widget_force_redraw (
    Z_ARRANGER_OBJECT_WIDGET (self->widget));
}

static void
set_counterpart_widget_visible (
  ArrangerObject *              self,
  ArrangerObjectInfoCounterpart counterpart)
{
  g_return_if_fail (
    self->can_have_lanes ||
    (counterpart != AOI_COUNTERPART_LANE &&
     counterpart != AOI_COUNTERPART_LANE_TRANSIENT));

  self =
    arranger_object_get_counterpart (
      self, counterpart);
  if (!self->widget)
    arranger_object_gen_widget (self);
  gtk_widget_set_visible (
    GTK_WIDGET (self->widget),
    arranger_object_should_be_visible (
      self));
  set_widget_selection_state_flags (self);
}

/**
 * Sets the widget visibility and selection state
 * to this counterpart
 * only, or to all counterparts if all is 1.
 */
void
arranger_object_set_widget_visibility_and_state (
  ArrangerObject * self,
  int              all)
{
  if (all)
    {
      set_counterpart_widget_visible (
        self, AOI_COUNTERPART_MAIN);
      set_counterpart_widget_visible (
        self, AOI_COUNTERPART_MAIN_TRANSIENT);
      if (self->can_have_lanes)
        {
          set_counterpart_widget_visible (
            self, AOI_COUNTERPART_LANE);
          set_counterpart_widget_visible (
            self, AOI_COUNTERPART_LANE_TRANSIENT);
        }
    }
  else
    {
      set_counterpart_widget_visible (
        self, self->info.counterpart);
    }
}

static ArrangerObject *
find_region (
  Region * self)
{
  return
    (ArrangerObject *)
    region_find_by_name (self->name);
}

static ArrangerObject *
find_chord_object (
  ChordObject * clone)
{
  /* get actual region - clone's region might be
   * an unused clone */
  Region *r =
    (Region *)
    arranger_object_find (
      (ArrangerObject *) clone->region);

  ChordObject * chord;
  for (int i = 0; i < r->num_chord_objects; i++)
    {
      chord = r->chord_objects[i];
      if (chord_object_is_equal (
            chord, clone))
        return (ArrangerObject *) chord;
    }
  return NULL;
}

static ArrangerObject *
find_scale_object (
  ScaleObject * clone)
{
  for (int i = 0;
       i < P_CHORD_TRACK->num_scales; i++)
    {
      if (scale_object_is_equal (
            P_CHORD_TRACK->scales[i],
            clone))
        {
          return
            (ArrangerObject *)
            P_CHORD_TRACK->scales[i];
        }
    }
  return NULL;
}

static ArrangerObject *
find_marker (
  Marker * clone)
{
  for (int i = 0;
       i < P_MARKER_TRACK->num_markers; i++)
    {
      if (marker_is_equal (
            P_MARKER_TRACK->markers[i],
            clone))
        return
          (ArrangerObject *)
          P_MARKER_TRACK->markers[i];
    }
  return NULL;
}

static ArrangerObject *
find_automation_point (
  AutomationPoint * src)
{
  g_warn_if_fail (src->region);

  /* the src region might be an unused clone, find
   * the actual region. */
  Region * region =
    (Region *)
    arranger_object_find (
      (ArrangerObject *) src->region);

  int i;
  AutomationPoint * ap;
  for (i = 0; i < region->num_aps; i++)
    {
      ap = region->aps[i];
      if (automation_point_is_equal (src, ap))
        return (ArrangerObject *) ap;
    }

  return NULL;
}

static ArrangerObject *
find_midi_note (
  MidiNote * src)
{
  Region * r =
    region_find_by_name (src->region_name);
  g_return_val_if_fail (r, NULL);

  for (int i = 0; i < r->num_midi_notes; i++)
    {
      if (midi_note_is_equal (
            r->midi_notes[i], src))
        return (ArrangerObject *) r->midi_notes[i];
    }
  return NULL;
}

/**
 * Returns the ArrangerObject matching the
 * given one.
 *
 * This should be called when we have a copy or a
 * clone, to get the actual region in the project.
 */
ArrangerObject *
arranger_object_find (
  ArrangerObject * self)
{
  switch (self->type)
    {
    case TYPE (REGION):
      return
        find_region ((Region *) self);
    case TYPE (CHORD_OBJECT):
      return
        find_chord_object ((ChordObject *) self);
    case TYPE (SCALE_OBJECT):
      return
        find_scale_object ((ScaleObject *) self);
    case TYPE (MARKER):
      return
        find_marker ((Marker *) self);
    case TYPE (AUTOMATION_POINT):
      return
        find_automation_point (
          (AutomationPoint *) self);
    case TYPE (MIDI_NOTE):
      return
        find_midi_note ((MidiNote *) self);
    case TYPE (VELOCITY):
      {
        Velocity * clone =
          (Velocity *) self;
        MidiNote * mn =
          (MidiNote *)
          find_midi_note (clone->midi_note);
        g_return_val_if_fail (mn && mn->vel, NULL);
        return (ArrangerObject *) mn->vel;
      }
    default:
      g_return_val_if_reached (NULL);
    }
  g_return_val_if_reached (NULL);
}

static ArrangerObject *
clone_region (
  Region *                region,
  ArrangerObjectCloneFlag flag)
{
  int is_main = 0, i, j;
  if (flag == ARRANGER_OBJECT_CLONE_COPY_MAIN)
    is_main = 1;

  ArrangerObject * r_obj =
    (ArrangerObject *) region;
  Region * new_region = NULL;
  switch (region->type)
    {
    case REGION_TYPE_MIDI:
      {
        MidiRegion * mr =
          midi_region_new (
            &r_obj->pos,
            &r_obj->end_pos,
            is_main);
        MidiRegion * mr_orig = region;
        if (flag == ARRANGER_OBJECT_CLONE_COPY ||
            flag == ARRANGER_OBJECT_CLONE_COPY_MAIN)
          {
            for (i = 0;
                 i < mr_orig->num_midi_notes; i++)
              {
                MidiNote * mn =
                  (MidiNote *)
                  arranger_object_clone (
                    (ArrangerObject *)
                    mr_orig->midi_notes[i],
                    ARRANGER_OBJECT_CLONE_COPY_MAIN);

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
            region->pool_id, NULL, NULL, -1,
            0, &r_obj->pos, is_main);

        new_region = ar;
        new_region->pool_id = region->pool_id;
      }
    break;
    case REGION_TYPE_AUTOMATION:
      {
        Region * ar  =
          automation_region_new (
            &r_obj->pos,
            &r_obj->end_pos,
            is_main);
        Region * ar_orig = region;

        AutomationPoint * src_ap, * dest_ap;
        AutomationCurve * src_ac, * dest_ac;

        /* add automation points */
        for (j = 0; j < ar_orig->num_aps; j++)
          {
            src_ap = ar_orig->aps[j];
            ArrangerObject * src_ap_obj =
              (ArrangerObject *) src_ap;
            dest_ap =
              automation_point_new_float (
                src_ap->fvalue,
                &src_ap_obj->pos, F_MAIN);
            automation_region_add_ap (
              ar, dest_ap, 0);
          }

        /* add automation curves */
        for (j = 0; j < ar_orig->num_acs; j++)
          {
            src_ac = ar_orig->acs[j];
            ArrangerObject * src_ac_obj =
              (ArrangerObject *) src_ac;
            dest_ac =
              automation_curve_new (
                ar_orig->at->automatable->type,
                &src_ac_obj->pos, 1);
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
            &r_obj->pos,
            &r_obj->end_pos,
            is_main);
        Region * cr_orig = region;
        if (flag == ARRANGER_OBJECT_CLONE_COPY ||
            flag == ARRANGER_OBJECT_CLONE_COPY_MAIN)
          {
            ChordObject * co;
            for (i = 0;
                 i < cr_orig->num_chord_objects;
                 i++)
              {
                co =
                  (ChordObject *)
                  arranger_object_clone (
                    (ArrangerObject *)
                    cr_orig->chord_objects[i],
                    ARRANGER_OBJECT_CLONE_COPY_MAIN);

                chord_region_add_chord_object (
                  cr, co);
              }
          }

        new_region = cr;
      }
      break;
    }

  g_return_val_if_fail (new_region, NULL);

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

  return (ArrangerObject *) new_region;
}

static ArrangerObject *
clone_midi_note (
  MidiNote *              src,
  ArrangerObjectCloneFlag flag)
{
  int is_main =
    flag == ARRANGER_OBJECT_CLONE_COPY_MAIN;

  ArrangerObject * src_obj =
    (ArrangerObject *) src;
  MidiNote * mn =
    midi_note_new (
      src->region, &src_obj->pos,
      &src_obj->end_pos,
      src->val, src->vel->vel, is_main);

  return (ArrangerObject *) mn;
}

static ArrangerObject *
clone_chord_object (
  ChordObject *           src,
  ArrangerObjectCloneFlag flag)
{
  int is_main = 0;
  if (flag == ARRANGER_OBJECT_CLONE_COPY_MAIN)
    is_main = 1;

  ChordObject * chord =
    chord_object_new (
      src->region, src->index, is_main);

  return (ArrangerObject *) chord;
}

static ArrangerObject *
clone_scale_object (
  ScaleObject *           src,
  ArrangerObjectCloneFlag flag)
{
  int is_main = 0;
  if (flag == ARRANGER_OBJECT_CLONE_COPY_MAIN)
    is_main = 1;

  MusicalScale * musical_scale =
    musical_scale_clone (src->scale);
  ScaleObject * scale =
    scale_object_new (musical_scale, is_main);

  return (ArrangerObject *) scale;
}

static ArrangerObject *
clone_marker (
  Marker *                src,
  ArrangerObjectCloneFlag flag)
{
  int is_main = 0;
  if (flag == ARRANGER_OBJECT_CLONE_COPY_MAIN)
    is_main = 1;

  Marker * marker =
    marker_new (src->name, is_main);

  return (ArrangerObject *) marker;
}

static ArrangerObject *
clone_automation_point (
  AutomationPoint *       src,
  ArrangerObjectCloneFlag flag)
{
  int is_main = 0;
  if (flag == ARRANGER_OBJECT_CLONE_COPY_MAIN)
    is_main = 1;

  ArrangerObject * src_obj =
    (ArrangerObject *) src;
  AutomationPoint * ap =
    automation_point_new_float (
      src->fvalue, &src_obj->pos, is_main);

  return (ArrangerObject *) ap;
}

/**
 * Clone the ArrangerObject.
 *
 * Creates a new region and either links to the
 * original or copies every field.
 */
ArrangerObject *
arranger_object_clone (
  ArrangerObject *        self,
  ArrangerObjectCloneFlag flag)
{
  ArrangerObject * new_obj = NULL;
  switch (self->type)
    {
    case TYPE (REGION):
      new_obj =
        clone_region ((Region *) self, flag);
      break;
    case TYPE (MIDI_NOTE):
      new_obj =
        clone_midi_note ((MidiNote *) self, flag);
      break;
    case TYPE (CHORD_OBJECT):
      new_obj =
        clone_chord_object (
          (ChordObject *) self, flag);
      break;
    case TYPE (SCALE_OBJECT):
      new_obj =
        clone_scale_object (
          (ScaleObject *) self, flag);
      break;
    case TYPE (AUTOMATION_POINT):
      new_obj =
        clone_automation_point (
          (AutomationPoint *) self, flag);
      break;
    case TYPE (MARKER):
      new_obj =
        clone_marker (
          (Marker *) self, flag);
      break;
    case TYPE (VELOCITY):
      {
        Velocity * src = (Velocity *) self;
        int is_main =
          flag == ARRANGER_OBJECT_CLONE_COPY_MAIN;
        new_obj =
          (ArrangerObject *)
          velocity_new (
            src->midi_note, src->vel, is_main);
      }
      break;
    default:
      g_return_val_if_reached (NULL);
    }

  /* set positions */
  new_obj->pos = self->pos;
  new_obj->cache_pos = self->pos;
  if (self->has_length)
    {
      new_obj->end_pos = self->end_pos;
      new_obj->cache_end_pos = self->end_pos;
    }
  if (self->can_loop)
    {
      new_obj->clip_start_pos = self->clip_start_pos;
      new_obj->cache_clip_start_pos =
        self->clip_start_pos;
      new_obj->loop_start_pos = self->loop_start_pos;
      new_obj->cache_loop_start_pos =
        self->cache_loop_start_pos;
      new_obj->loop_end_pos = self->loop_end_pos;
      new_obj->cache_loop_end_pos =
        self->cache_loop_end_pos;
    }
  if (self->can_fade)
    {
      new_obj->fade_in_pos = self->fade_in_pos;
      new_obj->cache_fade_in_pos =
        self->cache_fade_in_pos;
      new_obj->fade_out_pos = self->fade_out_pos;
      new_obj->cache_fade_out_pos =
        self->cache_fade_out_pos;
    }

  return new_obj;
}

/**
 * Splits the given object at the given Position,
 * deletes the original object and adds 2 new
 * objects in the same parent (Track or
 * AutomationTrack or Region).
 *
 * The given object must be the main object, as this
 * will create 2 new main objects.
 *
 * @param region The ArrangerObject to split. This
 *   ArrangerObject will be deleted.
 * @param pos The Position to split at.
 * @param pos_is_local If the position is local (1)
 *   or global (0).
 * @param r1 Address to hold the pointer to the
 *   newly created ArrangerObject 1.
 * @param r2 Address to hold the pointer to the
 *   newly created ArrangerObject 2.
 */
void
arranger_object_split (
  ArrangerObject *  self,
  const Position *  pos,
  const int         pos_is_local,
  ArrangerObject ** r1,
  ArrangerObject ** r2)
{
  /* create the new objects */
  *r1 =
    arranger_object_clone (
      self, ARRANGER_OBJECT_CLONE_COPY_MAIN);
  *r2 =
    arranger_object_clone (
      self, ARRANGER_OBJECT_CLONE_COPY_MAIN);

  /* get global/local positions (the local pos
   * is after traversing the loops) */
  Position globalp, localp;
  if (pos_is_local)
    {
      position_set_to_pos (&globalp, pos);
      position_add_ticks (
        &globalp, self->pos.total_ticks);
      position_set_to_pos (&localp, pos);
    }
  else
    {
      position_set_to_pos (&globalp, pos);
      if (self->type == ARRANGER_OBJECT_TYPE_REGION)
        {
          long localp_frames =
            region_timeline_frames_to_local (
              (Region *) self, globalp.frames, 1);
          position_from_frames (
            &localp, localp_frames);
        }
      else
        {
          position_set_to_pos (&localp, &globalp);
        }
    }

  /* for first region just set the end pos */
  arranger_object_end_pos_setter (
    *r1, &globalp);

  /* for the second set the clip start and start
   * pos. */
  arranger_object_clip_start_pos_setter (
    *r2, &localp);
  arranger_object_pos_setter (
    *r2, &globalp);

  /* add them to the parent */
  switch (self->type)
    {
    case ARRANGER_OBJECT_TYPE_REGION:
      {
        Track * track =
          arranger_object_get_track (self);
        Region * src_region =
          (Region *) self;
        Region * region1 =
          (Region *) *r1;
        Region * region2 =
          (Region *) *r2;
        track_add_region (
          track, region1, src_region->at,
          src_region->lane_pos,
          F_GEN_NAME, F_PUBLISH_EVENTS);
        track_add_region (
          track, region2, src_region->at,
          src_region->lane_pos,
          F_GEN_NAME, F_PUBLISH_EVENTS);
      }
      break;
    case ARRANGER_OBJECT_TYPE_MIDI_NOTE:
      {
        MidiNote * src_midi_note =
          (MidiNote *) self;
        Region * parent_region =
          src_midi_note->region;
        midi_region_add_midi_note (
          parent_region, (MidiNote *) *r1);
        midi_region_add_midi_note (
          parent_region, (MidiNote *) *r2);
      }
      break;
    default:
      break;
    }

  /* generate widgets so update visibility in the
   * arranger can work */
  arranger_object_gen_widget (*r1);
  arranger_object_gen_widget (*r2);

  /* select them */
  ArrangerSelections * sel =
    arranger_object_get_selections_for_type (
      self->type);
  arranger_selections_remove_object (
    sel, self);
  arranger_selections_add_object (
    sel, *r1);
  arranger_selections_add_object (
    sel, *r2);

  /* change to r1 if the original region was the
   * clip editor region */
  if (CLIP_EDITOR->region == (Region *) self)
    {
      clip_editor_set_region (
        CLIP_EDITOR, (Region *) *r1);
    }

  /* remove and free the original object */
  switch (self->type)
    {
    case ARRANGER_OBJECT_TYPE_REGION:
      {
        track_remove_region (
          arranger_object_get_track (self),
          (Region *) self, F_PUBLISH_EVENTS,
          F_FREE);
      }
      break;
    case ARRANGER_OBJECT_TYPE_MIDI_NOTE:
      {
        Region * parent_region =
          ((MidiNote *) self)->region;
        midi_region_remove_midi_note (
          parent_region, (MidiNote *) self,
          F_FREE, F_PUBLISH_EVENTS);
      }
      break;
    default:
      g_warn_if_reached ();
      break;
    }

  EVENTS_PUSH (ET_ARRANGER_OBJECT_CREATED, *r1);
  EVENTS_PUSH (ET_ARRANGER_OBJECT_CREATED, *r2);
}

/**
 * Undoes what arranger_object_split() did.
 */
void
arranger_object_unsplit (
  ArrangerObject *         r1,
  ArrangerObject *         r2,
  ArrangerObject **        obj)
{
  /* create the new object */
  *obj =
    arranger_object_clone (
      r1, ARRANGER_OBJECT_CLONE_COPY_MAIN);

  /* set the end pos to the end pos of r2 */
  arranger_object_end_pos_setter (
    *obj, &r2->end_pos);

  /* add it to the parent */
  switch (r1->type)
    {
    case ARRANGER_OBJECT_TYPE_REGION:
      {
        track_add_region (
          arranger_object_get_track (r1),
          (Region *) *obj, ((Region *) r1)->at,
          ((Region *) r1)->lane_pos,
          F_GEN_NAME, F_PUBLISH_EVENTS);
      }
      break;
    case ARRANGER_OBJECT_TYPE_MIDI_NOTE:
      {
        Region * parent_region =
          ((MidiNote *) r1)->region;
        midi_region_add_midi_note (
          parent_region, (MidiNote *) *obj);
      }
      break;
    default:
      break;
    }

  /* generate widgets so update visibility in the
   * arranger can work */
  arranger_object_gen_widget (*obj);

  /* select it */
  ArrangerSelections * sel =
    arranger_object_get_selections_for_type (
      (*obj)->type);
  arranger_selections_remove_object (
    sel, r1);
  arranger_selections_remove_object (
    sel, r2);
  arranger_selections_add_object (
    sel, *obj);

  /* change to r1 if the original region was the
   * clip editor region */
  if (CLIP_EDITOR->region == (Region *) r1)
    {
      clip_editor_set_region (
        CLIP_EDITOR, (Region *) *obj);
    }

  /* remove and free the original regions */
  switch (r1->type)
    {
    case ARRANGER_OBJECT_TYPE_REGION:
      {
        track_remove_region (
          arranger_object_get_track (r1),
          (Region *) r1, F_PUBLISH_EVENTS, F_FREE);
        track_remove_region (
          arranger_object_get_track (r2),
          (Region *) r2, F_PUBLISH_EVENTS, F_FREE);
      }
      break;
    case ARRANGER_OBJECT_TYPE_MIDI_NOTE:
      {
        midi_region_remove_midi_note (
          ((MidiNote *) r1)->region,
          (MidiNote *) r1, F_PUBLISH_EVENTS, F_FREE);
        midi_region_remove_midi_note (
          ((MidiNote *) r2)->region,
          (MidiNote *) r2, F_PUBLISH_EVENTS, F_FREE);
      }
      break;
    default:
      break;
    }

  EVENTS_PUSH (ET_ARRANGER_OBJECT_CREATED, *obj);
}

/**
 * Sets the name of the object, if the object can
 * have a name.
 */
void
arranger_object_set_name (
  ArrangerObject *         self,
  const char *             name,
  ArrangerObjectUpdateFlag flag)
{
  switch (self->type)
    {
    case ARRANGER_OBJECT_TYPE_MARKER:
      {
        arranger_object_set_string (
          Marker, self, name, name, flag);
      }
      break;
    case ARRANGER_OBJECT_TYPE_REGION:
      {
        arranger_object_set_string (
          Region, self, name, name, flag);
      }
      break;
    default:
      break;
    }
  EVENTS_PUSH (ET_ARRANGER_OBJECT_CHANGED, self);
}

/**
 * Returns the widget for the object.
 */
ArrangerObjectWidget *
arranger_object_get_widget (
  ArrangerObject * self)
{
  return Z_ARRANGER_OBJECT_WIDGET (self->widget);
}
