/*
 * gui/widgets/midi_modifier_arranger.c - arranger for box
 *   below midi arranger for velocity, etc.
 *
 * Copyright (C) 2019 Alexandros Theodotou
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
#include "gui/widgets/ruler.h"
#include "gui/widgets/timeline_ruler.h"
#include "gui/widgets/velocity.h"
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
 */
void
midi_modifier_arranger_widget_select (
  MidiModifierArrangerWidget * self,
  double               offset_x,
  double               offset_y)
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
      midi_arranger_widget_toggle_select_midi_note (
        MIDI_ARRANGER,
        vel->midi_note,
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
  GdkModifierType          state_mask,
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
  if (state_mask & GDK_CONTROL_MASK)
    {
      /* if ctrl pressed toggle on/off */
      midi_arranger_widget_toggle_select_midi_note (
        MIDI_ARRANGER,
        vel_w->velocity->midi_note, 1);
    }
  else if (!array_contains (
            (void **)self->velocities,
           self->num_velocities,
           vel_w->velocity))
    {
      /* else if not already selected select only it */
      midi_arranger_widget_select_all (
        MIDI_ARRANGER, 0);
      midi_arranger_widget_toggle_select_midi_note (
        MIDI_ARRANGER,
        vel_w->velocity->midi_note, 0);
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
  g_message ("diff %d", diff);
  for (int i = 0; i < self->num_velocities; i++)
    {
      Velocity * vel = self->velocities[i];
      if (vel == self->start_velocity)
        continue;
      vel->vel = CLAMP (vel->vel + diff, 1, 127);
      g_message ("set vel to %d",
                 vel->vel);
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
      ui_set_cursor (
        GTK_WIDGET (vel->widget), "default");
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
