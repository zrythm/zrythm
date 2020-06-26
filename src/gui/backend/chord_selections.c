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

#include "audio/chord_region.h"
#include "audio/chord_track.h"
#include "audio/engine.h"
#include "audio/position.h"
#include "audio/track.h"
#include "audio/transport.h"
#include "gui/backend/event_manager.h"
#include "gui/backend/chord_selections.h"
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
chord_selections_can_be_pasted (
  ChordSelections * ts,
  Position *        pos,
  ZRegion *          r)
{
  if (!r || r->id.type != REGION_TYPE_CHORD)
    return 0;

  ArrangerObject * r_obj =
    (ArrangerObject *) r;
  if (r_obj->pos.frames + pos->frames < 0)
    return 0;

  return 1;
}

void
chord_selections_paste_to_pos (
  ChordSelections * ts,
  Position *        playhead)
{
  ZRegion * region =
    clip_editor_get_region (CLIP_EDITOR);
  g_return_if_fail (
    region && region->id.type == REGION_TYPE_CHORD);

  /* get region-local pos */
  Position pos;
  pos.frames =
    region_timeline_frames_to_local (
      region, playhead->frames, 0);
  position_from_frames (&pos, pos.frames);
  double pos_ticks = position_to_ticks (&pos);

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
  ChordObject * chord;
  for (i = 0; i < ts->num_chord_objects; i++)
    {
      chord = ts->chord_objects[i];
      ArrangerObject * chord_obj =
        (ArrangerObject *) chord;
      region_identifier_copy (
        &chord_obj->region_id,
        &CLIP_EDITOR->region_id);

      curr_ticks =
        position_to_ticks (&chord_obj->pos);
      position_from_ticks (
        &chord_obj->pos, pos_ticks + DIFF);

      /* clone and add to track */
      ChordObject * cp =
        (ChordObject *)
        arranger_object_clone (
          chord_obj,
          ARRANGER_OBJECT_CLONE_COPY_MAIN);
      region =
        chord_object_get_region (cp);
      chord_region_add_chord_object (
        region, cp, F_PUBLISH_EVENTS);
    }
#undef DIFF
}

SERIALIZE_SRC (
  ChordSelections, chord_selections)
DESERIALIZE_SRC (
  ChordSelections, chord_selections)
PRINT_YAML_SRC (
  ChordSelections, chord_selections)
