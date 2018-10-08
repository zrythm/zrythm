/*
 * audio/track.c - the back end for a timeline track
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

#include <stdlib.h>

#include "audio/position.h"
#include "audio/region.h"
#include "audio/track.h"
#include "gui/widgets/track.h"

Track *
track_new (Channel * channel)
{
  Track * track = calloc (1, sizeof (Track));

  track->channel = channel;
  track->widget = track_widget_new (track);
  Position start = { 1, 1, 1, 0, 0 };
  Position end = { 1, 3, 1, 9, 123456 };
  track->regions[track->num_regions++] = region_new (track,
                                  &start,
                                  &end);
  start.bars = 2;
  end.bars = 6;
  track->regions[track->num_regions++] = region_new (track,
                                  &start,
                                  &end);

  return track;
}
