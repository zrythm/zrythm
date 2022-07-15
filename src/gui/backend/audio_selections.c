/*
 * Copyright (C) 2020 Alexandros Theodotou <alex at zrythm dot org>
 *
 * This file is part of Zrythm
 *
 * Zrythm is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Zrythm is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "audio/audio_region.h"
#include "audio/chord_track.h"
#include "audio/engine.h"
#include "audio/position.h"
#include "audio/track.h"
#include "audio/transport.h"
#include "gui/backend/audio_selections.h"
#include "gui/backend/event.h"
#include "gui/backend/event_manager.h"
#include "gui/widgets/midi_region.h"
#include "project.h"
#include "utils/arrays.h"
#include "utils/audio.h"
#include "utils/flags.h"
#include "utils/objects.h"
#include "utils/yaml.h"
#include "zrythm_app.h"

#include <gtk/gtk.h>

/**
 * Sets whether a range selection exists and sends
 * events to update the UI.
 */
void
audio_selections_set_has_range (
  AudioSelections * self,
  bool              has_range)
{
  self->has_selection = true;

  EVENTS_PUSH (ET_AUDIO_SELECTIONS_RANGE_CHANGED, NULL);
}

/**
 * Returns if the selections can be pasted.
 *
 * @param pos Position to paste to.
 * @param region ZRegion to paste to.
 */
bool
audio_selections_can_be_pasted (
  AudioSelections * ts,
  Position *        pos,
  ZRegion *         r)
{
  if (!r || r->id.type != REGION_TYPE_AUDIO)
    return false;

  ArrangerObject * r_obj = (ArrangerObject *) r;
  if (r_obj->pos.frames + pos->frames < 0)
    return false;

  /* TODO */
  return false;
}
