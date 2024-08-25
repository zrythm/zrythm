// SPDX-FileCopyrightText: © 2018-2020, 2022, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <cmath>

#include "actions/arranger_selections.h"
#include "actions/undo_manager.h"
#include "dsp/audio_track.h"
#include "dsp/chord_object.h"
#include "dsp/chord_region.h"
#include "dsp/midi_region.h"
#include "gui/backend/event.h"
#include "gui/backend/event_manager.h"
#include "gui/widgets/arranger.h"
#include "gui/widgets/bot_dock_edge.h"
#include "gui/widgets/center_dock.h"
#include "gui/widgets/chord_arranger.h"
#include "gui/widgets/chord_editor_space.h"
#include "gui/widgets/clip_editor.h"
#include "gui/widgets/clip_editor_inner.h"
#include "gui/widgets/main_window.h"
#include "gui/widgets/ruler.h"
#include "gui/widgets/track.h"
#include "project.h"
#include "utils/ui.h"
#include "zrythm.h"
#include "zrythm_app.h"

#include "gtk_wrapper.h"

void
chord_arranger_widget_create_chord (
  ArrangerWidget * self,
  const Position   pos,
  int              chord_index,
  ChordRegion     &region)
{
  z_return_if_fail (
    chord_index < static_cast<int> (CHORD_EDITOR->chords_.size ()));
  self->action = UiOverlayAction::CREATING_MOVING;

  /* get local pos */
  Position local_pos{ pos.ticks_ - region.pos_.ticks_ };

  /* create a new chord and add it to the region */
  auto chord = region.append_object (
    std::make_shared<ChordObject> (
      region.id_, chord_index, region.chord_objects_.size ()),
    true);

  chord->set_position (&local_pos, ArrangerObject::PositionType::Start, false);

  /* set as start object */
  arranger_widget_set_start_object (self, chord);

  chord->select (true, false, false);
}

int
chord_arranger_widget_get_chord_at_y (double y)
{
  double adj_y = y - 1;
  double adj_px_per_key =
    chord_editor_space_widget_get_chord_height (MW_CHORD_EDITOR_SPACE) + 1;
  return (int) std::floor (adj_y / adj_px_per_key);
}

int
chord_arranger_calc_deltamax_for_chord_movement (int y_delta)
{
  for (auto chord : CHORD_SELECTIONS->objects_ | type_is<ChordObject> ())
    {
      if (chord->chord_index_ + y_delta < 0)
        {
          y_delta = 0;
        }
      else if (
        chord->chord_index_ + y_delta
        >= (static_cast<int> (CHORD_EDITOR->chords_.size ()) - 1))
        {
          y_delta =
            (static_cast<int> (CHORD_EDITOR->chords_.size ()) - 1)
            - chord->chord_index_;
        }
    }
  return y_delta;
}

void
chord_arranger_on_drag_end (ArrangerWidget * self)
{
  auto start_chord_in_prj =
    dynamic_pointer_cast<ChordObject> (self->prj_start_object.lock ());
  auto transient_start_chord_in_prj =
    start_chord_in_prj
      ? start_chord_in_prj->get_transient<ChordObject> ()
      : nullptr;
  // const auto chord_at_start =
  // dynamic_cast<ChordObject *> (self->start_object.get ());

  try
    {
      switch (self->action)
        {
        case UiOverlayAction::STARTING_MOVING:
          {
            /* if something was clicked with ctrl without moving*/
            if (self->ctrl_held)
              {
                if (start_chord_in_prj && self->start_object_was_selected)
                  {
                    /* deselect it */
                    start_chord_in_prj->select (false, true, false);
                  }
              }
          }
          break;
        case UiOverlayAction::MOVING:
          {
            double ticks_diff =
              start_chord_in_prj->pos_.ticks_
              - transient_start_chord_in_prj->pos_.ticks_;
            int chord_diff =
              start_chord_in_prj->chord_index_
              - transient_start_chord_in_prj->chord_index_;
            UNDO_MANAGER->perform (
              std::make_unique<ArrangerSelectionsAction::MoveChordAction> (
                *CHORD_SELECTIONS, ticks_diff, chord_diff, true));
          }
          break;
        case UiOverlayAction::MOVING_COPY:
        case UiOverlayAction::MOVING_LINK:
          {
            double ticks_diff =
              start_chord_in_prj->pos_.ticks_
              - transient_start_chord_in_prj->pos_.ticks_;
            int chord_diff =
              start_chord_in_prj->chord_index_
              - transient_start_chord_in_prj->chord_index_;
            UNDO_MANAGER->perform (
              std::make_unique<
                ArrangerSelectionsAction::MoveOrDuplicateChordAction> (
                *CHORD_SELECTIONS, false, ticks_diff, chord_diff, true));
          }
          break;
        case UiOverlayAction::NONE:
        case UiOverlayAction::STARTING_SELECTION:
          {
            CHORD_SELECTIONS->clear (false);
          }
          break;
        case UiOverlayAction::CREATING_MOVING:
          {
            UNDO_MANAGER->perform (
              std::make_unique<CreateArrangerSelectionsAction> (
                *CHORD_SELECTIONS));
          }
          break;
        case UiOverlayAction::DELETE_SELECTING:
        case UiOverlayAction::ERASING:
          arranger_widget_handle_erase_action (self);
          break;
        /* if didn't click on something */
        default:
          {
          }
          break;
        }
    }
  catch (const ZrythmException &e)
    {
      e.handle (_ ("Failed to perform action"));
    }
}