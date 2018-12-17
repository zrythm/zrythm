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

#define GET_SELECTED_TRACKS \
  Track * selected_tracks[200]; \
  int num_selected = 0; \
  tracklist_get_selected_tracks (PROJECT->tracklist,\
                                 selected_tracks,\
                                 &num_selected);

typedef struct AutomationTrack AutomationTrack;
typedef struct _AutomationTracklistWidget AutomationTracklistWidget;
typedef struct Track Track;

/**
 * Internal Tracklist.
 */
typedef struct AutomationTracklist
{
  /**
   * These should be updated with ALL of the automatables
   * available in the channel and its plugins every time there
   * is an update.
   */
  AutomationTrack *            automation_tracks[4000];
  int                          num_automation_tracks;

  /**
   * Pointer back to owner track.
   */
  Track *                      track;

  AutomationTracklistWidget *  widget;
} AutomationTracklist;

AutomationTracklist *
automation_tracklist_new (Track * track);

void
automation_tracklist_setup (AutomationTracklist * self);

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
automation_tracklist_update (AutomationTracklist * self);

AutomationTrack *
automation_tracklist_fetch_first_invisible_at (
  AutomationTracklist * self);
#endif
