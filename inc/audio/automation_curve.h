/*
 * audio/automation_curve.h - Automation curve
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

#ifndef __AUDIO_AUTOMATION_CURVE_H__
#define __AUDIO_AUTOMATION_CURVE_H__

#include "audio/position.h"

#define AP_MAX_CURVINESS 6.f
#define AP_MIN_CURVINESS \
  (1.f / AP_MAX_CURVINESS)
#define AP_CURVINESS_RANGE \
  (AP_MAX_CURVINESS - AP_MIN_CURVINESS)
#define AP_MID_CURVINESS 1.f

typedef struct AutomationTrack AutomationTrack;
typedef struct _AutomationCurveWidget AutomationCurveWidget;

typedef enum AutomationCurveType
{
  AUTOMATION_CURVE_TYPE_BOOL,
  AUTOMATION_CURVE_TYPE_STEP,
  AUTOMATION_CURVE_TYPE_FLOAT
} AutomationCurveType;

typedef struct AutomationCurve
{
  int                      id;
  Position                 pos;
  float                    curviness; ///< curviness, default 1
  AutomationCurveType      type;
  AutomationTrack *        at; ///< pointer back to parent
  AutomationCurveWidget *  widget;
} AutomationCurve;

/**
 * Creates curviness point in given track at given Position
 */
AutomationCurve *
automation_curve_new (AutomationTrack *   at,
                      Position *          pos);

/**
 * Frees the automation point.
 */
void
automation_curve_free (AutomationCurve * ap);

/**
 * The function to return a point on the curve.
 *
 * See https://stackoverflow.com/questions/17623152/how-map-tween-a-number-based-on-a-dynamic-curve
 */
double
automation_curve_get_y_px (AutomationCurve * start_ap, ///< start point (0, 0)
                           double               x, ///< x coordinate in px
                           double               width, ///< total width in px
                           double               height); ///< total height in px

void
automation_curve_set_curviness (AutomationCurve * ac, float curviness);

#endif // __AUDIO_AUTOMATION_CURVE_H__


