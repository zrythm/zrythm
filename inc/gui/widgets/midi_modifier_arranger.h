/*
 * SPDX-FileCopyrightText: © 2019-2021 Alexandros Theodotou <alex@zrythm.org>
 *
 * SPDX-License-Identifier: LicenseRef-ZrythmLicense
 */

#ifndef __GUI_WIDGETS_MIDI_MODIFIER_ARRANGER_H__
#define __GUI_WIDGETS_MIDI_MODIFIER_ARRANGER_H__

#include "gui/backend/tool.h"
#include "gui/widgets/arranger.h"

#include <gtk/gtk.h>

#define MW_MIDI_MODIFIER_ARRANGER (MW_MIDI_EDITOR_SPACE->modifier_arranger)

typedef struct Velocity        Velocity;
typedef struct _VelocityWidget VelocityWidget;
typedef struct _ArrangerWidget ArrangerWidget;

/**
 * @addtogroup widgets
 *
 * @{
 */

/**
 * Sets the start velocities of all velocities
 * in the current region.
 */
void
midi_modifier_arranger_widget_set_start_vel (ArrangerWidget * self);

void
midi_modifier_arranger_widget_select_vels_in_range (
  ArrangerWidget * self,
  double           offset_x);

void
midi_modifier_arranger_widget_resize_velocities (
  ArrangerWidget * self,
  double           offset_y);

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
  bool             append_to_selections);

/**
 * Draws a ramp from the start coordinates to the
 * given coordinates.
 */
void
midi_modifier_arranger_widget_ramp (
  ArrangerWidget * self,
  double           offset_x,
  double           offset_y);

/**
 * @}
 */

#endif
