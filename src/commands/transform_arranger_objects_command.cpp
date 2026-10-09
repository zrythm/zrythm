// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <cassert>

#include "commands/transform_arranger_objects_command.h"
#include "structure/arrangement/arranger_object_all.h"
#include "utils/variant_helpers.h"

namespace zrythm::commands
{

namespace
{

TransformableObjectState
capture_state (const structure::arrangement::ArrangerObjectUuidReference &object)
{
  TransformableObjectState state;
  std::visit (
    [&] (auto &&obj) {
      using ObjectT = utils::base_type<decltype (obj)>;
      if constexpr (std::is_same_v<ObjectT, structure::arrangement::MidiNote>)
        {
          state.position_ticks_ = obj->position ()->asTick ();
          state.length_ticks_ = obj->length ()->asTick ();
          state.pitch_ = obj->pitch ();
          state.velocity_ = obj->velocity ();
        }
      else if constexpr (
        std::is_same_v<ObjectT, structure::arrangement::AutomationPoint>)
        {
          state.position_ticks_ = obj->position ()->asTick ();
          state.automation_value_ = obj->value ();
          state.curve_algorithm_ = obj->curveOpts ()->algorithm ();
          state.curve_curviness_ = obj->curveOpts ()->curviness ();
        }
    },
    utils::convert_to_variant_qobj<
      structure::arrangement::ArrangerObjectPtrVariant> (object.get ()));
  return state;
}

void
write_state (
  const structure::arrangement::ArrangerObjectUuidReference &object,
  const TransformableObjectState                            &state)
{
  std::visit (
    [&] (auto &&obj) {
      using ObjectT = utils::base_type<decltype (obj)>;
      if constexpr (std::is_same_v<ObjectT, structure::arrangement::MidiNote>)
        {
          obj->position ()->set_ticks (state.position_ticks_);
          obj->length ()->set_ticks (state.length_ticks_);
          obj->setPitch (state.pitch_);
          obj->setVelocity (state.velocity_);
        }
      else if constexpr (
        std::is_same_v<ObjectT, structure::arrangement::AutomationPoint>)
        {
          obj->position ()->set_ticks (state.position_ticks_);
          obj->setValue (state.automation_value_);
          obj->curveOpts ()->setAlgorithm (state.curve_algorithm_);
          obj->curveOpts ()->setCurviness (state.curve_curviness_);
        }
    },
    utils::convert_to_variant_qobj<
      structure::arrangement::ArrangerObjectPtrVariant> (object.get ()));
}

} // namespace

TransformArrangerObjectsCommand::TransformArrangerObjectsCommand (
  std::vector<structure::arrangement::ArrangerObjectUuidReference> objects,
  std::vector<TransformableObjectState>                            after_states,
  const QString                                                   &text)
    : QUndoCommand (text), objects_ (std::move (objects)),
      after_states_ (std::move (after_states))
{
  assert (objects_.size () == after_states_.size ());
  before_states_.reserve (objects_.size ());
  for (const auto &object : objects_)
    {
      before_states_.push_back (capture_state (object));
    }
}

void
TransformArrangerObjectsCommand::undo ()
{
  for (const auto &[object, state] : std::views::zip (objects_, before_states_))
    {
      write_state (object, state);
    }
}

void
TransformArrangerObjectsCommand::redo ()
{
  for (const auto &[object, state] : std::views::zip (objects_, after_states_))
    {
      write_state (object, state);
    }
}

} // namespace zrythm::commands
