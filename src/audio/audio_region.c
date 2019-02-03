/*
 * audio/audio_region.c - A region in the timeline having a start
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

#include "audio/channel.h"
#include "audio/audio_region.h"
#include "audio/track.h"
#include "gui/widgets/main_window.h"
#include "gui/widgets/audio_region.h"
#include "gui/widgets/region.h"
#include "project.h"

/**
 * Creates region (used when loading projects).
 */
AudioRegion *
audio_region_get_or_create_blank (int id)
{
  if (PROJECT->regions[id])
    {
      return (AudioRegion *) PROJECT->regions[id];
    }
  else
    {
      AudioRegion * audio_region = calloc (1, sizeof (AudioRegion));
      Region * region = (Region *) audio_region;

      region->id = id;
      PROJECT->regions[id] = region;
      PROJECT->num_regions++;

      g_message ("Creating blank audio region %d", id);

      return audio_region;
    }
}

AudioRegion *
audio_region_new (Track *    track,
                  Position * start_pos,
                  Position * end_pos)
{
  AudioRegion * self =
    calloc (1, sizeof (AudioRegion));

  region_init ((Region *) self,
               REGION_TYPE_AUDIO,
               track,
               start_pos,
               end_pos);

  return self;
}

void
audio_region_add_audio_clip (AudioRegion * self,
                             AudioClip *   ac)
{
  self->audio_clip = ac;
  /*audio_region_widget_refresh (self->widget);*/
}

