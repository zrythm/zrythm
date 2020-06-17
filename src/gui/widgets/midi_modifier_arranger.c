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

#include "actions/arranger_selections.h"
#include "audio/midi_note.h"
#include "audio/velocity.h"
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
#include "utils/arrays.h"
#include "utils/flags.h"
#include "zrythm_app.h"

/**
 * Sets the start velocities of all velocities
 * in the current region.
 */
void
midi_modifier_arranger_widget_set_start_vel (
  ArrangerWidget * self)
{
  ZRegion * region =
    clip_editor_get_region (CLIP_EDITOR);
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
    offset_x >= 0 ?
      &selection_start_pos :
      &selection_end_pos,
    F_PADDING);
  ui_px_to_pos_editor (
    self->start_x + offset_x,
    offset_x >= 0 ?
      &selection_end_pos :
      &selection_start_pos,
    F_PADDING);

  /* find enclosed velocities */
  int velocities_size = 1;
  Velocity ** velocities =
    (Velocity **)
    malloc (
      (size_t) velocities_size *
      sizeof (Velocity *));
  *num_vels = 0;
  ZRegion * region =
    clip_editor_get_region (CLIP_EDITOR);
  g_return_val_if_fail (
    region && region->id.type == REGION_TYPE_MIDI,
    NULL);
  ArrangerObject * r_obj =
    (ArrangerObject *) region;
  track_get_velocities_in_range (
    arranger_object_get_track (r_obj),
    &selection_start_pos, &selection_end_pos,
    &velocities, num_vels, &velocities_size, hit);

  return velocities;
}

void
midi_modifier_arranger_widget_select_vels_in_range (
  ArrangerWidget * self,
  double           offset_x)
{
  /* find enclosed velocities */
  int num_velocities = 0;
  Velocity ** velocities =
    get_enclosed_velocities (
      self, offset_x, &num_velocities, true);

  arranger_selections_clear (
    (ArrangerSelections *) MA_SELECTIONS);
  for (int i = 0; i < num_velocities; i++)
    {
      Velocity * vel = velocities[i];
      MidiNote * mn = velocity_get_midi_note (vel);

      arranger_object_select (
        (ArrangerObject *) mn, F_SELECT, F_APPEND);
    }

  free (velocities);
}

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
  int num_velocities = 0;
  Velocity ** velocities =
    get_enclosed_velocities (
      self, offset_x, &num_velocities, true);

  /* ramp */
  Velocity * vel;
  int px, val;
  double y1, y2, x1, x2;
  int height =
    gtk_widget_get_allocated_height (
      GTK_WIDGET (self));
  Position start_pos;
  for (int i = 0; i < num_velocities; i++)
    {
      vel = velocities[i];
      MidiNote * mn =
        velocity_get_midi_note (vel);
      midi_note_get_global_start_pos (
        mn, &start_pos);
      px =
        ui_pos_to_px_editor (
          &start_pos, F_PADDING);

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
        (int)
        (y1 +
         ((y2 - y1)/(x2 - x1)) *
           ((double) px - x1));

      /* normalize and multiply by 127 to get
       * velocity value */
      val =
        (int)
        (((double) val / (double) height) * 127.0);
      val = CLAMP (val, 1, 127);
      /*g_message ("val %d", val);*/

      velocity_set_val (
        vel, val);
    }
  free (velocities);

  /* find velocities not hit */
  num_velocities = 0;
  velocities =
    get_enclosed_velocities (
      self, offset_x, &num_velocities, false);

  /* reset their value */
  for (int i = 0; i < num_velocities; i++)
    {
      vel = velocities[i];
      velocity_set_val (vel, vel->vel_at_start);
    }
  free (velocities);
}

