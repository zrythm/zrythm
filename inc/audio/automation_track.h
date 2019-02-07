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
#include "audio/automation_curve.h"
#include "audio/automation_point.h"

#define MAX_AUTOMATION_POINTS 1200

enum GenerateCurvePoints
{
  NO_GENERATE_CURVE_POINTS,
  GENERATE_CURVE_POINTS
};

typedef struct Port Port;
typedef struct _AutomationTrackWidget AutomationTrackWidget;
typedef struct Track Track;
typedef struct Automatable Automatable;

typedef struct AutomationTrack
{
  /**
   * The automatable this automation track is for.
   */
  Automatable *             automatable;
  AutomationTrackWidget *   widget;

  /**
   * Owner track.
   *
   * For convenience only.
   */
  Track *                   track;

  /**
   * The automation points.
   *
   * Must always stay sorted by position.
   */
  AutomationPoint *         automation_points[MAX_AUTOMATION_POINTS];
  int                       num_automation_points;
  AutomationCurve *         automation_curves[MAX_AUTOMATION_POINTS];
  int                       num_automation_curves;

  int                       visible;

  /**
   * Position of multipane handle.
   */
  int                       handle_pos;
} AutomationTrack;

/**
 * Creates an automation track for the given automatable
 */
AutomationTrack *
automation_track_new (Automatable *  automatable);

/**
 * Sets the automatable to the automation track and updates the GUI
 */
void
automation_track_set_automatable (
  AutomationTrack * automation_track,
  Automatable *     a);

void
automation_track_free (AutomationTrack * at);

void
automation_track_print_automation_points (
  AutomationTrack * at);

/**
 * Forces sort of the automation points.
 */
void
automation_track_force_sort (AutomationTrack * at);

/**
 * Adds automation point and optionally generates curve points accordingly.
 */
void
automation_track_add_automation_point (
  AutomationTrack * at,
  AutomationPoint * ap,
  int               generate_curve_points);

/**
 * Adds automation curve.
 */
void
automation_track_add_automation_curve (AutomationTrack * at,
                                       AutomationCurve * ac);

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

AutomationPoint *
automation_track_get_ap_before_curve (AutomationTrack * at,
                                      AutomationCurve * ac);

/**
 * Returns the ap after the curve point.
 */
AutomationPoint *
automation_track_get_ap_after_curve (AutomationTrack * at,
                                     AutomationCurve * ac);

/**
 * Returns the curve point right after the given ap
 */
AutomationCurve *
automation_track_get_next_curve_ac (AutomationTrack * at,
                                    AutomationPoint * ap);

/**
 * Removes automation point from automation track.
 */
void
automation_track_remove_ap (AutomationTrack * at,
                            AutomationPoint * ap);

/**
 * Removes automation curve from automation track.
 */
void
automation_track_remove_ac (AutomationTrack * at,
                            AutomationCurve * ac);

int
automation_track_get_ap_index (AutomationTrack * at,
                            AutomationPoint * ap);

int
automation_track_get_curve_index (AutomationTrack * at,
                                  AutomationCurve * ac);

/**
 * Updates automation track & its GUI
 */
//void
//automation_track_update (AutomationTrack * at);

#endif // __AUDIO_AUTOMATION_TRACK_H__
