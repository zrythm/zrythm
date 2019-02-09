/*
 * gui/widgets/midi_modifier_arranger.h - arranger for box
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

#ifndef __GUI_WIDGETS_MIDI_MODIFIER_ARRANGER_H__
#define __GUI_WIDGETS_MIDI_MODIFIER_ARRANGER_H__

#include "gui/widgets/arranger.h"

#include <gtk/gtk.h>

#define MIDI_MODIFIER_ARRANGER_WIDGET_TYPE \
  (midi_modifier_arranger_widget_get_type ())
G_DECLARE_FINAL_TYPE (MidiModifierArrangerWidget,
                      midi_modifier_arranger_widget,
                      Z,
                      MIDI_MODIFIER_ARRANGER_WIDGET,
                      ArrangerWidget)

#define MIDI_MODIFIER_ARRANGER \
  MW_PIANO_ROLL->modifier_arranger

typedef struct Velocity Velocity;
typedef struct _VelocityWidget VelocityWidget;

typedef struct _MidiModifierArrangerWidget
{
  ArrangerWidget           parent_instance;

  /**
   * Velocities doing action upon.
   */
  Velocity *               velocities[600];
  int                      num_velocities;
  Velocity *               start_velocity; ///< clicked velocity

  /* temporary start positions, set on drag_begin, and used in drag_update
   * to resize the objects accordingly */
  int                      velocity_start_values[600];
} MidiModifierArrangerWidget;

/**
 * To be called from get_child_position in parent widget.
 *
 * Used to allocate the overlay children.
 */
void
midi_modifier_arranger_widget_set_allocation (
  MidiModifierArrangerWidget * self,
  GtkWidget *          widget,
  GdkRectangle *       allocation);

MidiNote *
midi_modifier_arranger_widget_get_midi_note_at_x (double x);

Velocity *
midi_modifier_arranger_widget_get_hit_velocity (
  MidiModifierArrangerWidget *  self,
  double                        x,
  double                        y);

void
midi_modifier_arranger_widget_update_inspector (
  MidiModifierArrangerWidget * self);

void
midi_modifier_arranger_widget_select_all (
  MidiModifierArrangerWidget *  self,
  int                           select);

void
midi_modifier_arranger_widget_toggle_select_velocity (
  MidiModifierArrangerWidget * self,
  Velocity                     vel,
  int                          append);

/**
 * Shows context menu.
 *
 * To be called from parent on right click.
 */
void
midi_modifier_arranger_widget_show_context_menu (
  MidiModifierArrangerWidget * self);

void
midi_modifier_arranger_widget_on_drag_begin_velocity_hit (
  MidiModifierArrangerWidget * self,
  GdkModifierType              state_mask,
  double                       start_y,
  VelocityWidget *             vw);

void
midi_modifier_arranger_widget_find_start_poses (
  MidiModifierArrangerWidget * self);

void
midi_modifier_arranger_widget_find_and_select_items (
  MidiModifierArrangerWidget * self,
  double                   offset_x,
  double                   offset_y);

void
midi_modifier_arranger_widget_move_items_y (
  MidiModifierArrangerWidget * self,
  double                       offset_y);

void
midi_modifier_arranger_widget_on_drag_end (
  MidiModifierArrangerWidget * self);

void
midi_modifier_arranger_widget_setup (
  MidiModifierArrangerWidget * self);

/**
 * Readd children.
 */
void
midi_modifier_arranger_widget_refresh_children (
  MidiModifierArrangerWidget * self);

#endif
