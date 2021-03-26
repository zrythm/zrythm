/*
 * Copyright (C) 2020 Alexandros Theodotou <alex at zrythm dot org>
 *
 * This file is part of Zrythm
 *
 * Zrythm is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, version 3.
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
#include "gui/backend/event.h"
#include "gui/backend/event_manager.h"
#include "gui/backend/audio_selections.h"
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
 * Inits the audio selections.
 */
void
audio_selections_init (
  AudioSelections * self)
{
  self->schema_version =
    AUDIO_SELECTIONS_SCHEMA_VERSION;
  position_init (&self->sel_start);
  position_init (&self->sel_end);
  region_identifier_init (&self->region_id);
  self->pool_id = -1;
}

/**
 * Creates a new AudioSelections instance.
 */
AudioSelections *
audio_selections_new (void)
{
  AudioSelections * self =
    object_new (AudioSelections);

  audio_selections_init (self);

  return self;
}

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

  EVENTS_PUSH (
    ET_AUDIO_SELECTIONS_RANGE_CHANGED, NULL);
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
  Position *             pos,
  ZRegion *              r)
{
  if (!r || r->id.type != REGION_TYPE_AUDIO)
    return false;

  ArrangerObject * r_obj =
    (ArrangerObject *) r;
  if (r_obj->pos.frames + pos->frames < 0)
    return false;

  /* TODO */
  return false;
}
