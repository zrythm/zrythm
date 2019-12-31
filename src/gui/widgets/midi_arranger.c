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

#include "actions/arranger_selections.h"
#include "audio/channel.h"
#include "audio/instrument_track.h"
#include "audio/midi_region.h"
#include "audio/mixer.h"
#include "audio/track.h"
#include "audio/tracklist.h"
#include "audio/transport.h"
#include "audio/velocity.h"
#include "gui/widgets/arranger.h"
#include "gui/widgets/bot_dock_edge.h"
#include "gui/widgets/center_dock.h"
#include "gui/widgets/clip_editor.h"
#include "gui/widgets/clip_editor_inner.h"
#include "gui/widgets/color_area.h"
#include "gui/widgets/editor_ruler.h"
#include "gui/widgets/inspector.h"
#include "gui/widgets/main_window.h"
#include "gui/widgets/midi_arranger_bg.h"
#include "gui/widgets/midi_arranger.h"
#include "gui/widgets/midi_editor_space.h"
#include "gui/widgets/midi_modifier_arranger.h"
#include "gui/widgets/midi_note.h"
#include "gui/widgets/piano_roll_keys.h"
#include "gui/widgets/region.h"
#include "gui/widgets/ruler.h"
#include "gui/widgets/timeline_bg.h"
#include "gui/widgets/timeline_ruler.h"
#include "gui/widgets/track.h"
#include "gui/widgets/tracklist.h"
#include "project.h"
#include "utils/arrays.h"
#include "utils/gtk.h"
#include "utils/flags.h"
#include "utils/ui.h"
#include "utils/resources.h"
#include "settings/settings.h"
#include "zrythm.h"

#include <gtk/gtk.h>

#if 0
int
midi_arranger_widget_get_note_at_y (double y)
{
  double adj_y = y - 1;
  double adj_px_per_key =
    MW_PIANO_ROLL_KEYS->px_per_key + 1;
  if (PIANO_ROLL->drum_mode)
    {
      return
        PIANO_ROLL->drum_descriptors[
          (int)(adj_y / adj_px_per_key)].value;
    }
  else
    return
      PIANO_ROLL->piano_descriptors[
        (int)(adj_y / adj_px_per_key)].value;
}
#endif

/**
 * Called on drag begin in parent when background is double
 * clicked (i.e., a note is created).
 * @param pos The absolute position in the piano
 *   roll.
 */
void
midi_arranger_widget_create_note (
  ArrangerWidget * self,
  Position * pos,
  int                  note,
  ZRegion * region)
{
  ArrangerObject * region_obj =
    (ArrangerObject *) region;

  /* get local pos */
  Position local_pos;
  position_from_ticks (
    &local_pos,
    pos->total_ticks -
    region_obj->pos.total_ticks);

  /* set action */
  if (PIANO_ROLL->drum_mode)
    self->action =
      UI_OVERLAY_ACTION_MOVING;
  else
    self->action =
      UI_OVERLAY_ACTION_CREATING_RESIZING_R;

  /* create midi note */
  MidiNote * midi_note =
    midi_note_new (
      region, &local_pos, &local_pos,
      (midi_byte_t) note,
      VELOCITY_DEFAULT, 1);
  ArrangerObject * midi_note_obj =
    (ArrangerObject *) midi_note;

  /* add it to region */
  midi_region_add_midi_note (
    region, midi_note);

  /*arranger_object_gen_widget (midi_note_obj);*/

  /* set visibility */
  /*arranger_object_set_widget_visibility_and_state (*/
    /*midi_note_obj, 1);*/

  /* set end pos */
  Position tmp;
  position_set_min_size (
    &midi_note_obj->pos,
    &tmp, self->snap_grid);
  arranger_object_set_position (
    midi_note_obj, &tmp,
    ARRANGER_OBJECT_POSITION_TYPE_END,
    F_NO_CACHED, F_NO_VALIDATE);
  arranger_object_set_position (
    midi_note_obj, &tmp,
    ARRANGER_OBJECT_POSITION_TYPE_END,
    F_CACHED, F_NO_VALIDATE);

  /* set as start object */
  self->start_object = midi_note_obj;

  EVENTS_PUSH (
    ET_ARRANGER_OBJECT_CREATED, midi_note);

  /* select it */
  arranger_object_select (
    midi_note_obj, F_SELECT, F_NO_APPEND);
}

