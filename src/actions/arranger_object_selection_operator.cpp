// SPDX-FileCopyrightText: © 2025-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "actions/arranger_object_selection_operator.h"
#include "commands/add_arranger_object_command.h"
#include "commands/change_timebase_override_command.h"
#include "commands/change_uuid_identifiable_object_property_command.h"
#include "commands/move_arranger_objects_command.h"
#include "commands/remove_arranger_object_command.h"
#include "commands/resize_arranger_objects_command.h"
#include "dsp/timebase.h"
#include "dsp/timestretch_engine.h"
#include "structure/arrangement/arranger_object_all.h"
#include "structure/arrangement/audio_clip.h"
#include "structure/tracks/track_all.h"
#include "structure/tracks/tracklist.h"
#include "utils/logger.h"
#include "utils/math_utils.h"
#include "utils/ranges.h"
#include "utils/variant_helpers.h"

namespace zrythm::actions
{

/**
 * @brief Configures a freshly cloned object as the right half of a cut.
 *
 * The clone is made to start at the content position the original plays at
 * @p cut_pos and end at the original's timeline end, such that playback
 * across the cut is identical to the uncut original.
 */
template <structure::arrangement::BoundedObject ObjectT>
static void
configure_cut_right_half (
  const ObjectT    &orig,
  ObjectT          &right,
  dsp::TimelineTick cut_pos,
  dsp::TimelineTick orig_timeline_end)
{
  if constexpr (structure::arrangement::ClipObject<ObjectT>)
    {
      right.position ()->setTicks (cut_pos.asDouble ());
      // Length: end at the original's timeline end. Must be set after the
      // position (the warp mapping is relative to it) and before the loop
      // range (bounds-tracking may reset the loop positions when the length
      // changes).
      structure::arrangement::set_end_from_timeline_ticks (
        right, orig_timeline_end);

      // Clip start: the content position the original plays at the cut,
      // accounting for the clip start and looping (same convention as
      // playback).
      right.set_loop_range (
        orig.content_position_at_timeline (cut_pos),
        orig.loopStartPosition ()->asTick (),
        orig.loopEndPosition ()->asTick ());
    }
  else
    {
      // Non-clip bounded objects (e.g. MIDI notes) live in their parent
      // clip's content coordinates and are edited in the clip editor's
      // unwound content space: start at the unwound content position under
      // the cut, keeping the original's other properties (from the clone)
      // and the remaining length.
      const auto * parent_clip = qobject_cast<
        const structure::arrangement::Clip *> (orig.parentObject ());
      if (parent_clip != nullptr)
        {
          const auto content_at_cut =
            parent_clip->contentWarp ()->timelineToContent (cut_pos);
          const auto orig_content_end =
            orig.position ()->asTick () + orig.length ()->asTick ();
          right.position ()->setTicks (content_at_cut.asDouble ());
          right.length ()->setTicks (
            (orig_content_end - content_at_cut).asDouble ());
        }
    }

  if constexpr (structure::arrangement::FadeableObject<ObjectT>)
    {
      // Shift the fade offsets so the fades continue seamlessly across the
      // cut. Fade offsets are not warped (see ArrangerObjectFadeRange), so
      // the timeline-domain cut offset applies directly.
      const auto o_cut = cut_pos - orig.position ()->asTick ();
      right.fadeRange ()->startOffset ()->setTicks (
        std::max (
          dsp::TimelineTick{
            units::ticks (orig.fadeRange ()->startOffset ()->ticks ()) }
            - o_cut,
          dsp::TimelineTick{})
          .asDouble ());
      right.fadeRange ()->endOffset ()->setTicks (
        std::max (
          dsp::TimelineTick{
            units::ticks (orig.fadeRange ()->endOffset ()->ticks ()) }
            - o_cut,
          dsp::TimelineTick{})
          .asDouble ());
    }
}

/**
 * @brief Returns the content position of the first chord strictly after
 * @p chord in its parent clip, or std::nullopt if there is none.
 */
static std::optional<dsp::ContentTick>
next_chord_content_position (const structure::arrangement::ChordObject &chord)
{
  const auto * clip = qobject_cast<const structure::arrangement::ChordClip *> (
    chord.parentObject ());
  if (clip == nullptr)
    return std::nullopt;
  const auto                      pos = chord.position ()->asTick ();
  std::optional<dsp::ContentTick> next;
  for (const auto * other : clip->get_sorted_children_view ())
    {
      const auto other_pos = other->position ()->asTick ();
      if (other_pos > pos)
        {
          // The view is sorted by position: the first child after @p chord
          // is the next one
          next = other_pos;
          break;
        }
    }
  return next;
}

ArrangerObjectSelectionOperator ::ArrangerObjectSelectionOperator (
  undo::UndoStack                               &undoStack,
  QItemSelectionModel                           &selectionModel,
  ObjectOwnerProvider                            objectOwnerProvider,
  structure::arrangement::ArrangerObjectFactory &objectFactory,
  TimelineObjectsEnumerator                      timelineObjectsEnumerator,
  QObject *                                      parent)
    : QObject (parent), undo_stack_ (undoStack),
      selection_model_ (selectionModel),
      object_owner_provider_ (std::move (objectOwnerProvider)),
      object_factory_ (objectFactory),
      timeline_objects_enumerator_ (std::move (timelineObjectsEnumerator))
{
}

bool
ArrangerObjectSelectionOperator::moveByTicks (double tick_delta)
{
  if (tick_delta == 0.0)
    {
      // No movement needed
      return true;
    }

  // Extract selected objects from selection model
  auto selected_objects = extractSelectedObjects ();
  if (selected_objects.empty ())
    {
      z_warning ("No objects selected for movement");
      return false;
    }

  // Validate object bounds (don't move objects before timeline start)
  if (!validateHorizontalMovement (selected_objects, tick_delta))
    {
      z_warning ("Horizontal movement validation failed");
      return false;
    }

  // Create and push command
  auto command = [&selected_objects, tick_delta] ()
    -> std::unique_ptr<commands::MoveArrangerObjectsCommand> {
    if (std::ranges::any_of (selected_objects, [] (auto &&object_ref) {
          return object_ref.template get_object_as<
                   structure::arrangement::TempoObject> ()
                   != nullptr
                 || object_ref.template get_object_as<
                      structure::arrangement::TimeSignatureObject> ()
                      != nullptr;
        }))
      {
        return std::make_unique<
          commands::MoveTempoMapAffectingArrangerObjectsCommand> (
          std::move (selected_objects), units::ticks (tick_delta));
      }

    return std::make_unique<commands::MoveArrangerObjectsCommand> (
      std::move (selected_objects), units::ticks (tick_delta));
  }();
  undo_stack_.push (command.release ());

  return true;
}

bool
ArrangerObjectSelectionOperator::moveNotesByPitch (int pitch_delta)
{
  return process_vertical_move (static_cast<double> (pitch_delta));
}

bool
ArrangerObjectSelectionOperator::changeVelocities (int velocity_delta)
{
  if (velocity_delta == 0)
    {
      return true;
    }

  // Extract selected objects from selection model
  auto selected_objects = extractSelectedObjects ();
  if (selected_objects.empty ())
    {
      z_debug ("No objects selected for velocity change");
      return false;
    }

  // Per-note clamping is applied in MoveArrangerObjectsCommand::redo, so push
  // the raw delta. Skip only when no selected note would actually change, to
  // avoid pushing no-op undo entries (e.g. all notes already pinned at a bound
  // and the delta pushes further into it).
  const bool any_change = std::ranges::any_of (
    selected_objects, [velocity_delta] (const auto &obj_ref) {
      auto * note =
        obj_ref.template get_object_as<structure::arrangement::MidiNote> ();
      return note
             && std::clamp (note->velocity () + velocity_delta, 0, 127)
                  != note->velocity ();
    });
  if (!any_change)
    {
      return true;
    }

  // Create and push command
  auto * command = new commands::MoveArrangerObjectsCommand (
    std::move (selected_objects), units::ticks (0),
    static_cast<double> (velocity_delta),
    commands::MoveArrangerObjectsCommand::VerticalChangeType::Velocity);
  undo_stack_.push (command);

  return true;
}

bool
ArrangerObjectSelectionOperator::rampVelocities (
  structure::arrangement::MidiClip * clip,
  double                             start_ticks,
  double                             start_value,
  double                             end_ticks,
  double                             end_value)
{
  // Collect the target notes: the selected MIDI notes, or, when none are
  // selected, all notes of the given clip inside the line's tick span
  std::vector<structure::arrangement::MidiNote *> target_notes;
  for (const auto &obj_ref : extractSelectedObjects ())
    {
      if (
        auto * note =
          obj_ref.template get_object_as<structure::arrangement::MidiNote> ();
        note != nullptr)
        {
          target_notes.push_back (note);
        }
    }
  if (target_notes.empty ())
    {
      if (clip == nullptr)
        {
          z_debug ("No notes selected for velocity ramp");
          return false;
        }
      const double span_start = std::min (start_ticks, end_ticks);
      const double span_end = std::max (start_ticks, end_ticks);
      for (
        auto * note :
        clip->structure::arrangement::ArrangerObjectOwner<
          structure::arrangement::MidiNote>::get_sorted_children_view ())
        {
          const double note_ticks =
            structure::arrangement::timeline_ticks (*note).asDouble ();
          if (note_ticks >= span_start && note_ticks <= span_end)
            {
              target_notes.push_back (note);
            }
        }
      if (target_notes.empty ())
        {
          z_debug ("No notes inside the velocity ramp span");
          return false;
        }
    }

  // Velocity of the ramp line at the given timeline position, clamped to
  // the nearest endpoint outside the line's span
  const auto velocity_at = [=] (double ticks) {
    const double t =
      (end_ticks == start_ticks)
        ? 1.0
        : std::clamp ((ticks - start_ticks) / (end_ticks - start_ticks), 0.0, 1.0);
    return std::clamp (
      static_cast<int> (
        std::lround (start_value + t * (end_value - start_value))),
      0, 127);
  };

  undo::UndoStack::ScopedMacro macro (
    undo_stack_, QObject::tr ("Ramp Velocities"));
  for (auto * note : target_notes)
    {
      const int new_velocity = velocity_at (
        structure::arrangement::timeline_ticks (*note).asDouble ());
      if (new_velocity == note->velocity ())
        continue;
      undo_stack_.push (new commands::ChangeUuidIdentifiableObjectPropertyCommand (
        *note, object_factory_.registry (), QStringLiteral ("velocity"),
        new_velocity));
    }

  return true;
}

bool
ArrangerObjectSelectionOperator::deleteObjects ()
{
  // Extract selected objects from selection model
  auto selected_objects = extractSelectedObjects ();
  if (selected_objects.empty ())
    {
      z_debug ("No objects selected to delete");
      return false;
    }

  // Check for undeletable objects
  const auto all_deletable =
    std::ranges::all_of (selected_objects, [] (const auto &obj_ref) {
      auto obj_var = utils::convert_to_variant_qobj<
        structure::arrangement::ArrangerObjectPtrVariant> (obj_ref.get ());
      return std::visit (
        [] (const auto &obj) {
          return structure::arrangement::is_arranger_object_deletable (*obj);
        },
        obj_var);
    });
  if (!all_deletable)
    {
      z_warning ("Some selected objects cannot be deleted");
      return false;
    }

  // Create and push command
  undo::UndoStack::ScopedMacro macro (
    undo_stack_,
    QObject::tr ("Delete %1 Objects").arg (selected_objects.size ()));
  for (const auto &obj_ref : selected_objects)
    {
      auto obj_var = utils::convert_to_variant_qobj<
        structure::arrangement::ArrangerObjectPtrVariant> (obj_ref.get ());
      auto owner_var = object_owner_provider_ (obj_var);
      std::visit (
        [&] (auto &owner) {
          if (owner == nullptr)
            {
              z_warning ("No owner found for object {}", obj_ref.id ());
              return;
            }
          auto * command =
            new commands::RemoveArrangerObjectCommand (*owner, obj_ref);
          undo_stack_.push (command);
        },
        owner_var);
    }

  return true;
}

bool
ArrangerObjectSelectionOperator::deleteObject (
  structure::arrangement::ArrangerObject * object)
{
  if (object == nullptr)
    {
      z_warning ("No object given to delete");
      return false;
    }

  auto obj_var = utils::convert_to_variant_qobj<
    structure::arrangement::ArrangerObjectPtrVariant> (object);

  const auto deletable = std::visit (
    [] (const auto &obj) {
      return structure::arrangement::is_arranger_object_deletable (*obj);
    },
    obj_var);
  if (!deletable)
    {
      z_warning ("Object {} cannot be deleted", object->get_uuid ());
      return false;
    }

  auto owner_var = object_owner_provider_ (obj_var);
  return std::visit (
    [&] (auto &owner) {
      if (owner == nullptr)
        {
          z_warning ("No owner found for object {}", object->get_uuid ());
          return false;
        }
      const auto &children = owner->get_children_vector ();
      const auto  it = std::ranges::find (
        children, object->get_uuid (),
        &structure::arrangement::ArrangerObjectUuidReference::id);
      if (it == children.end ())
        {
          z_warning ("Object {} not found in its owner", object->get_uuid ());
          return false;
        }
      undo_stack_.push (new commands::RemoveArrangerObjectCommand (*owner, *it));
      return true;
    },
    owner_var);
}

bool
ArrangerObjectSelectionOperator::cutObjectsAt (double ticks)
{
  auto selected_objects = extractSelectedObjects ();
  if (selected_objects.empty ())
    {
      z_debug ("No objects selected to cut");
      return false;
    }

  SelectedObjectsVector targets;
  for (const auto &obj_ref : selected_objects)
    {
      if (is_cuttable_at (obj_ref, ticks))
        targets.push_back (obj_ref);
    }

  return cut_objects (targets, ticks);
}

bool
ArrangerObjectSelectionOperator::cutAllObjectsAt (
  double                         ticks,
  structure::arrangement::Clip * clip)
{
  SelectedObjectsVector targets;
  const auto            collect_if_cuttable = [&] (const auto &obj_ref) {
    if (is_cuttable_at (obj_ref, ticks))
      targets.push_back (obj_ref);
  };

  if (clip != nullptr)
    {
      // Editor context: cut the bounded children of the given clip.
      for (auto * model : clip->get_child_list_models ())
        {
          const int rows = model->rowCount ();
          for (int i = 0; i < rows; ++i)
            {
              auto variant = model->data (
                model->index (i, 0),
                structure::arrangement::ArrangerObjectListModel::
                  ArrangerObjectUuidReferenceRole);
              if (
                auto * obj_ref =
                  variant.value<
                    structure::arrangement::ArrangerObjectUuidReference *> ())
                {
                  collect_if_cuttable (*obj_ref);
                }
            }
        }
    }
  else
    {
      // Timeline context: cut across all tracks and lanes.
      if (!timeline_objects_enumerator_)
        {
          z_warning ("No timeline objects enumerator set - cannot cut all");
          return false;
        }
      timeline_objects_enumerator_ (collect_if_cuttable);
    }

  return cut_objects (targets, ticks);
}

bool
ArrangerObjectSelectionOperator::is_cuttable_at (
  const structure::arrangement::ArrangerObjectUuidReference &obj_ref,
  double                                                     ticks)
{
  const dsp::TimelineTick cut_pos{ units::ticks (ticks) };
  auto                    obj_var = utils::convert_to_variant_qobj<
    structure::arrangement::ArrangerObjectPtrVariant> (obj_ref.get ());
  return std::visit (
    [&] (const auto &obj) {
      using ObjectT = utils::base_type<decltype (obj)>;
      if constexpr (std::is_same_v<ObjectT, structure::arrangement::ChordObject>)
        {
          // Chords are unbounded but effectively span from their position
          // until the next chord or the clip's end, so they are cuttable
          // strictly inside that span.
          const auto * clip = qobject_cast<const structure::arrangement::Clip *> (
            obj->parentObject ());
          if (clip == nullptr)
            return false;
          if (
            cut_pos <= structure::arrangement::timeline_ticks (*clip)
            || cut_pos >= structure::arrangement::timeline_end_ticks (*clip))
            return false;
          const auto cut_content =
            clip->contentWarp ()->timelineToContent (cut_pos);
          if (cut_content <= obj->position ()->asTick ())
            return false;
          const auto next = next_chord_content_position (*obj);
          return !next.has_value () || cut_content < *next;
        }
      else if constexpr (!structure::arrangement::BoundedObject<ObjectT>)
        {
          return false;
        }
      else
        {
          if constexpr (!structure::arrangement::ClipObject<ObjectT>)
            {
              // Non-clip bounded objects (e.g. notes) live in their parent
              // clip's content coordinates — a parent clip is needed to map
              // the cut position.
              if (
                qobject_cast<const structure::arrangement::Clip *> (
                  obj->parentObject ())
                == nullptr)
                return false;
            }
          return cut_pos > structure::arrangement::timeline_ticks (*obj)
                 && cut_pos < structure::arrangement::timeline_end_ticks (*obj);
        }
    },
    obj_var);
}

bool
ArrangerObjectSelectionOperator::cut_objects (
  const SelectedObjectsVector &objects,
  double                       ticks)
{
  if (objects.empty ())
    {
      z_debug ("No cuttable objects at position {}", ticks);
      return false;
    }

  const dsp::TimelineTick cut_pos{ units::ticks (ticks) };

  // Resolve owners up front: objects without an owner are skipped, and if
  // none can be cut the operation is a no-op (no empty undo step).
  struct ResolvedTarget
  {
    structure::arrangement::ArrangerObjectUuidReference obj_ref;
    structure::arrangement::ArrangerObjectPtrVariant    obj_var;
    ArrangerObjectOwnerPtrVariant                       owner_var;
  };
  std::vector<ResolvedTarget> targets;
  targets.reserve (objects.size ());
  for (const auto &obj_ref : objects)
    {
      auto obj_var = utils::convert_to_variant_qobj<
        structure::arrangement::ArrangerObjectPtrVariant> (obj_ref.get ());
      auto       owner_var = object_owner_provider_ (obj_var);
      const bool has_owner = std::visit (
        [] (const auto &owner) { return owner != nullptr; }, owner_var);
      if (!has_owner)
        {
          z_warning ("No owner found for object {}", obj_ref.id ());
          continue;
        }
      targets.push_back (
        { obj_ref, std::move (obj_var), std::move (owner_var) });
    }
  if (targets.empty ())
    {
      z_debug ("No objects with owners to cut at position {}", ticks);
      return false;
    }

  undo::UndoStack::ScopedMacro macro (
    undo_stack_, QObject::tr ("Cut %1 Objects").arg (targets.size ()));
  for (const auto &target : targets)
    {
      std::visit (
        [&] (const auto &obj) {
          using ObjectT = utils::base_type<decltype (obj)>;

          std::optional<structure::arrangement::ArrangerObjectUuidReference>
            new_obj_ref_opt;
          if constexpr (structure::arrangement::BoundedObject<ObjectT>)
            {
              const auto tl_end =
                structure::arrangement::timeline_end_ticks (*obj);

              // Right half: clone and configure BEFORE resizing the original
              // (the resize may change the original's loop range via
              // bounds-tracking)
              auto new_obj_ref =
                object_factory_.clone_new_object_identity (*obj);
              auto * new_obj = new_obj_ref.template get_object_as<ObjectT> ();
              configure_cut_right_half (*obj, *new_obj, cut_pos, tl_end);

              // Left half: resize the original to end at the cut position.
              // For clips the delta is in timeline ticks; for objects inside
              // a clip (e.g. notes, edited in the clip's unwound content
              // space) it is in content ticks, ending the original at the
              // unwound content position under the cut.
              double resize_delta = (cut_pos - tl_end).asDouble ();
              if constexpr (!structure::arrangement::ClipObject<ObjectT>)
                {
                  const auto * parent_clip = qobject_cast<
                    const structure::arrangement::Clip *> (obj->parentObject ());
                  const auto content_at_cut =
                    parent_clip->contentWarp ()->timelineToContent (cut_pos);
                  resize_delta =
                    (content_at_cut
                     - (obj->position ()->asTick () + obj->length ()->asTick ()))
                      .asDouble ();
                }
              undo_stack_.push (new commands::ResizeArrangerObjectsCommand (
                { target.obj_ref }, commands::ResizeType::Bounds,
                commands::ResizeDirection::FromEnd, resize_delta));

              new_obj_ref_opt = std::move (new_obj_ref);
            }
          else if constexpr (
            std::is_same_v<ObjectT, structure::arrangement::ChordObject>)
            {
              // Chords are unbounded and play until the next chord: the
              // clone starting at the cut automatically ends the original's
              // effective span, so no resize is needed. Like other editor
              // content, the clone is placed at the unwound content position
              // under the cut.
              const auto * clip = qobject_cast<
                const structure::arrangement::Clip *> (obj->parentObject ());
              if (clip != nullptr)
                {
                  auto new_obj_ref =
                    object_factory_.clone_new_object_identity (*obj);
                  auto * new_obj =
                    new_obj_ref.template get_object_as<ObjectT> ();
                  new_obj->position ()->setTicks (
                    clip->contentWarp ()->timelineToContent (cut_pos).asDouble ());
                  new_obj_ref_opt = std::move (new_obj_ref);
                }
            }
          if (!new_obj_ref_opt.has_value ())
            return;

          // Add the right half to the same owner.
          std::visit (
            [&] (auto &owner) {
              undo_stack_.push (new commands::AddArrangerObjectCommand (
                *owner, *new_obj_ref_opt));
            },
            target.owner_var);
        },
        target.obj_var);
    }

  return true;
}

bool
ArrangerObjectSelectionOperator::cloneObjects ()
{
  // Extract selected objects from selection model
  auto selected_objects = extractSelectedObjects ();
  if (selected_objects.empty ())
    {
      z_debug ("No objects selected to clone");
      return false;
    }

  // Check for uncloneable objects
  const auto all_cloneable =
    std::ranges::all_of (selected_objects, [] (const auto &obj_ref) {
      auto obj_var = utils::convert_to_variant_qobj<
        structure::arrangement::ArrangerObjectPtrVariant> (obj_ref.get ());
      return std::visit (
        [] (const auto &obj) {
          return structure::arrangement::is_arranger_object_deletable (*obj);
        },
        obj_var);
    });
  if (!all_cloneable)
    {
      z_warning ("Some selected objects cannot be cloned");
      return false;
    }

  // Create and push command
  undo::UndoStack::ScopedMacro macro (
    undo_stack_, QObject::tr ("Copy %1 Objects").arg (selected_objects.size ()));
  for (const auto &obj_ref : selected_objects)
    {
      auto obj_var = utils::convert_to_variant_qobj<
        structure::arrangement::ArrangerObjectPtrVariant> (obj_ref.get ());
      auto new_obj_ref = std::visit (
        [&] (const auto &obj)
          -> structure::arrangement::ArrangerObjectUuidReference {
          return object_factory_.clone_new_object_identity (*obj);
        },
        obj_var);

      auto owner_var = object_owner_provider_ (obj_var);
      std::visit (
        [&] (auto &owner) {
          if (owner == nullptr)
            {
              z_warning ("No owner found for object {}", obj_ref.id ());
              return;
            }
          auto * command =
            new commands::AddArrangerObjectCommand (*owner, new_obj_ref);
          undo_stack_.push (command);
        },
        owner_var);
    }

  return true;
}

bool
ArrangerObjectSelectionOperator::toggleMute ()
{
  auto selected_objects = extractSelectedObjects ();
  if (selected_objects.empty ())
    {
      z_debug ("No objects selected for mute toggle");
      return false;
    }

  struct MuteTarget
  {
    structure::arrangement::ArrangerObject * owner_obj;
    QObject *                                mute_obj;
    bool                                     current_muted;
  };

  std::vector<MuteTarget> targets;
  for (const auto &obj_ref : selected_objects)
    {
      auto * mute = obj_ref.get ()->mute ();
      if (mute != nullptr)
        {
          targets.push_back ({ obj_ref.get (), mute, mute->muted () });
        }
    }

  if (targets.empty ())
    {
      z_debug ("No muteable objects selected");
      return false;
    }

  const bool new_muted = !targets.front ().current_muted;

  undo::UndoStack::ScopedMacro macro (
    undo_stack_,
    new_muted
      ? QObject::tr ("Mute %1 Objects").arg (targets.size ())
      : QObject::tr ("Unmute %1 Objects").arg (targets.size ()));
  for (const auto &target : targets)
    {
      undo_stack_.push (new commands::ChangeUuidIdentifiableObjectPropertyCommand (
        *target.owner_obj, *target.mute_obj, object_factory_.registry (),
        QStringLiteral ("muted"), new_muted));
    }

  return true;
}

bool
ArrangerObjectSelectionOperator::setStretchAlgorithm (
  dsp::StretchOptions::Algorithm algorithm)
{
  auto selected_objects = extractSelectedObjects ();
  if (selected_objects.empty ())
    {
      z_debug ("No objects selected for algorithm change");
      return false;
    }

  std::vector<structure::arrangement::AudioClip *> targets;
  for (const auto &obj_ref : selected_objects)
    {
      auto * clip = obj_ref.get_object_as<structure::arrangement::AudioClip> ();
      if (clip != nullptr)
        targets.push_back (clip);
    }

  if (targets.empty ())
    {
      z_debug ("No audio clips selected");
      return false;
    }

  undo::UndoStack::ScopedMacro macro (
    undo_stack_,
    QObject::tr ("Change Timestretch Algorithm on %1 Clip(s)")
      .arg (targets.size ()));
  for (auto * clip : targets)
    {
      undo_stack_.push (new commands::ChangeUuidIdentifiableObjectPropertyCommand (
        *clip, object_factory_.registry (), QStringLiteral ("stretchAlgorithm"),
        QVariant::fromValue (algorithm)));
    }

  return true;
}

bool
ArrangerObjectSelectionOperator::setTimebaseOverride (dsp::Timebase timebase)
{
  auto selected_objects = extractSelectedObjects ();
  if (selected_objects.empty ())
    {
      z_debug ("No objects selected for timebase override");
      return false;
    }

  undo::UndoStack::ScopedMacro macro (
    undo_stack_, QObject::tr ("Set Timebase Override"));
  for (const auto &obj_ref : selected_objects)
    {
      auto obj_var = utils::convert_to_variant_qobj<
        structure::arrangement::ArrangerObjectPtrVariant> (obj_ref.get ());
      std::visit (
        [&] (const auto &obj) {
          using ObjectT = utils::base_type<decltype (obj)>;
          if constexpr (structure::arrangement::ClipObject<ObjectT>)
            {
              undo_stack_.push (new commands::ChangeTimebaseOverrideCommand (
                *obj->timebaseProvider (), timebase));
            }
        },
        obj_var);
    }
  return true;
}

bool
ArrangerObjectSelectionOperator::clearTimebaseOverride ()
{
  auto selected_objects = extractSelectedObjects ();
  if (selected_objects.empty ())
    {
      z_debug ("No objects selected for timebase clear");
      return false;
    }

  undo::UndoStack::ScopedMacro macro (
    undo_stack_, QObject::tr ("Clear Timebase Override"));
  for (const auto &obj_ref : selected_objects)
    {
      auto obj_var = utils::convert_to_variant_qobj<
        structure::arrangement::ArrangerObjectPtrVariant> (obj_ref.get ());
      std::visit (
        [&] (const auto &obj) {
          using ObjectT = utils::base_type<decltype (obj)>;
          if constexpr (structure::arrangement::ClipObject<ObjectT>)
            {
              undo_stack_.push (new commands::ChangeTimebaseOverrideCommand (
                *obj->timebaseProvider (), std::nullopt));
            }
        },
        obj_var);
    }
  return true;
}

bool
ArrangerObjectSelectionOperator::selectionHasTimebaseProviders () const
{
  auto selected_objects = extractSelectedObjects ();
  return std::ranges::any_of (selected_objects, [] (const auto &obj_ref) {
    auto obj_var = utils::convert_to_variant_qobj<
      structure::arrangement::ArrangerObjectPtrVariant> (obj_ref.get ());
    return std::visit (
      [] (const auto &obj) {
        using ObjectT = utils::base_type<decltype (obj)>;
        return structure::arrangement::ClipObject<ObjectT>;
      },
      obj_var);
  });
}

bool
ArrangerObjectSelectionOperator::moveAutomationPointsByDelta (double delta)
{
  return process_vertical_move (delta);
}

bool
ArrangerObjectSelectionOperator::process_vertical_move (double delta)
{

  if (utils::math::floats_equal (delta, 0.0))
    {
      // No movement needed
      return true;
    }

  // Extract selected objects from selection model
  auto selected_objects = extractSelectedObjects ();
  if (selected_objects.empty ())
    {
      z_warning ("No objects selected for movement");
      return false;
    }

  // Validate object vertical bounds
  if (!validateVerticalMovement (selected_objects, delta))
    {
      z_warning ("Vertical movement validation failed");
      return false;
    }

  // Create and push command
  auto * command = new commands::MoveArrangerObjectsCommand (
    std::move (selected_objects), units::ticks (0), delta);
  undo_stack_.push (command);

  return true;
}

bool
ArrangerObjectSelectionOperator::resizeObjects (
  commands::ResizeType      type,
  commands::ResizeDirection direction,
  double                    delta)
{
  if (utils::math::floats_equal (delta, 0.0))
    {
      // No resize needed
      return true;
    }

  // Extract selected objects from selection model
  auto selected_objects = extractSelectedObjects ();
  if (selected_objects.empty ())
    {
      z_warning ("No objects selected for resize");
      return false;
    }

  // Validate resize operation
  if (!validateResize (selected_objects, type, direction, delta))
    {
      z_warning ("Resize validation failed");
      return false;
    }

  // Create and push command
  auto * command = new commands::ResizeArrangerObjectsCommand (
    std::move (selected_objects), type, direction, delta);
  undo_stack_.push (command);

  return true;
}

auto
ArrangerObjectSelectionOperator::extractSelectedObjects () const
  -> SelectedObjectsVector
{
  SelectedObjectsVector objects;

  const auto selected_indexes = selection_model_.selectedIndexes ();
  for (const auto &index : selected_indexes)
    {
      // Get the object from the model index
      auto variant = index.data (
        structure::arrangement::ArrangerObjectListModel::
          ArrangerObjectUuidReferenceRole);
      if (
        variant
          .canConvert<structure::arrangement::ArrangerObjectUuidReference *> ())
        {
          auto * obj_ref = variant.value<
            structure::arrangement::ArrangerObjectUuidReference *> ();
          objects.push_back (*obj_ref);
        }
    }

  return objects;
}

bool
ArrangerObjectSelectionOperator::validateHorizontalMovement (
  const SelectedObjectsVector &objects,
  double                       tick_delta)
{
  return std::ranges::all_of (objects, [tick_delta] (const auto &obj_ref) {
    auto obj_var = utils::convert_to_variant_qobj<
      structure::arrangement::ArrangerObjectPtrVariant> (obj_ref.get ());
    return std::visit (
      [&] (auto &&obj) {
        using ObjectT = utils::base_type<decltype (obj)>;
        const auto current_timeline =
          structure::arrangement::timeline_ticks (*obj);
        const auto new_timeline =
          current_timeline + dsp::TimelineTick{ units::ticks (tick_delta) };
        if constexpr (
          std::is_same_v<ObjectT, structure::arrangement::TimeSignatureObject>)
          {
            // Time Signature objects are only allowed at bar boundaries
            const auto &tempo_map = obj->get_tempo_map ();
            const auto  musical_pos = tempo_map.tick_to_musical_position (
              au::round_as<int64_t> (units::ticks, new_timeline.asQuantity ()));
            if (
              musical_pos.beat != 1 || musical_pos.sixteenth != 1
              || musical_pos.tick != 0)
              {
                return false;
              }
          }
        return new_timeline.asDouble () >= 0.0;
      },
      obj_var);
  });
}

bool
ArrangerObjectSelectionOperator::validateVerticalMovement (
  const SelectedObjectsVector &objects,
  double                       delta)
{
  return zrythm::ranges::all_equal (
           objects, [] (const auto &obj_ref) { return obj_ref.get ()->type (); })
         && std::ranges::all_of (objects, [delta] (const auto &obj_ref) {
              auto obj_var = utils::convert_to_variant_qobj<
                structure::arrangement::ArrangerObjectPtrVariant> (
                obj_ref.get ());
              return std::visit (
                [&] (auto &&obj) {
                  using ObjectT = utils::base_type<decltype (obj)>;
                  if constexpr (
                    std::is_same_v<ObjectT, structure::arrangement::MidiNote>)
                    {
                      const auto new_pitch =
                        obj->pitch () + static_cast<int> (delta);
                      return new_pitch >= 0 && new_pitch < 128;
                    }
                  if constexpr (
                    std::is_same_v<
                      ObjectT, structure::arrangement::AutomationPoint>)
                    {
                      const auto new_value =
                        obj->value () + static_cast<float> (delta);
                      return new_value >= 0.0 && new_value <= 1.0;
                    }

                  return false; // Object unsupported for delta moving
                },
                obj_var);
            });
}

bool
ArrangerObjectSelectionOperator::validateResize (
  const SelectedObjectsVector &objects,
  commands::ResizeType         type,
  commands::ResizeDirection    direction,
  double                       delta)
{
  return std::ranges::all_of (
    objects, [type, direction, delta] (const auto &obj_ref) {
      auto * obj = obj_ref.get ();
      assert (obj != nullptr);
      switch (type)
        {
        case commands::ResizeType::Bounds:
        case commands::ResizeType::LoopPoints:
          return validateBoundsResize (
            utils::convert_to_variant_qobj<
              structure::arrangement::ArrangerObjectPtrVariant> (obj),
            direction, delta);
        case commands::ResizeType::Fades:
          return validateFadesResize (
            utils::convert_to_variant_qobj<
              structure::arrangement::ArrangerObjectPtrVariant> (obj),
            direction, delta);
        default:
          return false;
        }
    });
}

bool
ArrangerObjectSelectionOperator::validateBoundsResize (
  structure::arrangement::ArrangerObjectPtrVariant obj_var,
  commands::ResizeDirection                        direction,
  double                                           delta)
{
  return std::visit (
    [&] (const auto &obj) {
      using ObjectT = utils::base_type<decltype (obj)>;
      if constexpr (!structure::arrangement::BoundedObject<ObjectT>)
        return false; // Object doesn't support bounds

      if (direction == commands::ResizeDirection::FromStart)
        {
          if constexpr (structure::arrangement::TimelineObject<ObjectT>)
            {
              // Check that new position won't be negative
              const double new_position = obj->position ()->ticks () + delta;
              if (new_position < 0.0)
                return false;
            }
        }

      // Check that new length won't be less than minimum (1 tick)
      const double current_length = obj->length ()->ticks ();
      const double new_length =
        (direction == commands::ResizeDirection::FromStart)
          ? current_length - delta
          : current_length + delta;

      return new_length >= 1.0;
    },
    obj_var);
}

bool
ArrangerObjectSelectionOperator::validateFadesResize (
  structure::arrangement::ArrangerObjectPtrVariant obj_var,
  commands::ResizeDirection                        direction,
  double                                           delta)
{
  return std::visit (
    [&] (const auto &obj) {
      using ObjectT = utils::base_type<decltype (obj)>;
      if constexpr (!structure::arrangement::FadeableObject<ObjectT>)
        {
          return false; // Object doesn't support fades
        }
      else
        {
          const double fade_in = obj->fadeRange ()->startOffset ()->ticks ();
          const double fade_out = obj->fadeRange ()->endOffset ()->ticks ();

          if (direction == commands::ResizeDirection::FromStart)
            {
              // Reject drags that would drive the fade-in offset negative.
              if (fade_in + delta < 0.0)
                return false;
            }
          else // FromEnd
            {
              // Reject drags that would drive the fade-out offset negative.
              if (fade_out + delta < 0.0)
                return false;
            }

          return true;
        }
    },
    obj_var);
}

} // namespace zrythm::actions
