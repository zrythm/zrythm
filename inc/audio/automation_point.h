/*
 * audio/automation_point.h - Automation point
 *   and an end
 *
 * Copyright (C) 2018 Alexandros Theodotou
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
 * along with Zrythm.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef __AUDIO_AUTOMATION_POINT_H__
#define __AUDIO_AUTOMATION_POINT_H__

#include "audio/position.h"

typedef struct AutomationTrack AutomationTrack;
typedef struct AutomationPointWidget AutomationPointWidget;

typedef struct AutomationPoint
{
  int                      id;
  Position                 pos;
  float                    fvalue; ///< float value
  int                      bvalue; ///< boolean value
  int                      svalue; ///< step value
  AutomationTrack *        at; ///< pointer back to parent
  AutomationPointWidget *  widget;
} AutomationPoint;

/**
 * Creates automation point in given track at given Position
 */
AutomationPoint *
automation_point_new_float (AutomationTrack *   at,
                            float               value,
                            Position *          pos);

int
automation_point_get_y_in_px (AutomationPoint * ap);

/**
 * Updates the value and notifies interested parties.
 */
void
automation_point_update_fvalue (AutomationPoint * ap,
                                float             fval);

/**
 * Frees the automation point.
 */
void
automation_point_free (AutomationPoint * ap);

#endif // __AUDIO_AUTOMATION_POINT_H__