void
midi_modifier_arranger_widget_resize_velocities (
  ArrangerWidget * self,
  double                       offset_y)
{
  int height =
    gtk_widget_get_allocated_height (
      GTK_WIDGET (self));

  double start_ratio =
    1.0 - self->start_y / (double) height;
  double ratio =
    1.0 -
    (self->start_y + offset_y) /
      (double) height;
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

  for (int i = 0;
       i < MA_SELECTIONS->num_midi_notes; i++)
    {
      Velocity * vel =
        MA_SELECTIONS->midi_notes[i]->vel;
      Velocity * vel_at_start =
        ((MidiArrangerSelections *)
        self->sel_at_start)->midi_notes[i]->vel;

      velocity_set_val (
        vel,
        CLAMP (
          vel_at_start->vel + self->vel_diff,
          1, 127));

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
}

/**
 * Called on drag end.
 *
 * Sets default cursors back and sets the start midi note
 * to NULL if necessary.
 */
/*void*/
/*midi_modifier_arranger_widget_on_drag_end (*/
  /*ArrangerWidget * self)*/
/*{*/

  /*MidiNote * midi_note;*/
  /*Velocity * vel;*/
  /*for (int i = 0;*/
       /*i < MA_SELECTIONS->num_midi_notes;*/
       /*i++)*/
    /*{*/
      /*midi_note =*/
        /*MA_SELECTIONS->midi_notes[i];*/
      /*vel = midi_note->vel;*/

      /*ArrangerObject * vel_obj =*/
        /*(ArrangerObject *) vel;*/
      /*if (Z_IS_ARRANGER_OBJECT_WIDGET (*/
            /*vel_obj->widget))*/
        /*{*/
          /*arranger_object_widget_update_tooltip (*/
            /*(ArrangerObjectWidget *)*/
            /*vel_obj->widget, 0);*/
        /*}*/

      /*EVENTS_PUSH (*/
        /*ET_ARRANGER_OBJECT_CHANGED, midi_note);*/
    /*}*/

  /*arranger_widget_update_visibility (*/
    /*(ArrangerWidget *) self);*/

  /*switch (self->action)*/
    /*{*/
    /*case UI_OVERLAY_ACTION_RESIZING_UP:*/
      /*{*/
        /*[> FIXME <]*/
        /*UndoableAction * ua =*/
          /*arranger_selections_action_new_edit (*/
            /*(ArrangerSelections *) MA_SELECTIONS,*/
            /*(ArrangerSelections *) MA_SELECTIONS,*/
            /*ARRANGER_SELECTIONS_ACTION_EDIT_PRIMITIVE);*/
        /*undo_manager_perform (*/
          /*UNDO_MANAGER, ua);*/
      /*}*/
      /*break;*/
    /*case UI_OVERLAY_ACTION_RAMPING:*/
      /*{*/
        /*Position selection_start_pos,*/
                 /*selection_end_pos;*/
        /*ui_px_to_pos_editor (*/
          /*self->start_x,*/
          /*self->last_offset_x >= 0 ?*/
            /*&selection_start_pos :*/
            /*&selection_end_pos,*/
          /*F_PADDING);*/
        /*ui_px_to_pos_editor (*/
          /*self->start_x + self->last_offset_x,*/
          /*self->last_offset_x >= 0 ?*/
            /*&selection_end_pos :*/
            /*&selection_start_pos,*/
          /*F_PADDING);*/
        /*UndoableAction * ua =*/
          /*arranger_selections_action_new_edit (*/
            /*(ArrangerSelections *) MA_SELECTIONS,*/
            /*(ArrangerSelections *) MA_SELECTIONS,*/
            /*ARRANGER_SELECTIONS_ACTION_EDIT_PRIMITIVE);*/
        /*if (ua)*/
          /*undo_manager_perform (*/
            /*UNDO_MANAGER, ua);*/
      /*}*/
      /*break;*/
    /*default:*/
      /*break;*/
    /*}*/
/*}*/

/**
 * Returns the appropriate cursor based on the
 * current hover_x and y.
 */
/*ArrangerCursor*/
/*midi_modifier_arranger_widget_get_cursor (*/
  /*ArrangerWidget * self,*/
  /*UiOverlayAction              action,*/
  /*Tool                         tool)*/
/*{*/
  /*ArrangerCursor ac = ARRANGER_CURSOR_SELECT;*/

  /*switch (action)*/
    /*{*/
    /*case UI_OVERLAY_ACTION_NONE:*/
      /*if (tool == TOOL_SELECT_NORMAL ||*/
          /*tool == TOOL_SELECT_STRETCH ||*/
          /*tool == TOOL_EDIT)*/
        /*{*/
          /*ArrangerObject * vel_obj =*/
            /*arranger_widget_get_hit_arranger_object (*/
              /*(ArrangerWidget *) self,*/
              /*ARRANGER_OBJECT_TYPE_VELOCITY,*/
              /*self->hover_x, self->hover_y);*/
          /*int is_hit = vel_obj != NULL;*/
          /*int is_resize = 0;*/

          /*if (is_hit)*/
            /*{*/
                /*vel_obj->widget);*/
              /*is_resize = ao_prv->resize_up;*/
            /*}*/

          /*[>g_message ("hit resize %d %d",<]*/
                     /*[>is_hit, is_resize);<]*/
          /*if (is_hit && is_resize)*/
            /*{*/
              /*return ARRANGER_CURSOR_RESIZING_UP;*/
            /*}*/
          /*else*/
            /*{*/
              /*[> set cursor to whatever it is <]*/
              /*if (tool == TOOL_EDIT)*/
                /*return ARRANGER_CURSOR_EDIT;*/
              /*else*/
                /*return ARRANGER_CURSOR_SELECT;*/
            /*}*/
        /*}*/
      /*else if (P_TOOL == TOOL_EDIT)*/
        /*ac = ARRANGER_CURSOR_EDIT;*/
      /*else if (P_TOOL == TOOL_ERASER)*/
        /*ac = ARRANGER_CURSOR_ERASER;*/
      /*else if (P_TOOL == TOOL_RAMP)*/
        /*ac = ARRANGER_CURSOR_RAMP;*/
      /*else if (P_TOOL == TOOL_AUDITION)*/
        /*ac = ARRANGER_CURSOR_AUDITION;*/
      /*break;*/
    /*case UI_OVERLAY_ACTION_STARTING_DELETE_SELECTION:*/
    /*case UI_OVERLAY_ACTION_DELETE_SELECTING:*/
    /*case UI_OVERLAY_ACTION_ERASING:*/
      /*ac = ARRANGER_CURSOR_ERASER;*/
      /*break;*/
    /*case UI_OVERLAY_ACTION_STARTING_MOVING_COPY:*/
    /*case UI_OVERLAY_ACTION_MOVING_COPY:*/
      /*ac = ARRANGER_CURSOR_GRABBING_COPY;*/
      /*break;*/
    /*case UI_OVERLAY_ACTION_STARTING_MOVING:*/
    /*case UI_OVERLAY_ACTION_MOVING:*/
      /*ac = ARRANGER_CURSOR_GRABBING;*/
      /*break;*/
    /*case UI_OVERLAY_ACTION_STARTING_MOVING_LINK:*/
    /*case UI_OVERLAY_ACTION_MOVING_LINK:*/
      /*ac = ARRANGER_CURSOR_GRABBING_LINK;*/
      /*break;*/
    /*case UI_OVERLAY_ACTION_RESIZING_L:*/
      /*ac = ARRANGER_CURSOR_RESIZING_L;*/
      /*break;*/
    /*case UI_OVERLAY_ACTION_RESIZING_R:*/
      /*ac = ARRANGER_CURSOR_RESIZING_R;*/
      /*break;*/
    /*case UI_OVERLAY_ACTION_RESIZING_UP:*/
      /*ac = ARRANGER_CURSOR_RESIZING_UP;*/
      /*break;*/
    /*case UI_OVERLAY_ACTION_STARTING_SELECTION:*/
    /*case UI_OVERLAY_ACTION_SELECTING:*/
      /*ac = ARRANGER_CURSOR_SELECT;*/
      /*[> TODO depends on tool <]*/
      /*break;*/
    /*case UI_OVERLAY_ACTION_STARTING_RAMP:*/
    /*case UI_OVERLAY_ACTION_RAMPING:*/
      /*ac = ARRANGER_CURSOR_RAMP;*/
      /*break;*/
    /*default:*/
      /*g_warn_if_reached ();*/
      /*ac = ARRANGER_CURSOR_SELECT;*/
      /*break;*/
    /*}*/

  /*return ac;*/

/*}*/