/**
 * Called when in selection mode.
 *
 * Called by arranger widget during drag_update to find and
 * select the midi notes enclosed in the selection area.
 *
 * @param[in] delete If this is a select-delete
 *   operation
 */
/*void*/
/*midi_arranger_widget_select (*/
  /*ArrangerWidget * self,*/
  /*double               offset_x,*/
  /*double               offset_y,*/
  /*int                  delete)*/
/*{*/
  /*ARRANGER_WIDGET_GET_PRIVATE (self);*/

  /*if (!delete)*/
    /*[> deselect all <]*/
    /*arranger_widget_select_all (*/
      /*Z_ARRANGER_WIDGET (self), 0);*/

  /*[> find enclosed midi notes <]*/
  /*GtkWidget *    midi_note_widgets[800];*/
  /*int            num_midi_note_widgets = 0;*/
  /*arranger_widget_get_hit_widgets_in_range (*/
    /*Z_ARRANGER_WIDGET (self),*/
    /*MIDI_NOTE_WIDGET_TYPE,*/
    /*self->start_x,*/
    /*self->start_y,*/
    /*offset_x,*/
    /*offset_y,*/
    /*midi_note_widgets,*/
    /*&num_midi_note_widgets);*/

  /*ArrangerObjectWidget * midi_note_widget;*/
  /*ArrangerObject * midi_note_obj;*/
  /*if (delete)*/
    /*{*/
      /*[> delete the enclosed midi notes <]*/
      /*for (int i = 0; i < num_midi_note_widgets; i++)*/
        /*{*/
          /*midi_note_widget =*/
            /*Z_ARRANGER_OBJECT_WIDGET (*/
              /*midi_note_widgets[i]);*/
          /*ARRANGER_OBJECT_WIDGET_GET_PRIVATE (*/
            /*midi_note_widget);*/
          /*midi_note_obj =*/
            /*arranger_object_get_main (*/
              /*ao_prv->arranger_object);*/

          /*MidiNote * mn =*/
            /*(MidiNote *) midi_note_obj;*/

          /*midi_region_remove_midi_note (*/
            /*mn->region, mn,*/
            /*F_NO_FREE, F_PUBLISH_EVENTS);*/
        /*}*/
    /*}*/
  /*else*/
    /*{*/
      /*[> select the enclosed midi_notes <]*/
      /*for (int i = 0; i < num_midi_note_widgets; i++)*/
        /*{*/
          /*midi_note_widget =*/
            /*Z_ARRANGER_OBJECT_WIDGET (*/
              /*midi_note_widgets[i]);*/
          /*ARRANGER_OBJECT_WIDGET_GET_PRIVATE (*/
            /*midi_note_widget);*/
          /*midi_note_obj =*/
            /*arranger_object_get_main (*/
              /*ao_prv->arranger_object);*/
          /*arranger_object_select (*/
            /*midi_note_obj, F_SELECT, F_APPEND);*/
        /*}*/
    /*}*/
/*}*/

/**
 * Called during drag_update in the parent when
 * resizing the selection. It sets the start
 * Position of the selected MidiNote's.
 *
 * @param pos Absolute position in the arrranger.
 * @parram dry_run Don't resize notes; just check
 *   if the resize is allowed (check if invalid
 *   resizes will happen)
 *
 * @return 0 if the operation was successful,
 *   nonzero otherwise.
 */
int
midi_arranger_widget_snap_midi_notes_l (
  ArrangerWidget *self,
  Position *          pos,
  int                 dry_run)
{
  ArrangerObject * r_obj =
    (ArrangerObject *) CLIP_EDITOR->region;

  /* get delta with first clicked note's start
   * pos */
  long delta;
  delta =
    pos->total_ticks -
    (self->start_object->
      cache_pos.total_ticks +
    r_obj->pos.total_ticks);

  Position new_start_pos, new_global_start_pos;
  MidiNote * midi_note;
  ArrangerObject * mn_obj;
  for (int i = 0;
       i < MA_SELECTIONS->num_midi_notes;
       i++)
    {
      midi_note = MA_SELECTIONS->midi_notes[i];
      mn_obj =
        (ArrangerObject *) midi_note;

      /* calculate new start pos by adding
       * delta to the cached start pos */
      position_set_to_pos (
        &new_start_pos,
        &mn_obj->cache_pos);
      position_add_ticks (
        &new_start_pos, delta);

      /* get the global star pos first to
       * snap it */
      position_set_to_pos (
        &new_global_start_pos,
        &new_start_pos);
      position_add_ticks (
        &new_global_start_pos,
        r_obj->pos.total_ticks);

      /* snap the global pos */
      if (SNAP_GRID_ANY_SNAP (
            self->snap_grid) &&
          !self->shift_held)
        position_snap (
          NULL, &new_global_start_pos,
          NULL, CLIP_EDITOR->region,
          self->snap_grid);

      /* convert it back to a local pos */
      position_set_to_pos (
        &new_start_pos,
        &new_global_start_pos);
      position_add_ticks (
        &new_start_pos,
        - r_obj->pos.total_ticks);

      if (position_is_before (
            &new_global_start_pos,
            &POSITION_START) ||
          position_is_after_or_equal (
            &new_start_pos,
            &mn_obj->end_pos))
        return -1;
      else if (!dry_run)
        {
          arranger_object_pos_setter (
            mn_obj, &new_start_pos);
        }
    }

  EVENTS_PUSH (
    ET_ARRANGER_SELECTIONS_CHANGED,
    MA_SELECTIONS);

  return 0;
}

