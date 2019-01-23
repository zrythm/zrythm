/*
 * actions/timeline_selections.h - timeline
 *
 * Copyright (C) 2019 Alexandros Theodotou
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef __ACTIONS_TIMELINE_SELECTIONS_H__
#define __ACTIONS_TIMELINE_SELECTIONS_H__

typedef struct Region Region;
typedef struct AutomationPoint AutomationPoint;
typedef struct Chord Chord;

/**
 * Selections to be used for the timeline's current
 * selections, copying, undoing, etc.
 */
typedef struct TimelineSelections
{
  Region *                 regions[600];  ///< regions doing action upon (timeline)
  int                      num_regions;
  Region *                 top_region; ///< highest selected region
  Region *                 bot_region; ///< lowest selected region
  AutomationPoint *        automation_points[600];  ///< automation points doing action upon (timeline)
  int                      num_automation_points;
  Chord *                  chords[800];
  int                      num_chords;
} TimelineSelections;

/**
 * Clone the struct for copying, undoing, etc.
 */
//void
//timeline_selections_clone (TimelineSelections * src,
                           //TimelineSelections * dest);

/**
 * Serializes to XML.
 *
 * MUST be free'd.
 */
char *
timeline_selections_serialize (
  TimelineSelections * ts); ///< TS to serialize

#endif
