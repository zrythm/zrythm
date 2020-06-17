/*
 * Copyright (C) 2019 Alexandros Theodotou <alex at zrythm dot org>
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

#include "audio/automation_region.h"
#include "audio/chord_track.h"
#include "audio/engine.h"
#include "audio/position.h"
#include "audio/track.h"
#include "audio/transport.h"
#include "gui/backend/event_manager.h"
#include "gui/backend/automation_selections.h"
#include "gui/widgets/midi_region.h"
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
int
automation_selections_can_be_pasted (
  AutomationSelections * ts,
  Position *             pos,
  ZRegion *              r)
{
  if (!r || r->id.type != REGION_TYPE_AUTOMATION)
    return 0;

  ArrangerObject * r_obj =
    (ArrangerObject *) r;
  if (r_obj->pos.frames + pos->frames < 0)
    return 0;

  return 1;
}

void
automation_selections_paste_to_pos (
  AutomationSelections * ts,
  Position *           playhead)
{
  ZRegion * region =
    clip_editor_get_region (CLIP_EDITOR);
  ArrangerObject * r_obj =
    (ArrangerObject *) region;
  g_return_if_fail (
    region &&
    region->id.type == REGION_TYPE_AUTOMATION);

  /* get region-local pos */
  Position pos;
  pos.frames =
    region_timeline_frames_to_local (
      region, playhead->frames, 0);
  position_from_frames (&pos, pos.frames);
  double pos_ticks =
    position_to_ticks (&pos) +
    r_obj->pos.total_ticks;

  /* get pos of earliest object */
  Position start_pos;
  arranger_selections_get_start_pos (
    (ArrangerSelections *) ts, &start_pos, false);
  position_print (&start_pos);
  double start_pos_ticks =
    position_to_ticks (&start_pos);

  /* FIXME doesn't work when you paste at a negative
   * position in the region */

  for (int i = 0; i < ts->num_automation_points; i++)
    {
      AutomationPoint * ap =
        ts->automation_points[i];
      ArrangerObject * ap_obj =
        (ArrangerObject *) ap;

      /* update positions */
      double curr_ticks =
        position_to_ticks (&ap_obj->pos);
      double diff = curr_ticks - start_pos_ticks;
      position_from_ticks (
        &ap_obj->pos, pos_ticks + diff);

      /* clone and add to track */
      AutomationPoint * ap_clone =
        (AutomationPoint *)
        arranger_object_clone (
          (ArrangerObject *) ap,
          ARRANGER_OBJECT_CLONE_COPY_MAIN);
      ArrangerObject * cp_obj =
        (ArrangerObject *) ap_clone;
      region =
        region_find (&cp_obj->region_id);
      automation_region_add_ap (
        region, ap_clone, F_PUBLISH_EVENTS);
    }
#undef DIFF
}

SERIALIZE_SRC (
  AutomationSelections, automation_selections)
DESERIALIZE_SRC (
  AutomationSelections, automation_selections)
PRINT_YAML_SRC (
  AutomationSelections, automation_selections)
