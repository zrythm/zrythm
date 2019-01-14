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
#include "gui/widgets/arranger.h"
#include "gui/widgets/center_dock.h"
#include "gui/widgets/midi_modifier_arranger.h"
#include "gui/widgets/ruler.h"
#include "gui/widgets/timeline_ruler.h"
#include "gui/widgets/velocity.h"

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
      allocation->width = 8;
      allocation->height = vel_px;
    }
}

MidiNote *
midi_modifier_arranger_widget_get_midi_note_at_x (double x)
{

  /* TODO */
  return NULL;
}

Velocity *
midi_modifier_arranger_widget_get_hit_velocity (
  MidiModifierArrangerWidget *  self,
  double                        x,
  double                        y)
{

  /* TODO */
  return NULL;
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

void
midi_modifier_arranger_widget_toggle_select_velocity (
  MidiModifierArrangerWidget * self,
  Velocity                     vel,
  int                          append)
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
midi_modifier_arranger_widget_on_drag_begin_velocity_hit (
  MidiModifierArrangerWidget * self,
  GdkModifierType              state_mask,
  double                       start_y,
  VelocityWidget *             vw)
{

  /* TODO */
}

void
midi_modifier_arranger_widget_find_start_poses (
  MidiModifierArrangerWidget * self)
{

  /* TODO */
}

void
midi_modifier_arranger_widget_find_and_select_items (
  MidiModifierArrangerWidget * self,
  double                   offset_x,
  double                   offset_y)
{

  /* TODO */
}

void
midi_modifier_arranger_widget_move_items_y (
  MidiModifierArrangerWidget * self,
  double                       offset_y)
{

  /* TODO */
}

void
midi_modifier_arranger_widget_on_drag_end (
  MidiModifierArrangerWidget * self)
{

  /* TODO */
}

void
midi_modifier_arranger_widget_setup (
  MidiModifierArrangerWidget * self)
{
  /* set arranger size */
  RULER_WIDGET_GET_PRIVATE (MW_RULER);
  gtk_widget_set_size_request (
    GTK_WIDGET (self),
    prv->total_px,
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
