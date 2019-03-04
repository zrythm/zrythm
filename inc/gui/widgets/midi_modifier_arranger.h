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
  /** 1-127 */
  int                      start_vel_val;

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

VelocityWidget *
midi_modifier_arranger_widget_get_hit_velocity (
  MidiModifierArrangerWidget *  self,
  double                        x,
  double                        y);

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
  double               offset_y);

void
midi_modifier_arranger_widget_update_inspector (
  MidiModifierArrangerWidget * self);

void
midi_modifier_arranger_widget_select_all (
  MidiModifierArrangerWidget *  self,
  int                           select);

/**
 * Shows context menu.
 *
 * To be called from parent on right click.
 */
void
midi_modifier_arranger_widget_show_context_menu (
  MidiModifierArrangerWidget * self);

void
midi_modifier_arranger_widget_resize_velocities (
  MidiModifierArrangerWidget * self,
  double                       offset_y);

void
midi_modifier_arranger_on_drag_begin_vel_hit (
  MidiModifierArrangerWidget * self,
  GdkModifierType          state_mask,
  VelocityWidget *             vel_w,
  double                       start_y);

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
