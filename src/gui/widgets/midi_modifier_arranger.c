/*
 * Copyright (C) 2019-2022 Alexandros Theodotou <alex at zrythm dot org>
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

#include "actions/arranger_selections.h"
#include "dsp/midi_note.h"
#include "dsp/velocity.h"
#include "gui/backend/event.h"
#include "gui/backend/event_manager.h"
#include "gui/widgets/arranger.h"
#include "gui/widgets/bot_dock_edge.h"
#include "gui/widgets/center_dock.h"
#include "gui/widgets/clip_editor.h"
#include "gui/widgets/clip_editor_inner.h"
#include "gui/widgets/midi_arranger.h"
#include "gui/widgets/midi_editor_space.h"
#include "gui/widgets/midi_modifier_arranger.h"
#include "gui/widgets/midi_note.h"
#include "gui/widgets/ruler.h"
#include "gui/widgets/timeline_panel.h"
#include "gui/widgets/timeline_ruler.h"
#include "gui/widgets/velocity.h"
#include "project.h"
#include "settings/settings.h"
#include "utils/arrays.h"
#include "utils/flags.h"
#include "utils/objects.h"
#include "zrythm_app.h"

/**
 * Sets the start velocities of all velocities
 * in the current region.
 */
void
midi_modifier_arranger_widget_set_start_vel (
  ArrangerWidget * self)
{
  ZRegion * region = clip_editor_get_region (CLIP_EDITOR);
  g_return_if_fail (
    region && region->id.type == REGION_TYPE_MIDI);

  for (int i = 0; i < region->num_midi_notes; i++)
    {
      MidiNote * mn = region->midi_notes[i];
      Velocity * vel = mn->vel;
      vel->vel_at_start = vel->vel;
    }
}

/**
 * Returns the enclosed velocities.
 *
 * @param hit Return hit or unhit velocities.
 */
static Velocity **
get_enclosed_velocities (
  ArrangerWidget * self,
  double           offset_x,
  int *            num_vels,
  bool             hit)
{
  Position selection_start_pos, selection_end_pos;
  ui_px_to_pos_editor (
    self->start_x,
    offset_x >= 0 ? &selection_start_pos : &selection_end_pos,
    F_PADDING);
  ui_px_to_pos_editor (
    self->start_x + offset_x,
    offset_x >= 0 ? &selection_end_pos : &selection_start_pos,
    F_PADDING);

  ZRegion * region = clip_editor_get_region (CLIP_EDITOR);
  g_return_val_if_fail (
    region && region->id.type == REGION_TYPE_MIDI, NULL);

  /* find enclosed velocities */
  size_t      velocities_size = 1;
  Velocity ** velocities =
    object_new_n (velocities_size, Velocity *);
  *num_vels = 0;
  midi_region_get_velocities_in_range (
    region, &selection_start_pos, &selection_end_pos,
    &velocities, num_vels, &velocities_size, hit);

  return velocities;
}

void
midi_modifier_arranger_widget_select_vels_in_range (
  ArrangerWidget * self,
  double           offset_x)
{
  /* find enclosed velocities */
  int         num_velocities = 0;
  Velocity ** velocities = get_enclosed_velocities (
    self, offset_x, &num_velocities, true);

  arranger_selections_clear (
    (ArrangerSelections *) MA_SELECTIONS, F_NO_FREE,
    F_NO_PUBLISH_EVENTS);
  for (int i = 0; i < num_velocities; i++)
    {
      Velocity * vel = velocities[i];
      MidiNote * mn = velocity_get_midi_note (vel);

      arranger_object_select (
        (ArrangerObject *) mn, F_SELECT, F_APPEND,
        F_NO_PUBLISH_EVENTS);
    }

  free (velocities);
}

/**
 * Gets velocities hit by the given point, or x
 * only.
 *
 * @param y Y or -1 to ignore.
 */