/**
 * Sets the currently hovered note and queues a
 * redraw if it changed.
 *
 * @param pitch The note pitch, or -1 for no note.
 */
void
midi_arranger_widget_set_hovered_note (
  ArrangerWidget * self,
  int              pitch)
{
  if (self->hovered_note != pitch)
    {
      GdkRectangle rect;
      arranger_widget_get_visible_rect (self, &rect);
      double adj_px_per_key =
        MW_PIANO_ROLL_KEYS->px_per_key + 1.0;
      if (self->hovered_note != -1)
        {
          /* redraw the previous note area to
           * unhover it */
          rect.y =
            (int)
            (adj_px_per_key *
               (127.0 - (double) self->hovered_note) -
             1.0);
          rect.height = (int) adj_px_per_key;
          arranger_widget_redraw_rectangle (
            self, &rect);
        }
      self->hovered_note = pitch;

      if (pitch != -1)
        {
          /* redraw newly hovered note area */
          rect.y =
            (int)
            (adj_px_per_key *
               (127.0 - (double) pitch) - 1);
          rect.height = (int) adj_px_per_key;
          arranger_widget_redraw_rectangle (
            self, &rect);
        }
    }
}

/**
 * Called during drag_update in parent when
 * resizing the selection. It sets the end
 * Position of the selected MIDI notes.
 *
 * @param pos Absolute position in the arrranger.
 * @parram dry_run Don't resize notes; just check
 *   if the resize is allowed (check if invalid
 *   resizes will happen)
 *
 * @return 0 if the operation was successful,
 *   nonzero otherwise.
 */
int
midi_arranger_widget_snap_midi_notes_r (
  ArrangerWidget *self,
  Position *          pos,
  int                 dry_run)
{
  ArrangerObject * r_obj =
    (ArrangerObject *) CLIP_EDITOR->region;

  /* get delta with first clicked notes's end
   * pos */
  long delta =
    pos->total_ticks -
    (self->start_object->
      cache_end_pos.total_ticks +
    r_obj->pos.total_ticks);

  MidiNote * midi_note;
  Position new_end_pos, new_global_end_pos;
  for (int i = 0;
       i < MA_SELECTIONS->num_midi_notes;
       i++)
    {
      midi_note =
        MA_SELECTIONS->midi_notes[i];
      ArrangerObject * mn_obj =
        (ArrangerObject *) midi_note;

      Track * track =
        arranger_object_get_track (mn_obj);

      /* get new end pos by adding delta
       * to the cached end pos */
      position_set_to_pos (
        &new_end_pos,
        &mn_obj->cache_end_pos);
      position_add_ticks (
        &new_end_pos, delta);

      /* get the global end pos first to snap it */
      position_set_to_pos (
        &new_global_end_pos,
        &new_end_pos);
      position_add_ticks (
        &new_global_end_pos,
        r_obj->pos.total_ticks);

      /* snap the global pos */
      if (SNAP_GRID_ANY_SNAP (
            self->snap_grid) &&
          !self->shift_held)
        position_snap (
          NULL, &new_global_end_pos, track,
          NULL, self->snap_grid);

      /* convert it back to a local pos */
      position_set_to_pos (
        &new_end_pos,
        &new_global_end_pos);
      position_add_ticks (
        &new_end_pos,
        - r_obj->pos.total_ticks);

      if (position_is_before_or_equal (
            &new_end_pos,
            &mn_obj->pos))
        return -1;
      else if (!dry_run)
        {
          arranger_object_end_pos_setter (
            mn_obj, &new_end_pos);
        }
    }

  EVENTS_PUSH (
    ET_ARRANGER_SELECTIONS_CHANGED,
    MA_SELECTIONS);

  return 0;
}

