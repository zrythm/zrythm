/*
 * audio/automation_tracklist.h - Tracklist backend
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

/** \file
 */

#ifndef __AUDIO_AUTOMATION_TRACKLIST_H__
#define __AUDIO_AUTOMATION_TRACKLIST_H__

#define GET_SELECTED_AUTOMATION_TRACKS \
  Track * selected_tracks[200]; \
  int num_selected = 0; \
  tracklist_get_selected_tracks (PROJECT->tracklist,\
                                 selected_tracks,\
                                 &num_selected);

typedef struct AutomationTrack AutomationTrack;
typedef struct _AutomationTracklistWidget AutomationTracklistWidget;
typedef struct Track Track;

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
  AutomationTrack *            automation_tracks[4000];
  int                          num_automation_tracks;

  /**
   * Pointer back to owner track.
   *
   * For convenience only. Not to be serialized.
   */
  Track *                      track; ///< cache

  AutomationTracklistWidget *  widget;
} AutomationTracklist;

void
automation_tracklist_init (AutomationTracklist * self,
                           Track *               track);

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
automation_tracklist_refresh (AutomationTracklist * self);

AutomationTrack *
automation_tracklist_fetch_first_invisible_at (
  AutomationTracklist * self);
#endif
