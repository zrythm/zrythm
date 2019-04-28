/*
 * Copyright (C) 2019 Alexandros Theodotou <alex at zrythm dot org>
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

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
      VelocityWidget * vw = Z_VELOCITY_WIDGET (widget);
      MidiNote * mn = vw->velocity->midi_note;

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
        1,
        0);
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
  self->start_vel_val = self->start_velocity->vel;
  array_append (self->velocities,
                self->num_velocities,
                vel_w->velocity);

  /* update arranger action */
  if (vel_w->cursor_state ==
        UI_CURSOR_STATE_RESIZE_UP)
    ar_prv->action = UI_OVERLAY_ACTION_RESIZING_UP;
  else
    ar_prv->action = UI_OVERLAY_ACTION_NONE;

  /* select/ deselect midi_notes */
  if (ar_prv->ctrl_held)
    {
      /* if ctrl pressed toggle on/off */
      ARRANGER_WIDGET_SELECT_MIDI_NOTE (
        MIDI_ARRANGER,
        vel_w->velocity->midi_note, 1, 1, 0);
    }
  else if (!array_contains (
            (void **)self->velocities,
           self->num_velocities,
           vel_w->velocity))
    {
      /* else if not already selected select only it */
      midi_arranger_widget_select_all (
        MIDI_ARRANGER, 0);
      ARRANGER_WIDGET_SELECT_MIDI_NOTE (
        MIDI_ARRANGER,
        vel_w->velocity->midi_note, 1, 0, 0);
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

  double px = ar_prv->start_y + offset_y;
  double ratio = px / height;
  ratio = 1.0 - ratio;

  self->start_velocity->vel =
    CLAMP (ratio * 127, 1, 127);
  int diff = self->start_velocity->vel -
    self->start_vel_val;
  if (self->start_velocity->widget)
    velocity_widget_update_tooltip (
      self->start_velocity->widget, 1);
  /*g_message ("diff %d", diff);*/
  for (int i = 0; i < self->num_velocities; i++)
    {
      Velocity * vel = self->velocities[i];
      if (vel == self->start_velocity)
        continue;
      vel->vel = CLAMP (vel->vel + diff, 1, 127);
      g_message ("set vel to %d",
                 vel->vel);

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
  for (int i = 0;
       i < self->num_velocities;
       i++)
    {
      Velocity * vel =
        self->velocities[i];
      ui_set_cursor_from_name (
        GTK_WIDGET (vel->widget), "default");
      if (vel->widget)
        velocity_widget_update_tooltip (
          vel->widget, 0);
    }
  self->start_velocity = NULL;

  ARRANGER_WIDGET_GET_PRIVATE (self);

  /* if didn't click on something */
  if (ar_prv->action !=
        UI_OVERLAY_ACTION_STARTING_MOVING)
    {
      self->start_velocity = NULL;
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

  switch (action)
    {
    case UI_OVERLAY_ACTION_NONE:
      if (P_TOOL == TOOL_SELECT_NORMAL ||
           P_TOOL == TOOL_SELECT_STRETCH ||
           P_TOOL == TOOL_EDIT)
        {
          MidiNoteWidget * mnw = NULL;

          int is_hit =
            mnw != NULL;
          int is_resize_l =
            mnw && mnw->resize_l;
          int is_resize_r =
            mnw && mnw->resize_r;

          if (is_hit && is_resize_l)
            {
              return ARRANGER_CURSOR_RESIZING_L;
            }
          else if (is_hit && is_resize_r)
            {
              return ARRANGER_CURSOR_RESIZING_R;
            }
          else if (is_hit)
            {
              return ARRANGER_CURSOR_GRAB;
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
    default:
      g_warn_if_reached ();
      ac = ARRANGER_CURSOR_SELECT;
      break;
    }

  return ac;

}

/**
 * Readd children.
 */
void
midi_modifier_arranger_widget_refresh_children (
  MidiModifierArrangerWidget * self)
{

  /* TODO */
}

static void
midi_modifier_arranger_widget_class_init (MidiModifierArrangerWidgetClass * klass)
{
}

static void
midi_modifier_arranger_widget_init (MidiModifierArrangerWidget * self)
{
}
