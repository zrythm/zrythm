/*
 * Copyright (C) 2018-2019 Alexandros Theodotou <alex at zrythm dot org>
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
 * along with Zrythm.  If not, see <https://www.gnu.org/licenses/>.
 */

/**
 * \file
 *
 * The automation containing regions and other
 * objects.
 */

#include "actions/arranger_selections.h"
#include "actions/undoable_action.h"
#include "actions/undo_manager.h"
#include "audio/automation_region.h"
#include "audio/automation_track.h"
#include "audio/automation_tracklist.h"
#include "audio/audio_track.h"
#include "audio/audio_bus_track.h"
#include "audio/channel.h"
#include "audio/chord_object.h"
#include "audio/chord_track.h"
#include "audio/instrument_track.h"
#include "audio/marker_track.h"
#include "audio/master_track.h"
#include "audio/midi_region.h"
#include "audio/mixer.h"
#include "audio/scale_object.h"
#include "audio/track.h"
#include "audio/tracklist.h"
#include "audio/transport.h"
#include "gui/backend/automation_selections.h"
#include "gui/widgets/arranger.h"
#include "gui/widgets/automation_arranger.h"
#include "gui/widgets/automation_editor_space.h"
#include "gui/widgets/automation_point.h"
#include "gui/widgets/bot_dock_edge.h"
#include "gui/widgets/center_dock.h"
#include "gui/widgets/chord_object.h"
#include "gui/widgets/clip_editor.h"
#include "gui/widgets/clip_editor_inner.h"
#include "gui/widgets/color_area.h"
#include "gui/widgets/editor_ruler.h"
#include "gui/widgets/inspector.h"
#include "gui/widgets/main_window.h"
#include "gui/widgets/marker.h"
#include "gui/widgets/midi_arranger.h"
#include "gui/widgets/midi_arranger_bg.h"
#include "gui/widgets/midi_region.h"
#include "gui/widgets/pinned_tracklist.h"
#include "gui/widgets/midi_note.h"
#include "gui/widgets/region.h"
#include "gui/widgets/scale_object.h"
#include "gui/widgets/ruler.h"
#include "gui/widgets/automation_arranger.h"
#include "gui/widgets/automation_arranger_bg.h"
#include "gui/widgets/track.h"
#include "gui/widgets/tracklist.h"
#include "project.h"
#include "settings/settings.h"
#include "utils/arrays.h"
#include "utils/cairo.h"
#include "utils/flags.h"
#include "utils/objects.h"
#include "utils/ui.h"
#include "zrythm.h"

#include <gtk/gtk.h>

/**
 * Create an AutomationPointat the given Position
 * in the given Track's AutomationTrack.
 *
 * @param pos The pre-snapped position.
 */
void
automation_arranger_widget_create_ap (
  ArrangerWidget * self,
  const Position *   pos,
  const double       start_y,
  ZRegion *           region)
{
  AutomationTrack * at = CLIP_EDITOR->region->at;
  g_return_if_fail (at);

  self->action =
    UI_OVERLAY_ACTION_CREATING_MOVING;

  ArrangerObject * region_obj =
    (ArrangerObject *) region;

  /* get local pos */
  Position local_pos;
  position_from_ticks (
    &local_pos,
    pos->total_ticks -
    region_obj->pos.total_ticks);

  int height =
    gtk_widget_get_allocated_height (
      GTK_WIDGET (self));
  /* do height - because it's uside down */
	float normalized_val =
		(float) ((height - start_y) / height);
  float value =
    automatable_normalized_val_to_real (
      at->automatable, normalized_val);

  self->action =
    UI_OVERLAY_ACTION_CREATING_MOVING;

  /* create a new ap */
  AutomationPoint * ap =
    automation_point_new_float (
      value, normalized_val, &local_pos, F_MAIN);
  ArrangerObject * ap_obj =
    (ArrangerObject *) ap;

  /* set it as start object */
  self->start_object = ap_obj;

  /* add it to automation track */
  automation_region_add_ap (
    CLIP_EDITOR->region, ap);

  /* set position to all counterparts */
  arranger_object_set_position (
    ap_obj, &local_pos,
    ARRANGER_OBJECT_POSITION_TYPE_START,
    F_NO_CACHED, F_NO_VALIDATE);
  arranger_object_set_position (
    ap_obj, &local_pos,
    ARRANGER_OBJECT_POSITION_TYPE_START,
    F_CACHED, F_NO_VALIDATE);

  EVENTS_PUSH (
    ET_ARRANGER_OBJECT_CREATED, ap);
  arranger_object_select (
    ap_obj, F_SELECT,
    F_NO_APPEND);
}

/**
 * Change curviness of selected curves.
 */
void
automation_arranger_widget_resize_curves (
  ArrangerWidget * self,
  double                     offset_y)
{
  double diff = offset_y - self->last_offset_y;
  diff = - diff;
  diff = diff / 120.0;
  for (int i = 0;
       i < AUTOMATION_SELECTIONS->num_automation_points; i++)
    {
      AutomationPoint * ap =
        AUTOMATION_SELECTIONS->automation_points[i];
      if (ap->curve_up)
        diff = - diff;
      double new_curve_val =
        CLAMP (
          ap->curviness + diff,
          AP_MIN_CURVINESS, 2.0);
      int new_curve_up = ap->curve_up;
      if (new_curve_val >= AP_MAX_CURVINESS)
        {
          new_curve_val -= AP_MAX_CURVINESS;
          new_curve_val =
            AP_MAX_CURVINESS - new_curve_val;
          new_curve_up = !ap->curve_up;
        }
      automation_point_set_curviness (
        ap, new_curve_val, new_curve_up);
    }

  EVENTS_PUSH (
    ET_ARRANGER_SELECTIONS_CHANGED,
    AUTOMATION_SELECTIONS);
}
