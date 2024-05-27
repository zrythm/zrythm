// SPDX-FileCopyrightText: © 2020 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/audio_region.h"
#include "dsp/chord_track.h"
#include "dsp/engine.h"
#include "dsp/position.h"
#include "dsp/track.h"
#include "dsp/transport.h"
#include "gui/backend/audio_selections.h"
#include "gui/backend/event.h"
#include "gui/backend/event_manager.h"
#include "project.h"
#include "utils/arrays.h"
#include "utils/audio.h"
#include "utils/flags.h"
#include "utils/objects.h"
#include "utils/yaml.h"
#include "zrythm_app.h"

#include "gtk_wrapper.h"

/**
 * Sets whether a range selection exists and sends
 * events to update the UI.
 */
void
audio_selections_set_has_range (AudioSelections * self, bool has_range)
{
  self->has_selection = true;

  EVENTS_PUSH (EventType::ET_AUDIO_SELECTIONS_RANGE_CHANGED, NULL);
}

/**
 * Returns if the selections can be pasted.
 *
 * @param pos Position to paste to.
 * @param region Region to paste to.
 */
bool
audio_selections_can_be_pasted (AudioSelections * ts, Position * pos, Region * r)
{
  if (!r || r->id.type != RegionType::REGION_TYPE_AUDIO)
    return false;

  ArrangerObject * r_obj = (ArrangerObject *) r;
  if (r_obj->pos.frames + pos->frames < 0)
    return false;

  /* TODO */
  return false;
}
