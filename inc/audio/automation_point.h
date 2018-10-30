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

#define AP_MAX_CURVINESS 6.f
static const float _ap_min_curviness = 1.f / AP_MAX_CURVINESS;
#define AP_MIN_CURVINESS _ap_min_curviness
static const float _ap_curviness_range = AP_MAX_CURVINESS - AP_MIN_CURVINESS;
#define AP_CURVINESS_RANGE _ap_curviness_range
#define AP_MID_CURVINESS 1.f

typedef struct AutomationTrack AutomationTrack;
typedef struct AutomationPointWidget AutomationPointWidget;

typedef enum AutomationPointType
{
  AUTOMATION_POINT_VALUE, ///< point that changes the point value when you move it
  AUTOMATION_POINT_CURVE ///< point that changes the curve when you move it
} AutomationPointType;

typedef struct AutomationPoint
{
  Position                 pos;
  float                    fvalue; ///< float value
  int                      bvalue; ///< boolean value
  int                      svalue; ///< step value
  float                    curviness; ///< curviness, default 1
  AutomationPointType      type;
  AutomationTrack *        at; ///< pointer back to parent
  AutomationPointWidget *  widget;
} AutomationPoint;

/**
 * Creates automation point in given track at given Position
 */
AutomationPoint *
automation_point_new_float (AutomationTrack *   at,
                            AutomationPointType type,
                            float               value,
                            Position *          pos);

/**
 * Creates curviness point in given track at given Position
 */
AutomationPoint *
automation_point_new_curve (AutomationTrack *   at,
                            Position *          pos);

int
automation_point_get_y_in_px (AutomationPoint * ap);

/**
 * Frees the automation point.
 */
void
automation_point_free (AutomationPoint * ap);

/**
 * The function to return a point on the curve.
 *
 * See https://stackoverflow.com/questions/17623152/how-map-tween-a-number-based-on-a-dynamic-curve
 */
int
automation_point_get_y_px (AutomationPoint * start_ap, ///< start point (0, 0)
                           int               x, ///< x coordinate in px
                           int               width, ///< total width in px
                           int               height); ///< total height in px

#endif // __AUDIO_AUTOMATION_POINT_H__

