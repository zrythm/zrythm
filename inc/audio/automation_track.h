/*
 * Copyright (C) 2018-2019 Alexandros Theodotou <alex at zrythm dot org>
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

#include "audio/automatable.h"
#include "audio/automation_curve.h"
#include "audio/automation_point.h"
#include "audio/position.h"

#define MAX_AUTOMATION_POINTS 1200

enum GenerateCurvePoints
{
  NO_GENERATE_CURVE_POINTS,
  GENERATE_CURVE_POINTS
};

typedef struct Port Port;
typedef struct _AutomationTrackWidget
  AutomationTrackWidget;
typedef struct Track Track;
typedef struct Automatable Automatable;
typedef struct AutomationLane AutomationLane;

typedef struct AutomationTrack
{
  /** Index in parent AutomationTracklist. */
  int                       index;

  /**
   * The automatable this automation track is for.
   */
  Automatable *             automatable; ///< cache

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
  AutomationPoint *   aps[MAX_AUTOMATION_POINTS];
  int                 num_aps;
  AutomationCurve *   acs[MAX_AUTOMATION_POINTS];
  int                 num_acs;

  /**
   * Associated lane.
   */
  AutomationLane *          al;
  int                       al_index;
} AutomationTrack;

static const cyaml_schema_field_t
  automation_track_fields_schema[] =
{
	CYAML_FIELD_INT (
    "index", CYAML_FLAG_DEFAULT,
    AutomationTrack, index),
  CYAML_FIELD_MAPPING_PTR (
    "automatable",
    CYAML_FLAG_DEFAULT | CYAML_FLAG_OPTIONAL,
    AutomationTrack, automatable,
    automatable_fields_schema),
  CYAML_FIELD_SEQUENCE_COUNT (
    "aps", CYAML_FLAG_DEFAULT,
    AutomationTrack, aps, num_aps,
    &automation_point_schema, 0, CYAML_UNLIMITED),
  CYAML_FIELD_SEQUENCE_COUNT (
    "acs", CYAML_FLAG_DEFAULT,
    AutomationTrack, acs, num_acs,
    &automation_curve_schema, 0, CYAML_UNLIMITED),
	CYAML_FIELD_INT (
    "al_index", CYAML_FLAG_DEFAULT,
    AutomationTrack, al_index),

	CYAML_FIELD_END
};

static const cyaml_schema_value_t
  automation_track_schema =
{
	CYAML_VALUE_MAPPING (
    CYAML_FLAG_POINTER,
    AutomationTrack,
    automation_track_fields_schema),
};

void
automation_track_init_loaded (
  AutomationTrack * self);

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
automation_track_add_ap (
  AutomationTrack * at,
  AutomationPoint * ap,
  int               generate_curve_points);

/**
 * Adds automation curve.
 */
void
automation_track_add_ac (
  AutomationTrack * at,
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

//int
//automation_track_get_ap_index (AutomationTrack * at,
                            //AutomationPoint * ap);

//int
//automation_track_get_curve_index (AutomationTrack * at,
                                  //AutomationCurve * ac);

/**
 * Returns the automation curve at the given pos.
 */
AutomationCurve *
automation_track_get_ac_at_pos (
  AutomationTrack * self,
  Position *        pos);

/**
 * Returns the automation point before the pos.
 */
AutomationPoint *
automation_track_get_ap_before_pos (
  AutomationTrack * self,
  Position *        pos);

/**
 * Returns the actual parameter value at the given
 * position.
 *
 * If there is no automation point/curve during
 * the position, it returns the current value
 * of the parameter it is automating.
 */
float
automation_track_get_normalized_val_at_pos (
  AutomationTrack * at,
  Position *        pos);

/**
 * Updates automation track & its GUI
 */
//void
//automation_track_update (AutomationTrack * at);

#endif // __AUDIO_AUTOMATION_TRACK_H__
