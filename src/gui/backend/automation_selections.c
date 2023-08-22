// SPDX-FileCopyrightText: © 2019 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/automation_region.h"
#include "dsp/chord_track.h"
#include "dsp/engine.h"
#include "dsp/position.h"
#include "dsp/track.h"
#include "dsp/transport.h"
#include "gui/backend/automation_selections.h"
#include "gui/backend/event_manager.h"
#include "project.h"
#include "utils/arrays.h"
#include "utils/audio.h"
#include "utils/flags.h"
#include "utils/objects.h"
#include "utils/yaml.h"

#include <gtk/gtk.h>

/**
 * Returns if the selections can be pasted.
 *
 * @param pos Position to paste to.
 * @param region ZRegion to paste to.
 */
bool
automation_selections_can_be_pasted (
  AutomationSelections * ts,
  Position *             pos,
  ZRegion *              r)
{
  if (!r || r->id.type != REGION_TYPE_AUTOMATION)
    return false;

  ArrangerObject * r_obj = (ArrangerObject *) r;
  if (r_obj->pos.frames + pos->frames < 0)
    return false;

  return true;
}