/**
 * Called on move items_y setup.
 *
 * calculates the max possible y movement
 */
int
midi_arranger_calc_deltamax_for_note_movement (
  int y_delta)
{
  MidiNote * midi_note;
  for (int i = 0;
       i < MA_SELECTIONS->num_midi_notes;
       i++)
    {
      midi_note =
        MA_SELECTIONS->midi_notes[i];
      /*g_message ("midi note val %d, y delta %d",*/
                 /*midi_note->val, y_delta);*/
      if (midi_note->val + y_delta < 0)
        {
          y_delta = 0;
        }
      else if (midi_note->val + y_delta >= 127)
        {
          y_delta = 127 - midi_note->val;
        }
    }
  /*g_message ("y delta %d", y_delta);*/
  return y_delta;
  /*return y_delta < 0 ? -1 : 1;*/
}

/**
 * Listen to the currently selected notes.
 *
 * This function either turns on the notes if they
 * are not playing, changes the notes if the pitch
 * changed, or otherwise does nothing.
 *
 * @param listen Turn notes on if 1, or turn them
 *   off if 0.
 */
void
midi_arranger_listen_notes (
  ArrangerWidget * self,
  int              listen)
{
  if (!g_settings_get_int (
         S_UI, "listen-notes"))
    return;

  ArrangerSelections * sel =
    arranger_widget_get_selections (self);
  MidiArrangerSelections * mas =
    (MidiArrangerSelections *) sel;
  for (int i = 0; i < mas->num_midi_notes; i++)
    {
      MidiNote * mn = mas->midi_notes[i];
      midi_note_listen (mn, listen);
    }
}

/**
 * Called on drag end.
 *
 * Sets default cursors back and sets the start midi note
 * to NULL if necessary.
 */
/*void*/
/*midi_arranger_widget_on_drag_end (*/
  /*ArrangerWidget * self)*/
