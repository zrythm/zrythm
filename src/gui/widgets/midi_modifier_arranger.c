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

#include "actions/edit_midi_arranger_selections_action.h"
#include "audio/midi_note.h"
#include "audio/velocity.h"
#include "gui/widgets/arranger.h"
#include "gui/widgets/bot_dock_edge.h"
#include "gui/widgets/center_dock.h"
#include "gui/widgets/clip_editor.h"
#include "gui/widgets/midi_arranger.h"
#include "gui/widgets/midi_modifier_arranger.h"
#include "gui/widgets/midi_note.h"
#include "gui/widgets/ruler.h"
#include "gui/widgets/timeline_ruler.h"
#include "gui/widgets/velocity.h"
#include "project.h"
#include "utils/arrays.h"
#include "utils/flags.h"

G_DEFINE_TYPE (MidiModifierArrangerWidget,
               midi_modifier_arranger_widget,
               ARRANGER_WIDGET_TYPE)

/**
 * To be called from get_child_position in parent widget.
 *
 * Used to allocate the overlay children.
 */
void
midi_modifier_arranger_widget_set_allocation (
  MidiModifierArrangerWidget * self,
  GtkWidget *          widget,
  GdkRectangle *       allocation)
{
  if (Z_IS_VELOCITY_WIDGET (widget))
    {
      VelocityWidget * vw =
        Z_VELOCITY_WIDGET (widget);

      /* use transient or non transient note
       * depending on which is visible */
      MidiNote * mn = vw->velocity->midi_note;
      mn = midi_note_get_visible (mn);

      gint wx, wy;
      gtk_widget_translate_coordinates(
                GTK_WIDGET (mn->widget),
                GTK_WIDGET (self),
                0,
                0,
                &wx,
                &wy);
      guint height;
      height = gtk_widget_get_allocated_height (
        GTK_WIDGET (self));

      int vel_px =
        height *
        ((float) vw->velocity->vel / 127.f);
      allocation->x = wx;
      allocation->y = height - vel_px;
      allocation->width = 12;
      allocation->height = vel_px;
    }
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
void
midi_modifier_arranger_widget_select (
  MidiModifierArrangerWidget * self,
  double               offset_x,
  double               offset_y,
  int                  delete)
{
  ARRANGER_WIDGET_GET_PRIVATE (self);

  /* deselect all */
  arranger_widget_select_all (
    Z_ARRANGER_WIDGET (self), 0);

  /* find enclosed velocities */
  GtkWidget *    velocities[800];
  int            num_velocities = 0;
  arranger_widget_get_hit_widgets_in_range (
    Z_ARRANGER_WIDGET (self),
    VELOCITY_WIDGET_TYPE,
    ar_prv->start_x,
    ar_prv->start_y,
    offset_x,
    offset_y,
    velocities,
    &num_velocities);

  /* select the enclosed midi_notes */
  for (int i = 0; i < num_velocities; i++)
    {
      VelocityWidget * vel_w =
        Z_VELOCITY_WIDGET (velocities[i]);
      Velocity * vel =
        vel_w->velocity;
      ARRANGER_WIDGET_SELECT_MIDI_NOTE (
        MIDI_ARRANGER,
        vel->midi_note,
        1,
        1);
    }
}

VelocityWidget *
midi_modifier_arranger_widget_get_hit_velocity (
  MidiModifierArrangerWidget *  self,
  double                        x,
  double                        y)
{
  GtkWidget * widget =
    ui_get_hit_child (
      GTK_CONTAINER (self),
      x,
      y,
      VELOCITY_WIDGET_TYPE);
  if (widget)
    return Z_VELOCITY_WIDGET (widget);

  return NULL;
}

void
midi_modifier_arranger_on_drag_begin_vel_hit (
  MidiModifierArrangerWidget * self,
  VelocityWidget *             vel_w,
  double                       start_y)
{
  ARRANGER_WIDGET_GET_PRIVATE (self);

  self->start_velocity = vel_w->velocity;

  /* update arranger action */
  if (vel_w->resize)
    ar_prv->action = UI_OVERLAY_ACTION_RESIZING_UP;
  else
    ar_prv->action = UI_OVERLAY_ACTION_NONE;

  int selected =
    velocity_is_selected (self->start_velocity);

  MidiNote * mn = self->start_velocity->midi_note;

  /* select midi note if unselected */
  if (P_TOOL == TOOL_EDIT ||
      P_TOOL == TOOL_SELECT_NORMAL ||
      P_TOOL == TOOL_SELECT_STRETCH)
    {
      /* if ctrl held & not selected, add to
       * selections */
      if (ar_prv->ctrl_held &&
          !selected)
        {
          ARRANGER_WIDGET_SELECT_MIDI_NOTE (
            self, mn, F_SELECT, F_APPEND);
        }
      /* if ctrl not held & not selected, make it
       * the only
       * selection */
      else if (!ar_prv->ctrl_held &&
               !selected)
        {
          ARRANGER_WIDGET_SELECT_MIDI_NOTE (
            self, mn, F_SELECT, F_NO_APPEND);
        }
    }
}

void
midi_modifier_arranger_widget_update_inspector (
  MidiModifierArrangerWidget * self)
{

  /* TODO */
}

void
midi_modifier_arranger_widget_select_all (
  MidiModifierArrangerWidget *  self,
  int                           select)
{

  /* TODO */
}

/**
 * Shows context menu.
 *
 * To be called from parent on right click.
 */
void
midi_modifier_arranger_widget_show_context_menu (
  MidiModifierArrangerWidget * self)
{

  /* TODO */
}

void
midi_modifier_arranger_widget_resize_velocities (
  MidiModifierArrangerWidget * self,
  double                       offset_y)
{
  ARRANGER_WIDGET_GET_PRIVATE (self);

  guint height =
    gtk_widget_get_allocated_height (
      GTK_WIDGET (self));

  double start_ratio =
    1.0 - ar_prv->start_y / height;
  double ratio =
    1.0 - (ar_prv->start_y + offset_y) / height;
  int start_val = start_ratio * 127;
  int val = ratio * 127;
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

  Velocity * vel;
  for (int i = 0;
       i < MA_SELECTIONS->num_midi_notes; i++)
    {
      vel =
        MA_SELECTIONS->midi_notes[i]->vel;

      vel =
        velocity_get_main_trans_velocity (vel);
      velocity_set_val (
        vel,
        CLAMP (
          vel->cache_vel + self->vel_diff,
          1, 127),
        AO_UPDATE_ALL);

      /*g_message ("set val to %d (transient? %d)",*/
                 /*vel->vel,*/
                 /*velocity_is_transient (vel));*/

      if (vel->widget)
        velocity_widget_update_tooltip (
          vel->widget, 1);
    }
}

/**
 * Called on drag end.
 *
 * Sets default cursors back and sets the start midi note
 * to NULL if necessary.
 */
void
midi_modifier_arranger_widget_on_drag_end (
  MidiModifierArrangerWidget * self)
{
  ARRANGER_WIDGET_GET_PRIVATE (self);

  MidiNote * midi_note;
  Velocity * vel;
  for (int i = 0;
       i < MA_SELECTIONS->num_midi_notes;
       i++)
    {
      midi_note =
        MA_SELECTIONS->midi_notes[i];
      vel = midi_note->vel;

      if (vel->widget)
        velocity_widget_update_tooltip (
          vel->widget, 0);

      EVENTS_PUSH (ET_MIDI_NOTE_CHANGED,
                   midi_note);
    }

  midi_modifier_arranger_widget_update_visibility (
    self);

  if (ar_prv->action ==
        UI_OVERLAY_ACTION_RESIZING_UP)
    {
      UndoableAction * ua =
        (UndoableAction *)
        emas_action_new_vel_change (
          MA_SELECTIONS,
          self->vel_diff);
      undo_manager_perform (
        UNDO_MANAGER, ua);
    }

  self->start_velocity = NULL;

  /* if didn't click on something */
  if (ar_prv->action !=
        UI_OVERLAY_ACTION_STARTING_MOVING)
    {
      self->start_velocity = NULL;
    }
}

/**
 * Sets transient Velocity and actual Velocity
 * visibility based on the current action.
 */
void
midi_modifier_arranger_widget_update_visibility (
  MidiModifierArrangerWidget * self)
{
  /* see ARRANGER_SET_OBJ_VISIBILITY_ARRAY */
  Velocity * vel;
  for (int i = 0;
       i < MA_SELECTIONS->num_midi_notes; i++)
    {
      vel = MA_SELECTIONS->midi_notes[i]->vel;
      arranger_object_info_set_widget_visibility_and_state (
        &vel->obj_info, 1);
    }
}

void
midi_modifier_arranger_widget_setup (
  MidiModifierArrangerWidget * self)
{
  /* set arranger size */
  RULER_WIDGET_GET_PRIVATE (MW_RULER);
  gtk_widget_set_size_request (
    GTK_WIDGET (self),
    rw_prv->total_px,
    -1);
}

/**
 * Returns the appropriate cursor based on the
 * current hover_x and y.
 */
ArrangerCursor
midi_modifier_arranger_widget_get_cursor (
  MidiModifierArrangerWidget * self,
  UiOverlayAction action,
  Tool            tool)
{
  ArrangerCursor ac = ARRANGER_CURSOR_SELECT;
  ARRANGER_WIDGET_GET_PRIVATE (self);

  switch (action)
    {
    case UI_OVERLAY_ACTION_NONE:
      if (tool == TOOL_SELECT_NORMAL ||
          tool == TOOL_SELECT_STRETCH ||
          tool == TOOL_EDIT)
        {
          VelocityWidget * vw =
            midi_modifier_arranger_widget_get_hit_velocity (
              self,
              ar_prv->hover_x,
              ar_prv->hover_y);

          int is_hit =
            vw != NULL;
          int is_resize =
            vw && vw->resize;

          g_message ("hit resize %d %d",
                     is_hit, is_resize);
          if (is_hit && is_resize)
            {
              return ARRANGER_CURSOR_RESIZING_UP;
            }
          else
            {
              /* set cursor to whatever it is */
              if (tool == TOOL_EDIT)
                return ARRANGER_CURSOR_EDIT;
              else
                return ARRANGER_CURSOR_SELECT;
            }
        }
      else if (P_TOOL == TOOL_EDIT)
        ac = ARRANGER_CURSOR_EDIT;
      else if (P_TOOL == TOOL_ERASER)
        ac = ARRANGER_CURSOR_ERASER;
      else if (P_TOOL == TOOL_RAMP)
        ac = ARRANGER_CURSOR_RAMP;
      else if (P_TOOL == TOOL_AUDITION)
        ac = ARRANGER_CURSOR_AUDITION;
      break;
    case UI_OVERLAY_ACTION_STARTING_DELETE_SELECTION:
    case UI_OVERLAY_ACTION_DELETE_SELECTING:
    case UI_OVERLAY_ACTION_ERASING:
      ac = ARRANGER_CURSOR_ERASER;
      break;
    case UI_OVERLAY_ACTION_STARTING_MOVING_COPY:
    case UI_OVERLAY_ACTION_MOVING_COPY:
      ac = ARRANGER_CURSOR_GRABBING_COPY;
      break;
    case UI_OVERLAY_ACTION_STARTING_MOVING:
    case UI_OVERLAY_ACTION_MOVING:
      ac = ARRANGER_CURSOR_GRABBING;
      break;
    case UI_OVERLAY_ACTION_STARTING_MOVING_LINK:
    case UI_OVERLAY_ACTION_MOVING_LINK:
      ac = ARRANGER_CURSOR_GRABBING_LINK;
      break;
    case UI_OVERLAY_ACTION_RESIZING_L:
      ac = ARRANGER_CURSOR_RESIZING_L;
      break;
    case UI_OVERLAY_ACTION_RESIZING_R:
      ac = ARRANGER_CURSOR_RESIZING_R;
      break;
    case UI_OVERLAY_ACTION_RESIZING_UP:
      ac = ARRANGER_CURSOR_RESIZING_UP;
      break;
    case UI_OVERLAY_ACTION_STARTING_SELECTION:
    case UI_OVERLAY_ACTION_SELECTING:
      ac = ARRANGER_CURSOR_SELECT;
      /* TODO depends on tool */
      break;
    default:
      g_warn_if_reached ();
      ac = ARRANGER_CURSOR_SELECT;
      break;
    }

  return ac;

}

static inline void
add_children_from_region (
  MidiModifierArrangerWidget * self,
  Region *             region)
{
  int i, j;
  MidiNote * mn;
  Velocity * vel;
  for (i = 0; i < region->num_midi_notes; i++)
    {
      mn = region->midi_notes[i];

      for (j = 0; j < 2; j++)
        {
          if (j == 0)
            vel =
              velocity_get_main_velocity (
                mn->vel);
          else if (j == 1)
            vel =
              velocity_get_main_trans_velocity (
                mn->vel);

          if (!vel->widget)
            vel->widget =
              velocity_widget_new (vel);

          gtk_overlay_add_overlay (
            GTK_OVERLAY (self),
            GTK_WIDGET (vel->widget));
        }
    }
}

/**
 * Readd children.
 */
void
midi_modifier_arranger_widget_refresh_children (
  MidiModifierArrangerWidget * self)
{
  ARRANGER_WIDGET_GET_PRIVATE (self);
  int i, k;

  /* remove all children except bg */
  GList *children, *iter;

  children =
    gtk_container_get_children (
      GTK_CONTAINER (self));
  for (iter = children;
       iter != NULL;
       iter = g_list_next (iter))
    {
      GtkWidget * widget = GTK_WIDGET (iter->data);
      if (widget != (GtkWidget *) ar_prv->bg &&
          widget != (GtkWidget *) ar_prv->playhead)
        {
          g_object_ref (widget);
          gtk_container_remove (
            GTK_CONTAINER (self),
            widget);
        }
    }
  g_list_free (children);

  if (CLIP_EDITOR->region &&
      CLIP_EDITOR->region->type == REGION_TYPE_MIDI)
    {
      /* add notes of all regions in the track */
      TrackLane * lane;
      Track * track =
        CLIP_EDITOR->region->lane->track;
      for (k = 0; k < track->num_lanes; k++)
        {
          lane = track->lanes[k];

          for (i = 0; i < lane->num_regions; i++)
            {
              add_children_from_region (
                self, lane->regions[i]);
            }
        }
    }

  gtk_overlay_reorder_overlay (
    GTK_OVERLAY (self),
    (GtkWidget *) ar_prv->playhead, -1);
}

static void
midi_modifier_arranger_widget_class_init (
  MidiModifierArrangerWidgetClass * klass)
{
}

static void
midi_modifier_arranger_widget_init (
  MidiModifierArrangerWidget * self)
{
}
