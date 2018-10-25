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

typedef enum AutomationPointType
{
  AUTOMATION_POINT_VALUE, ///< point that changes the point value when you move it
  AUTOMATION_POINT_CURVE ///< point that changes the curve when you move it
} AutomationPointType;

typedef struct AutomationPoint
{
  Position              pos;
  float                 fvalue; ///< float value
  int                   bvalue; ///< boolean value
  int                   svalue; ///< step value
  AutomationPointType   type;
  AutomationTrack *     automation_track; ///< pointer back to parent
} AutomationPoint;

AutomationPoint *
automation_point_new (Track * track,
            Position * start_pos,
            Position * end_pos);

#endif // __AUDIO_AUTOMATION_POINT_H__

