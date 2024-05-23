// clang-format off
// SPDX-FileCopyrightText: © 2018-2020, 2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense
// clang-format on

#include <math.h>

#include "actions/arranger_selections.h"
#include "actions/undo_manager.h"
#include "actions/undoable_action.h"
#include "dsp/audio_bus_track.h"
#include "dsp/audio_track.h"
#include "dsp/automation_track.h"
#include "dsp/automation_tracklist.h"
#include "dsp/channel.h"
#include "dsp/chord_object.h"
#include "dsp/chord_region.h"
#include "dsp/chord_track.h"
#include "dsp/instrument_track.h"
#include "dsp/marker_track.h"
#include "dsp/master_track.h"
#include "dsp/midi_region.h"
#include "dsp/scale_object.h"
#include "dsp/track.h"
#include "dsp/tracklist.h"
#include "dsp/transport.h"
#include "gui/backend/event.h"
#include "gui/backend/event_manager.h"
#include "gui/widgets/arranger.h"
#include "gui/widgets/automation_point.h"
#include "gui/widgets/bot_dock_edge.h"
#include "gui/widgets/center_dock.h"
#include "gui/widgets/chord_arranger.h"
#include "gui/widgets/chord_editor_space.h"
#include "gui/widgets/chord_object.h"
#include "gui/widgets/clip_editor.h"
#include "gui/widgets/clip_editor_inner.h"
#include "gui/widgets/color_area.h"
#include "gui/widgets/editor_ruler.h"
#include "gui/widgets/main_window.h"
#include "gui/widgets/marker.h"
#include "gui/widgets/midi_arranger.h"
#include "gui/widgets/midi_note.h"
#include "gui/widgets/region.h"
#include "gui/widgets/ruler.h"
#include "gui/widgets/scale_object.h"
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
#include "zrythm_app.h"

#include <gtk/gtk.h>

/**
 * Create a ChordObject at the given Position in the
 * given Track.
 *
 * @param pos The pre-snapped position.
 */
ChordObject *
chord_arranger_widget_create_chord (
  ArrangerWidget * self,
  const Position * pos,
  int              chord_index,
  ZRegion *        region)
{
  g_return_val_if_fail (chord_index < CHORD_EDITOR->num_chords, NULL);
  self->action = UI_OVERLAY_ACTION_CREATING_MOVING;

  ArrangerObject * region_obj = (ArrangerObject *) region;

  /* get local pos */
  Position local_pos;
  position_from_ticks (&local_pos, pos->ticks - region_obj->pos.ticks);

  /* create a new chord */
  ChordObject * chord =
    chord_object_new (&region->id, chord_index, region->num_chord_objects);
  ArrangerObject * chord_obj = (ArrangerObject *) chord;

  /* add it to chord region */
  chord_region_add_chord_object (region, chord, F_PUBLISH_EVENTS);

  arranger_object_set_position (
    chord_obj, &local_pos,
    ArrangerObjectPositionType::ARRANGER_OBJECT_POSITION_TYPE_START,
    F_NO_VALIDATE);

  /* set as start object */
  self->start_object = chord_obj;

  arranger_object_select (chord_obj, F_SELECT, F_NO_APPEND, F_NO_PUBLISH_EVENTS);

  return chord;
}

/**
 * Returns the chord index at y.
 */
int
chord_arranger_widget_get_chord_at_y (double y)
{
  double adj_y = y - 1;
  double adj_px_per_key =
    chord_editor_space_widget_get_chord_height (MW_CHORD_EDITOR_SPACE) + 1;
  return (int) floor (adj_y / adj_px_per_key);
}

/**
 * Called on move items_y setup.
 *
 * calculates the max possible y movement
 */
int
chord_arranger_calc_deltamax_for_chord_movement (int y_delta)
{
  for (int i = 0; i < CHORD_SELECTIONS->num_chord_objects; i++)
    {
      ChordObject * co = CHORD_SELECTIONS->chord_objects[i];
      if (co->chord_index + y_delta < 0)
        {
          y_delta = 0;
        }
      else if (co->chord_index + y_delta >= (CHORD_EDITOR->num_chords - 1))
        {
          y_delta = (CHORD_EDITOR->num_chords - 1) - co->chord_index;
        }
    }
  return y_delta;
}
