/*
 * audio/automation_track.h - Automation track
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

#ifndef __AUDIO_AUTOMATION_TRACK_H__
#define __AUDIO_AUTOMATION_TRACK_H__

#include "audio/position.h"
#include "audio/automation_point.h"

#define MAX_AUTOMATION_POINTS 1200

enum GenerateCurvePoints
{
  NO_GENERATE_CURVE_POINTS,
  GENERATE_CURVE_POINTS
};

typedef struct Port Port;
typedef struct AutomationTrackWidget AutomationTrackWidget;
typedef struct Track Track;
typedef struct Automatable Automatable;

typedef struct AutomationTrack
{
  Automatable *             automatable; ///< the automatable
  AutomationTrackWidget *   widget;
  Track *                   track; ///< owner track
  AutomationPoint *         automation_points[MAX_AUTOMATION_POINTS];
                                      ///< automation points TODO sort every time
  int                       num_automation_points;
  int                       visible;
} AutomationTrack;

/**
 * Creates an automation track for the given automatable
 */
AutomationTrack *
automation_track_new(Track *        track,
                     Automatable *  automatable);

/**
 * Sets the automatable to the automation track and updates the GUI
 */
void
automation_track_set_automatable (AutomationTrack * automation_track,
                                  Automatable *     a);

/**
 * Gets automation track for given automatable, if any.
 */
AutomationTrack *
automation_track_get_for_automatable (Automatable * automatable);

void
automation_track_free (AutomationTrack * at);

void
automation_track_print_automation_points (AutomationTrack * at);

/**
 * Forces sort of the automation points.
 */
void
automation_track_force_sort (AutomationTrack * at);

/**
 * Adds automation point and optionally generates curve points accordingly.
 */
void
automation_track_add_automation_point (AutomationTrack * at,
                                       AutomationPoint * ap,
                                       int               generate_curve_points);

/**
 * Returns the automation point before the position.
 */
AutomationPoint *
automation_track_get_prev_ap (AutomationTrack * at,
                              AutomationPoint * _ap);

/**
 * Returns the automation point after the position.
 */
AutomationPoint *
automation_track_get_next_ap (AutomationTrack * at,
                              AutomationPoint * _ap);

/**
 * Returns the curve point right after the given ap
 */
AutomationPoint *
automation_track_get_next_curve_ap (AutomationTrack * at,
                                    AutomationPoint * _ap);

/**
 * Removes automation point from automation track.
 */
void
automation_track_remove_ap (AutomationTrack * at,
                            AutomationPoint * ap);

int
automation_track_get_index (AutomationTrack * at,
                            AutomationPoint * ap);

/**
 * Updates automation track & its GUI
 */
//void
//automation_track_update (AutomationTrack * at);

#endif // __AUDIO_AUTOMATION_TRACK_H__
