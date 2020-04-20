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

#include "audio/chord_track.h"
#include "audio/engine.h"
#include "audio/position.h"
#include "audio/track.h"
#include "audio/transport.h"
#include "gui/backend/events.h"
#include "gui/backend/automation_selections.h"
#include "gui/widgets/midi_region.h"
#include "project.h"
#include "utils/arrays.h"
#include "utils/audio.h"
#include "utils/flags.h"
#include "utils/objects.h"
#include "utils/yaml.h"

#include <gtk/gtk.h>

void
automation_selections_paste_to_pos (
  AutomationSelections * ts,
  Position *           pos)
{
  double pos_ticks = position_to_ticks (pos);

  /* get pos of earliest object */
  Position start_pos;
  arranger_selections_get_start_pos (
    (ArrangerSelections *) ts, &start_pos, 0);
  double start_pos_ticks =
    position_to_ticks (&start_pos);

  /* subtract the start pos from every object and
   * add the given pos */
#define DIFF (curr_ticks - start_pos_ticks)
#define ADJUST_POSITION(x) \
  curr_ticks = position_to_ticks (x); \
  position_from_ticks (x, pos_ticks + DIFF)

  double curr_ticks;
  int i;
  AutomationPoint * ap;
  ArrangerObject * ap_obj;
  for (i = 0; i < ts->num_automation_points; i++)
    {
      ap =
        ts->automation_points[i];
      ap_obj = (ArrangerObject *) ap;

      curr_ticks = position_to_ticks (&ap_obj->pos);
      position_from_ticks (
        &ap_obj->pos, pos_ticks + DIFF);
    }
#undef DIFF
}

SERIALIZE_SRC (
  AutomationSelections, automation_selections)
DESERIALIZE_SRC (
  AutomationSelections, automation_selections)
PRINT_YAML_SRC (
  AutomationSelections, automation_selections)