/*void*/
/*midi_modifier_arranger_widget_get_hit_velocities (*/
/*ArrangerWidget *  self,*/
/*double            x,*/
/*double            y,*/
/*ArrangerObject ** objs,*/
/*int *             num_objs)*/
/*{*/
/*ZRegion * r = clip_editor_get_region (CLIP_EDITOR);*/
/*if (!r)*/
/*break;*/

/*for (int i = 0; i < r->num_midi_notes; i++)*/
/*{*/
/*MidiNote * mn = r->midi_notes[i];*/
/*Velocity * vel = mn->vel;*/
/*obj = (ArrangerObject *) vel;*/

/*if (obj->deleted_temporarily)*/
/*continue;*/

/*arranger_object_set_full_rectangle (obj, self);*/

/*add_object_if_overlap (*/
/*self, rect, x, y, array,*/
/*array_size, obj);*/
/*}*/
/*}*/

/**
 * Draws a ramp from the start coordinates to the
 * given coordinates.
 */
void
midi_modifier_arranger_widget_ramp (
  ArrangerWidget * self,
  double           offset_x,
  double           offset_y)
{
  /* find enclosed velocities */
  int         num_velocities = 0;
  Velocity ** velocities = get_enclosed_velocities (
    self, offset_x, &num_velocities, true);

  /* ramp */
  Velocity * vel;
  int        px, val;
  double     y1, y2, x1, x2;
  int        height =
    gtk_widget_get_allocated_height (GTK_WIDGET (self));
  Position start_pos;
  for (int i = 0; i < num_velocities; i++)
    {
      vel = velocities[i];
      MidiNote * mn = velocity_get_midi_note (vel);
      midi_note_get_global_start_pos (mn, &start_pos);
      px = ui_pos_to_px_editor (&start_pos, F_PADDING);

      x1 = self->start_x;
      x2 = self->start_x + offset_x;
      y1 = height - self->start_y;
      y2 = height - (self->start_y + offset_y);
      /*g_message ("x1 %f.0 x2 %f.0 y1 %f.0 y2 %f.0",*/
      /*x1, x2, y1, y2);*/

      /* y = y1 + ((y2 - y1)/(x2 - x1))*(x - x1)
       * http://stackoverflow.com/questions/2965144/ddg#2965188 */
      /* get val in pixels */
      val =
        (int) (y1 + ((y2 - y1) / (x2 - x1)) * ((double) px - x1));

      /* normalize and multiply by 127 to get
       * velocity value */
      val = (int) (((double) val / (double) height) * 127.0);
      val = CLAMP (val, 1, 127);
      /*g_message ("val %d", val);*/

      velocity_set_val (vel, val);
    }
  free (velocities);

  /* find velocities not hit */
  num_velocities = 0;
  velocities = get_enclosed_velocities (
    self, offset_x, &num_velocities, false);

  /* reset their value */
  for (int i = 0; i < num_velocities; i++)
    {
      vel = velocities[i];
      velocity_set_val (vel, vel->vel_at_start);
    }
  free (velocities);

  EVENTS_PUSH (ET_VELOCITIES_RAMPED, NULL);
}