/*{*/
  /*ARRANGER_WIDGET_GET_PRIVATE (self);*/

  /*MidiNote * midi_note;*/
  /*for (int i = 0;*/
       /*i < MA_SELECTIONS->num_midi_notes;*/
       /*i++)*/
    /*{*/
      /*midi_note =*/
        /*MA_SELECTIONS->midi_notes[i];*/
      /*ArrangerObject * mn_obj =*/
        /*(ArrangerObject *) midi_note;*/

      /*if (Z_IS_ARRANGER_OBJECT_WIDGET (*/
            /*mn_obj->widget))*/
        /*{*/
          /*arranger_object_widget_update_tooltip (*/
            /*Z_ARRANGER_OBJECT_WIDGET (*/
              /*mn_obj->widget), 0);*/
        /*}*/

      /*EVENTS_PUSH (*/
        /*ET_ARRANGER_OBJECT_CREATED, midi_note);*/
    /*}*/

  /*switch (self->action)*/
    /*{*/
    /*case UI_OVERLAY_ACTION_RESIZING_L:*/
    /*{*/
      /*ArrangerObject * trans_note =*/
        /*arranger_object_get_main_trans (*/
          /*MA_SELECTIONS->midi_notes[0]);*/
      /*UndoableAction * ua =*/
        /*arranger_selections_action_new_resize (*/
          /*(ArrangerSelections *) MA_SELECTIONS,*/
          /*ARRANGER_SELECTIONS_ACTION_RESIZE_L,*/
          /*trans_note->pos.total_ticks -*/
          /*trans_note->cache_pos.total_ticks);*/
      /*undo_manager_perform (*/
        /*UNDO_MANAGER, ua);*/
      /*arranger_selections_reset_counterparts (*/
        /*(ArrangerSelections *) MA_SELECTIONS, 1);*/
    /*}*/
      /*break;*/
    /*case UI_OVERLAY_ACTION_RESIZING_R:*/
    /*{*/
      /*ArrangerObject * trans_note =*/
        /*arranger_object_get_main_trans (*/
          /*MA_SELECTIONS->midi_notes[0]);*/
      /*UndoableAction * ua =*/
        /*arranger_selections_action_new_resize (*/
          /*(ArrangerSelections *) MA_SELECTIONS,*/
          /*ARRANGER_SELECTIONS_ACTION_RESIZE_R,*/
          /*trans_note->end_pos.total_ticks -*/
          /*trans_note->cache_end_pos.total_ticks);*/
      /*undo_manager_perform (*/
        /*UNDO_MANAGER, ua);*/
      /*arranger_selections_reset_counterparts (*/
        /*(ArrangerSelections *) MA_SELECTIONS, 1);*/
    /*}*/
      /*break;*/
    /*case UI_OVERLAY_ACTION_STARTING_MOVING:*/
    /*{*/
      /* if something was clicked with ctrl without
       * moving*/
      /*if (self->ctrl_held)*/
        /*if (self->start_object &&*/
            /*arranger_object_is_selected (*/
              /*self->start_object))*/
          /*{*/
            /*[> deselect it <]*/
            /*arranger_object_select (*/
              /*self->start_object,*/
              /*F_NO_SELECT, F_APPEND);*/
          /*}*/
    /*}*/
      /*break;*/
    /*case UI_OVERLAY_ACTION_MOVING:*/
    /*{*/
      /*ArrangerObject * note_obj =*/
        /*arranger_object_get_main (*/
          /*MA_SELECTIONS->midi_notes[0]);*/
      /*MidiNote * mn =*/
        /*(MidiNote *) note_obj;*/
      /*long ticks_diff =*/
        /*note_obj->pos.total_ticks -*/
          /*note_obj->cache_pos.total_ticks;*/
      /*int pitch_diff =*/
        /*mn->val - mn->cache_val;*/
      /*UndoableAction * ua =*/
        /*arranger_selections_action_new_move_midi (*/
          /*MA_SELECTIONS, ticks_diff, pitch_diff);*/
      /*undo_manager_perform (*/
        /*UNDO_MANAGER, ua);*/
      /*arranger_selections_reset_counterparts (*/
        /*(ArrangerSelections *) MA_SELECTIONS, 1);*/
    /*}*/
      /*break;*/
    /*[> if copy/link-moved <]*/
    /*case UI_OVERLAY_ACTION_MOVING_COPY:*/
    /*case UI_OVERLAY_ACTION_MOVING_LINK:*/
    /*{*/
      /*ArrangerObject * note_obj =*/
        /*arranger_object_get_main (*/
          /*MA_SELECTIONS->midi_notes[0]);*/
      /*MidiNote * mn =*/
        /*(MidiNote *) note_obj;*/
      /*long ticks_diff =*/
        /*note_obj->pos.total_ticks -*/
          /*note_obj->cache_pos.total_ticks;*/
      /*int pitch_diff =*/
        /*mn->val - mn->cache_val;*/
      /*arranger_selections_reset_counterparts (*/
        /*(ArrangerSelections *) MA_SELECTIONS, 0);*/
      /*UndoableAction * ua =*/
        /*(UndoableAction *)*/
        /*arranger_selections_action_new_duplicate_midi (*/
          /*(ArrangerSelections *) MA_SELECTIONS,*/
          /*ticks_diff,*/
          /*pitch_diff);*/
      /*undo_manager_perform (*/
        /*UNDO_MANAGER, ua);*/
    /*}*/
      /*break;*/
    /*case UI_OVERLAY_ACTION_NONE:*/
    /*{*/
      /*arranger_selections_clear (*/
        /*(ArrangerSelections *) MA_SELECTIONS);*/
    /*}*/
      /*break;*/
    /*[> something was created <]*/
    /*case UI_OVERLAY_ACTION_CREATING_RESIZING_R:*/
    /*{*/
      /*UndoableAction * ua =*/
        /*arranger_selections_action_new_create (*/
          /*(ArrangerSelections *) MA_SELECTIONS);*/
      /*undo_manager_perform (*/
        /*UNDO_MANAGER, ua);*/
    /*}*/
      /*break;*/
    /*[> if didn't click on something <]*/
    /*default:*/
    /*{*/
    /*}*/
      /*break;*/
    /*}*/

  /*self->start_object = NULL;*/
  /*[>if (self->start_midi_note_clone)<]*/
    /*[>{<]*/
      /*[>midi_note_free (self->start_midi_note_clone);<]*/
      /*[>self->start_midi_note_clone = NULL;<]*/
    /*[>}<]*/

  /*EVENTS_PUSH (*/
    /*ET_ARRANGER_SELECTIONS_CHANGED, MA_SELECTIONS);*/
/*}*/

