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

/**
 * \file
 *
 * Automation tracklist containing automation
 * points and curves.
 */

#ifndef __AUDIO_AUTOMATION_TRACKLIST_H__
#define __AUDIO_AUTOMATION_TRACKLIST_H__

#include "utils/yaml.h"

#define GET_SELECTED_AUTOMATION_TRACKS \
  Track * selected_tracks[200]; \
  int num_selected = 0; \
  tracklist_get_selected_tracks (PROJECT->tracklist,\
                                 selected_tracks,\
                                 &num_selected);

typedef struct AutomationTrack AutomationTrack;
typedef struct _AutomationTracklistWidget
  AutomationTracklistWidget;
typedef struct Track Track;
typedef struct Automatable Automatable;
typedef struct AutomationLane AutomationLane;

/**
 * Each track has an automation tracklist with automation
 * tracks to be generated at runtime, and filled in with
 * automation points/curves when loading projects.
 */
typedef struct AutomationTracklist
{
  /**
   * Automation tracks to be generated/used at run time.
   *
   * These should be updated with ALL of the automatables
   * available in the channel and its plugins every time there
   * is an update.
   *
   * When loading projects, these should be first generated
   * and then updated with automation points/curves.
   */
  int                          at_ids[4000];
  AutomationTrack *            automation_tracks[4000];
  int                          num_automation_tracks;

  /**
   * These are the active automation lanes that are
   * shown in the UI, including hidden ones.
   *
   * Automation tracks become active automation
   * lanes when they have automation or are selected.
   *
   * They must be associated with an automation
   * track.
   */
  int                          al_ids[400];
  AutomationLane *             automation_lanes[400];
  int                          num_automation_lanes;

  /**
   * Pointer back to owner track.
   *
   * For convenience only. Not to be serialized.
   */
  int                          track_id;
  Track *                      track; ///< cache

  AutomationTracklistWidget *  widget;
} AutomationTracklist;

static const cyaml_schema_field_t
  automation_tracklist_fields_schema[] =
{
  CYAML_FIELD_SEQUENCE_COUNT (
    "at_ids", CYAML_FLAG_DEFAULT,
    AutomationTracklist, at_ids,
    num_automation_tracks,
    &int_schema, 0, CYAML_UNLIMITED),
  CYAML_FIELD_SEQUENCE_COUNT (
    "al_ids", CYAML_FLAG_DEFAULT,
    AutomationTracklist, al_ids,
    num_automation_lanes,
    &int_schema, 0, CYAML_UNLIMITED),
	CYAML_FIELD_INT (
    "track_id", CYAML_FLAG_DEFAULT,
    AutomationTracklist, track_id),

	CYAML_FIELD_END
};

static const cyaml_schema_value_t
  automation_tracklist_schema =
{
	CYAML_VALUE_MAPPING (
    CYAML_FLAG_POINTER,
    AutomationTracklist,
    automation_tracklist_fields_schema),
};

void
automation_tracklist_init (
  AutomationTracklist * self,
  Track *               track);

void
automation_tracklist_init_loaded (
  AutomationTracklist * self);

void
automation_tracklist_add_automation_lane (
  AutomationTracklist * self,
  AutomationLane *      al);

/**
 * Finds visible tracks and puts them in given array.
 */
void
automation_tracklist_get_visible_tracks (
  AutomationTracklist * self,
  AutomationTrack **    visible_tracks,
  int *                 num_visible);


/**
 * Updates the automation tracks. (adds missing)
 *
 * Builds an automation track for each automatable in the channel and its plugins,
 * unless it already exists.
 */
void
automation_tracklist_update (
  AutomationTracklist * self);

/**
 * Clones the automation tracklist elements from
 * src to dest.
 */
void
automation_tracklist_clone (
  AutomationTracklist * src,
  AutomationTracklist * dest);

AutomationTrack *
automation_tracklist_get_at_from_automatable (
  AutomationTracklist * self,
  Automatable *         a);

AutomationTrack *
automation_tracklist_get_first_invisible_at (
  AutomationTracklist * self);

void
automation_tracklist_free_members (
  AutomationTracklist * self);

#endif