void
midi_modifier_arranger_widget_resize_velocities (
  ArrangerWidget * self,
  double           offset_y)
{
  int height =
    gtk_widget_get_allocated_height (GTK_WIDGET (self));

  /* adjust for circle radius */
  double start_y = self->start_y;
  /*self->start_y - VELOCITY_WIDTH / 2;*/
  /*offset_y -= VELOCITY_WIDTH / 2;*/
  /*g_message ("start y %f offset y %f res %f height %f", start_y, offset_y, start_y + offset_y,*/
  /*(double) height);*/

  double start_ratio =
    CLAMP (1.0 - start_y / (double) height, 0.0, 1.0);
  double ratio = CLAMP (
    1.0 - (start_y + offset_y) / (double) height, 0.0, 1.0);
  g_return_if_fail (start_ratio <= 1.0);
  g_return_if_fail (ratio <= 1.0);
  int start_val = (int) (start_ratio * 127.0);
  int val = (int) (ratio * 127.0);
  self->vel_diff = val - start_val;

  /*Velocity * vel = self->start_velocity;*/
  /*vel =*/
  /*velocity_get_main_trans_velocity (vel);*/
  /*velocity_set_val (*/
  /*vel, CLAMP (ratio * 127, 1, 127));*/
  /*int diff = vel->vel - self->start_vel_val;*/
  /*self->vel_diff =*/
  /*if (vel->widget)*/
  /*velocity_widget_update_tooltip (*/
  /*vel->widget, 1);*/
  /*g_message ("diff %d", diff);*/

  for (int i = 0; i < MA_SELECTIONS->num_midi_notes; i++)
    {
      Velocity * vel = MA_SELECTIONS->midi_notes[i]->vel;
      Velocity * vel_at_start =
        ((MidiArrangerSelections *) self->sel_at_start)
          ->midi_notes[i]
          ->vel;

      velocity_set_val (
        vel,
        CLAMP (vel_at_start->vel + self->vel_diff, 1, 127));

      EVENTS_PUSH (ET_ARRANGER_OBJECT_CHANGED, vel);

#if 0
      ArrangerObject * vel_obj =
        (ArrangerObject *) vel;
      if (GTK_IS_WIDGET (vel_obj->widget))
        {
          arranger_object_widget_update_tooltip (
            Z_ARRANGER_OBJECT_WIDGET (
              vel_obj->widget), 1);
        }
#endif
    }

  if (self->start_object)
    {
      g_return_if_fail (
        self->start_object->type
        == ARRANGER_OBJECT_TYPE_VELOCITY);
      Velocity * vel = (Velocity *) self->start_object;
      g_settings_set_int (
        S_UI, "piano-roll-last-set-velocity", vel->vel);
    }
}

/**
 * Sets the value of each velocity hit at x to the
 * value corresponding to y.
 *
 * Used with the pencil tool.
 *
 * @param append_to_selections Append the hit
 *   velocities to the selections.
 */
void
midi_modifier_arranger_set_hit_velocity_vals (
  ArrangerWidget * self,
  double           x,
  double           y,
  bool             append_to_selections)
{
  GPtrArray * objs_arr = g_ptr_array_new ();
  arranger_widget_get_hit_objects_at_point (
    self, ARRANGER_OBJECT_TYPE_VELOCITY, x, -1, objs_arr);
  g_message ("%u velocities hit", objs_arr->len);

  int height =
    gtk_widget_get_allocated_height (GTK_WIDGET (self));
  double ratio = 1.0 - y / (double) height;
  int    val = CLAMP ((int) (ratio * 127.0), 1, 127);

  for (size_t i = 0; i < objs_arr->len; i++)
    {
      ArrangerObject * obj =
        (ArrangerObject *) g_ptr_array_index (objs_arr, i);
      Velocity *       vel = (Velocity *) obj;
      MidiNote *       mn = velocity_get_midi_note (vel);
      ArrangerObject * mn_obj = (ArrangerObject *) mn;

      /* if object not already selected, add to
       * selections */
      if (!arranger_selections_contains_object (
            (ArrangerSelections *) MA_SELECTIONS, mn_obj))
        {
          /* add a clone of midi note before the
           * change to sel_at_start */
          ArrangerObject * clone =
            arranger_object_clone ((ArrangerObject *) mn);
          arranger_selections_add_object (
            self->sel_at_start, clone);

          if (append_to_selections)
            {
              arranger_object_select (
                obj, F_SELECT, F_APPEND, F_NO_PUBLISH_EVENTS);
            }
        }

      velocity_set_val (vel, val);
    }

  g_ptr_array_unref (objs_arr);
}
