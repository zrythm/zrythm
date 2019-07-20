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

#ifndef __GUI_WIDGETS_MIDI_MODIFIER_ARRANGER_H__
#define __GUI_WIDGETS_MIDI_MODIFIER_ARRANGER_H__

#include "gui/backend/tool.h"
#include "gui/widgets/arranger.h"

#include <gtk/gtk.h>

#define MIDI_MODIFIER_ARRANGER_WIDGET_TYPE \
  (midi_modifier_arranger_widget_get_type ())
G_DECLARE_FINAL_TYPE (
  MidiModifierArrangerWidget,
  midi_modifier_arranger_widget,
  Z, MIDI_MODIFIER_ARRANGER_WIDGET,
  ArrangerWidget)

#define MW_MIDI_MODIFIER_ARRANGER \
  MW_MIDI_EDITOR_SPACE->modifier_arranger

typedef struct Velocity Velocity;
typedef struct _VelocityWidget VelocityWidget;

typedef struct _MidiModifierArrangerWidget
{
  ArrangerWidget parent_instance;

  /** Clicked Velocity. */
  Velocity *     start_velocity;

  /** 1-127. */
  int            start_vel_val;

  /**
   * Maximum Velocity diff applied in this action.
   *
   * Used in drag_end to create an UndableAction.
   * This can have any value, even greater than 127
   * and it will be clamped when applying it to
   * a Velocity.
   */
  int            vel_diff;
} MidiModifierArrangerWidget;

ARRANGER_W_DECLARE_FUNCS (
  MidiModifier, midi_modifier);
ARRANGER_W_DECLARE_CHILD_OBJ_FUNCS (
  MidiModifier, midi_modifier, Velocity, velocity);

void
midi_modifier_arranger_widget_resize_velocities (
  MidiModifierArrangerWidget * self,
  double                       offset_y);

/**
 * Draws a ramp from the start coordinates to the
 * given coordinates.
 */
void
midi_modifier_arranger_widget_ramp (
  MidiModifierArrangerWidget * self,
  double                       offset_x,
  double                       offset_y);

#endif
