/*
 * audio/automation_track.c - Automation track
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

#include "audio/automation_track.h"
#include "gui/widgets/automation_track.h"

AutomationTrack *
automation_track_new (Track *         track,
                      Automatable *   automatable)
{
  AutomationTrack * at = calloc (1, sizeof (AutomationTrack));

  at->track = track;
  at->automatable = automatable;
  at->widget = automation_track_widget_new (at);

  return at;
}

