/*
 * audio/automation_track.h - Automation track
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

#ifndef __AUDIO_AUTOMATION_TRACK_H__
#define __AUDIO_AUTOMATION_TRACK_H__

#include "audio/position.h"


typedef struct Port Port;
typedef struct AutomationTrackWidget AutomationTrackWidget;
typedef struct Track Track;
typedef struct AutomationPoint AutomationPoint;

typedef struct AutomationTrack
{
  Port *                    port; ///< the port it automates
  AutomationTrackWidget *   widget;
  Track                     * track; ///< owner track
  AutomationPoint *         automation_points[200]; /// automation points
  int                       num_automation_points;
} AutomationTrack;

AutomationTrack *
automation_track_new_new ();

#endif // __AUDIO_AUTOMATION_TRACK_H__
