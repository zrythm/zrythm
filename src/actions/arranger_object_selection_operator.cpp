// SPDX-FileCopyrightText: © 2025-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "utils/format_qt.h"

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
#include "utils/exceptions.h"
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
  ObjectOwnerProvider                            objectOwnerProvider,
  structure::arrangement::ArrangerObjectFactory &objectFactory,
  structure::project::ProjectRegistry           &projectRegistry,
  controllers::Clipboard                        &clipboard,
  ProjectIdProvider                              projectIdProvider,
  TimelineObjectsEnumerator                      timelineObjectsEnumerator,
  QObject *                                      parent)
    : QObject (parent), undo_stack_ (undoStack),
      object_owner_provider_ (std::move (objectOwnerProvider)),
      object_factory_ (objectFactory), project_registry_ (projectRegistry),
      clipboard_ (clipboard),
      project_id_provider_ (std::move (projectIdProvider)),
      timeline_objects_enumerator_ (std::move (timelineObjectsEnumerator))
{
}

bool
ArrangerObjectSelectionOperator::moveByTicks (
  QItemSelectionModel * selectionModel,
  double                tick_delta)
{
  if (tick_delta == 0.0)
    {
      // No movement needed
      return true;
    }

  // Extract selected objects from selection model
  auto selected_objects = extractSelectedObjects (selectionModel);
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
ArrangerObjectSelectionOperator::moveNotesByPitch (
  QItemSelectionModel * selectionModel,
  int                   pitch_delta)
{
  return process_vertical_move (
    selectionModel, static_cast<double> (pitch_delta));
}

bool
ArrangerObjectSelectionOperator::changeVelocities (
  QItemSelectionModel * selectionModel,
  int                   velocity_delta)
{
  if (velocity_delta == 0)
    {
      return true;
    }

  // Extract selected objects from selection model
  auto selected_objects = extractSelectedObjects (selectionModel);
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
  QItemSelectionModel *              selectionModel,
  structure::arrangement::MidiClip * clip,
  double                             start_ticks,
  double                             start_value,
  double                             end_ticks,
  double                             end_value)
{
  // Collect the target notes: the selected MIDI notes, or, when none are
  // selected, all notes of the given clip inside the line's tick span
  if (selectionModel == nullptr)
    {
      z_debug ("No selection model given; velocity ramp is a no-op");
      return false;
    }

  std::vector<structure::arrangement::MidiNote *> target_notes;
  for (const auto &obj_ref : extractSelectedObjects (selectionModel))
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
ArrangerObjectSelectionOperator::deleteObjects (
  QItemSelectionModel * selectionModel)
{
  // Extract selected objects from selection model
  auto selected_objects = extractSelectedObjects (selectionModel);
  if (selected_objects.empty ())
    {
      z_debug ("No objects selected to delete");
      return false;
    }

  // Validate before opening the macro: a refusal never opens one
  OwnerResolver resolver{ object_owner_provider_ };
  if (!can_delete_objects (selected_objects, resolver))
    {
      z_warning ("Some selected objects cannot be deleted");
      return false;
    }

  // Create and push command
  undo::UndoStack::ScopedMacro macro (
    undo_stack_,
    QObject::tr ("Delete %1 Objects").arg (selected_objects.size ()));
  return delete_objects (selected_objects, resolver);
}

bool
ArrangerObjectSelectionOperator::all_objects_deletable (
  const SelectedObjectsVector &objects)
{
  return std::ranges::all_of (objects, [] (const auto &obj_ref) {
    auto obj_var = utils::convert_to_variant_qobj<
      structure::arrangement::ArrangerObjectPtrVariant> (obj_ref.get ());
    return std::visit (
      [] (const auto &obj) {
        return structure::arrangement::is_arranger_object_deletable (*obj);
      },
      obj_var);
  });
}

bool
ArrangerObjectSelectionOperator::all_objects_copyable (
  const SelectedObjectsVector &objects)
{
  return std::ranges::all_of (objects, [] (const auto &obj_ref) {
    auto obj_var = utils::convert_to_variant_qobj<
      structure::arrangement::ArrangerObjectPtrVariant> (obj_ref.get ());
    return std::visit (
      [] (const auto * obj) {
        return structure::arrangement::is_arranger_object_copyable (*obj);
      },
      obj_var);
  });
}

bool
ArrangerObjectSelectionOperator::delete_objects (
  const SelectedObjectsVector &objects,
  OwnerResolver               &resolver)
{
  if (!all_objects_deletable (objects))
    {
      z_warning ("Some selected objects cannot be deleted");
      return false;
    }

  // Resolve all owners first: either all objects are deleted or none
  struct DeleteTarget
  {
    structure::arrangement::ArrangerObjectUuidReference obj_ref;
    ArrangerObjectOwnerPtrVariant                       owner_var;
  };
  std::vector<DeleteTarget> targets;
  targets.reserve (objects.size ());
  for (const auto &obj_ref : objects)
    {
      auto obj_var = utils::convert_to_variant_qobj<
        structure::arrangement::ArrangerObjectPtrVariant> (obj_ref.get ());
      auto owner_var = resolver.resolve (obj_var);
      if (
        std::visit (
          [] (const auto &owner) { return owner == nullptr; }, owner_var))
        {
          z_warning ("No owner found for object {}", obj_ref.id ());
          return false;
        }
      targets.push_back ({ obj_ref, owner_var });
    }

  if (targets.empty ())
    return false;

  for (auto &target : targets)
    {
      std::visit (
        [&] (auto &owner) {
          undo_stack_.push (
            new commands::RemoveArrangerObjectCommand (*owner, target.obj_ref));
        },
        target.owner_var);
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
ArrangerObjectSelectionOperator::cutObjectsAt (
  QItemSelectionModel * selectionModel,
  double                ticks)
{
  auto selected_objects = extractSelectedObjects (selectionModel);
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
ArrangerObjectSelectionOperator::cloneObjects (
  QItemSelectionModel * selectionModel)
{
  // Extract selected objects from selection model
  auto selected_objects = extractSelectedObjects (selectionModel);
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
      clone_and_attach (obj_var);
    }

  return true;
}

std::optional<QUuid>
ArrangerObjectSelectionOperator::clone_and_attach (
  structure::arrangement::ArrangerObjectPtrVariant                      obj_var,
  const std::function<void (structure::arrangement::ArrangerObject &)> &mutate)
{
  auto new_obj_ref = std::visit (
    [&] (const auto * obj) -> structure::arrangement::ArrangerObjectUuidReference {
      return object_factory_.clone_new_object_identity (*obj);
    },
    obj_var);

  if (mutate)
    mutate (*new_obj_ref.get ());

  auto       owner_var = object_owner_provider_ (obj_var);
  const bool attached = std::visit (
    [&] (auto &owner) {
      if (owner == nullptr)
        {
          z_warning (
            "No owner found for object {}",
            std::visit (
              [] (const auto * obj) { return obj->get_uuid (); }, obj_var));
          return false;
        }
      undo_stack_.push (
        new commands::AddArrangerObjectCommand (*owner, new_obj_ref));
      return true;
    },
    owner_var);
  if (!attached)
    {
      // The clone stays unowned: it is destroyed automatically once its
      // last reference (new_obj_ref here, if no command was pushed) goes
      // away
      return std::nullopt;
    }
  return type_safe::get (new_obj_ref.id ());
}

bool
ArrangerObjectSelectionOperator::copyObjects (
  QItemSelectionModel * selectionModel)
{
  auto selected_objects = extractSelectedObjects (selectionModel);
  if (selected_objects.empty ())
    {
      z_debug ("No objects selected to copy");
      return false;
    }

  OwnerResolver resolver{ object_owner_provider_ };
  return copy_objects (copyable_objects (selected_objects), resolver);
}

ArrangerObjectSelectionOperator::SelectedObjectsVector
ArrangerObjectSelectionOperator::copyable_objects (
  const SelectedObjectsVector &objects)
{
  SelectedObjectsVector copyable;
  for (const auto &obj_ref : objects)
    {
      auto obj_var = utils::convert_to_variant_qobj<
        structure::arrangement::ArrangerObjectPtrVariant> (obj_ref.get ());
      const bool copyable_object = std::visit (
        [] (const auto &obj) {
          return structure::arrangement::is_arranger_object_copyable (*obj);
        },
        obj_var);
      if (copyable_object)
        copyable.push_back (obj_ref);
      else
        {
          z_warning ("Object {} cannot be copied and is skipped", obj_ref.id ());
        }
    }
  return copyable;
}

ArrangerObjectSelectionOperator::ArrangerObjectOwnerPtrVariant
ArrangerObjectSelectionOperator::OwnerResolver::resolve (
  const structure::arrangement::ArrangerObjectPtrVariant &obj_var)
{
  const auto * raw = std::visit (
    [] (const auto * obj) -> const QObject * { return obj; }, obj_var);
  if (const auto it = cache_.find (raw); it != cache_.end ())
    return it->second;
  auto owner_var = provider_ (obj_var);
  cache_.emplace (raw, owner_var);
  return owner_var;
}

bool
ArrangerObjectSelectionOperator::can_delete_objects (
  const SelectedObjectsVector &objects,
  OwnerResolver               &resolver) const
{
  return std::ranges::all_of (objects, [&resolver] (const auto &obj_ref) {
    auto obj_var = utils::convert_to_variant_qobj<
      structure::arrangement::ArrangerObjectPtrVariant> (obj_ref.get ());
    const bool deletable = std::visit (
      [] (const auto &obj) {
        return structure::arrangement::is_arranger_object_deletable (*obj);
      },
      obj_var);
    if (!deletable)
      return false;
    const auto owner_var = resolver.resolve (obj_var);
    return std::visit (
      [] (const auto &owner) { return owner != nullptr; }, owner_var);
  });
}

bool
ArrangerObjectSelectionOperator::copy_objects (
  const SelectedObjectsVector &objects,
  OwnerResolver               &resolver)
{
  if (objects.empty ())
    {
      z_warning ("No copyable objects selected");
      return false;
    }

  if (!selection_in_single_position_space (objects))
    {
      z_warning (
        "Cannot copy a selection that mixes timeline and clip contents "
        "objects");
      return false;
    }

  // The paste anchor is the earliest position in the selection's position
  // space (timeline ticks on the timeline, content ticks in an editor —
  // uniform within one arranger pane).
  const auto anchor = std::ranges::min (object_positions (objects));

  // Remember the source lane of lane clips so paste can use the same lane
  // when the target track has one. The owner variant only knows the
  // ArrangerObjectOwner<T> base, so recover the concrete track lane (the
  // only owner type that is a track lane) with a cross-cast.
  nlohmann::json lane_indices = nlohmann::json::object ();
  for (const auto &obj_ref : objects)
    {
      auto obj_var = utils::convert_to_variant_qobj<
        structure::arrangement::ArrangerObjectPtrVariant> (obj_ref.get ());
      auto owner_var = resolver.resolve (obj_var);
      std::visit (
        [&] (auto &owner) {
          if (owner == nullptr)
            return;
          auto * lane = dynamic_cast<structure::tracks::TrackLane *> (owner);
          if (lane == nullptr)
            return;
          const auto * lane_list = qobject_cast<
            const structure::tracks::TrackLaneList *> (lane->parent ());
          const auto lane_idx =
            lane_list != nullptr ? lane_list->indexOfLane (lane) : std::nullopt;
          if (lane_idx.has_value ())
            {
              lane_indices
                [type_safe::get (obj_ref.id ())
                   .toString (QUuid::WithoutBraces)
                   .toStdString ()] = *lane_idx;
            }
        },
        owner_var);
    }

  auto metadata = nlohmann::json::object ();
  metadata[structure::project::ClipboardPayload::kAnchorTicksMetadataKey] =
    anchor.in (units::ticks);
  if (!lane_indices.empty ())
    metadata[structure::project::ClipboardPayload::kLaneIndicesMetadataKey] =
      std::move (lane_indices);

  std::vector<QUuid> roots;
  roots.reserve (objects.size ());
  for (const auto &obj_ref : objects)
    roots.push_back (type_safe::get (obj_ref.id ()));

  try
    {
      clipboard_.setPayload (
        structure::project::ClipboardPayload::create (
          project_registry_,
          structure::project::ClipboardPayload::Type::ArrangerObjects, roots,
          project_id_provider_ (), std::move (metadata)));
    }
  catch (const std::exception &e)
    {
      z_warning ("Failed to store copied objects: {}", e.what ());
      refuse_operation (
        QObject::tr ("The objects could not be copied to the clipboard"));
      return false;
    }
  return true;
}

bool
ArrangerObjectSelectionOperator::cutObjects (
  QItemSelectionModel * selectionModel)
{
  auto selected_objects = extractSelectedObjects (selectionModel);
  if (selected_objects.empty ())
    {
      z_debug ("No objects selected to cut");
      return false;
    }

  // Refuse before copying so cut never copies without deleting: cut
  // needs every selected object copyable and deletable. One resolver is
  // shared by the can-delete/copy/delete phases.
  OwnerResolver resolver{ object_owner_provider_ };
  if (
    !all_objects_copyable (selected_objects)
    || !can_delete_objects (selected_objects, resolver))
    {
      z_warning ("Some selected objects cannot be cut");
      refuse_operation (QObject::tr ("Some selected objects cannot be cut"));
      return false;
    }

  // Delete exactly the objects that were copied
  const auto copyable = copyable_objects (selected_objects);
  if (!copy_objects (copyable, resolver))
    return false;

  undo::UndoStack::ScopedMacro macro (
    undo_stack_, QObject::tr ("Cut %1 Objects").arg (copyable.size ()));
  return delete_objects (copyable, resolver);
}

void
ArrangerObjectSelectionOperator::refuse_operation (const QString &reason)
{
  z_warning ("Refusing operation: {}", reason);
  Q_EMIT operationRefused (reason);
}

std::optional<units::precise_tick_t>
ArrangerObjectSelectionOperator::duplicate_shift (
  const SelectedObjectsVector &objects)
{
  // Shift the duplicates by the selection's span so they continue where
  // the selection ends. For a zero-span selection (point objects at the
  // same position), shift by one bar instead.
  const auto positions = object_positions (objects);
  const auto first_pos = std::ranges::min (positions);
  auto       shift =
    std::ranges::max (
      objects | std::views::transform ([] (const auto &obj_ref) {
        return object_end_ticks (
          utils::convert_to_variant_qobj<
            structure::arrangement::ArrangerObjectPtrVariant> (obj_ref.get ()));
      }))
    - first_pos;
  if (shift <= units::precise_tick_t{})
    {
      // Measure the bar at the selection's timeline position: editor
      // objects' positions are clip-relative, so their pane-local position
      // must be offset by the owning clip's timeline position (the owner
      // of an editor object is its clip by design; the first object's clip
      // is a close-enough anchor for this bar-length fallback)
      const auto first_pos_in_timeline_ticks =
        [first_pos, &first_object = objects.front ()] {
          if (
            auto * parent_clip = qobject_cast<structure::arrangement::Clip *> (
              first_object.get ()->parentObject ()))
            {
              return first_pos + parent_clip->position ()->asTick ().asQuantity ();
            }
          return first_pos;
        }();
      shift =
        objects.front ()
          .get ()
          ->get_tempo_map ()
          .time_signature_at_tick (
            au::round_as<int64_t> (units::ticks, first_pos_in_timeline_ticks))
          .ticks_per_bar ();
    }

  if (shift <= units::precise_tick_t{})
    return std::nullopt;
  return shift;
}

QVariantList
ArrangerObjectSelectionOperator::duplicateObjects (
  QItemSelectionModel * selectionModel)
{
  auto selected_objects = extractSelectedObjects (selectionModel);
  if (selected_objects.empty ())
    {
      z_debug ("No objects selected to duplicate");
      return {};
    }
  // Duplication clones and re-attaches each selected object
  if (!all_objects_copyable (selected_objects))
    {
      z_warning ("Some selected objects cannot be duplicated");
      return {};
    }
  if (!selection_in_single_position_space (selected_objects))
    {
      z_warning (
        "Cannot duplicate a selection that mixes timeline and clip "
        "contents objects");
      return {};
    }

  const auto shift = duplicate_shift (selected_objects);
  if (!shift.has_value ())
    {
      z_warning ("Cannot determine the duplicate placement");
      return {};
    }

  // Time signatures are only allowed at bar boundaries: refuse the whole
  // duplication when a duplicate would land off a bar
  if (!std::ranges::all_of (selected_objects, [&shift] (const auto &obj_ref) {
        return time_signature_lands_on_bar (
          utils::convert_to_variant_qobj<
            structure::arrangement::ArrangerObjectPtrVariant> (obj_ref.get ()),
          dsp::TimelineTick{ *shift });
      }))
    {
      refuse_operation (
        QObject::tr ("Time signatures can only be placed at bar boundaries"));
      return {};
    }

  QVariantList                 new_ids;
  undo::UndoStack::ScopedMacro macro (
    undo_stack_,
    QObject::tr ("Duplicate %1 Objects").arg (selected_objects.size ()));
  for (const auto &obj_ref : selected_objects)
    {
      auto obj_var = utils::convert_to_variant_qobj<
        structure::arrangement::ArrangerObjectPtrVariant> (obj_ref.get ());
      if (
        const auto new_id = clone_and_attach (
          obj_var,
          [shift] (structure::arrangement::ArrangerObject &clone) {
            clone.position ()->addTicks (shift->in (units::ticks));
          });
        new_id.has_value ())
        {
          new_ids.push_back (new_id->toString (QUuid::WithoutBraces));
        }
    }

  return new_ids;
}

ArrangerObjectSelectionOperator::PasteOwnerResolver
ArrangerObjectSelectionOperator::timeline_paste_owner_resolver (
  structure::tracks::Track *                   targetTrack,
  structure::tracks::MarkerTrack *             markerTrack,
  structure::tracks::ChordTrack *              chordTrack,
  structure::arrangement::TempoObjectManager * tempoObjectManager) const
{
  return
    [targetTrack, markerTrack, chordTrack, tempoObjectManager] (
      const PreparedPaste                              &paste,
      const structure::arrangement::ArrangerObjectUuid &root_uuid,
      structure::arrangement::ArrangerObjectPtrVariant  obj_var)
      -> std::optional<ArrangerObjectOwnerPtrVariant> {
      std::optional<ArrangerObjectOwnerPtrVariant> owner_var = std::nullopt;
      std::visit (
        [&] (const auto * obj) {
          using ObjectT = utils::base_type<decltype (obj)>;
          if constexpr (
            std::is_same_v<ObjectT, structure::arrangement::MidiClip>
            || std::is_same_v<ObjectT, structure::arrangement::AudioClip>)
            {
              // Lane clips are pasted into the target track's lanes, using
              // the copied lane index when the target track has that lane
              const bool track_compatible = [&] {
                if constexpr (
                  std::is_same_v<ObjectT, structure::arrangement::MidiClip>)
                  return qobject_cast<const structure::tracks::InstrumentTrack *> (
                           targetTrack)
                           != nullptr
                         || qobject_cast<const structure::tracks::MidiTrack *> (
                              targetTrack)
                              != nullptr;
                else
                  return qobject_cast<const structure::tracks::AudioTrack *> (
                           targetTrack)
                         != nullptr;
              }();
              const auto * lanes =
                targetTrack != nullptr ? targetTrack->lanes () : nullptr;
              if (!track_compatible || lanes == nullptr || lanes->empty ())
                {
                  z_warning (
                    "Cannot paste a {} onto the given track", obj->type ());
                  return;
                }
              std::size_t lane_idx = 0;
              if (
                const auto lane_it = paste.lane_indices.find (root_uuid);
                lane_it != paste.lane_indices.end ())
                {
                  lane_idx = lane_it->second;
                }
              if (lane_idx >= lanes->size ())
                {
                  z_warning (
                    "Cannot paste a {} into lane {} (target track has {} "
                    "lanes)",
                    obj->type (), lane_idx, lanes->size ());
                  return;
                }
              owner_var = static_cast<
                structure::arrangement::ArrangerObjectOwner<ObjectT> *> (
                lanes->at (lane_idx));
            }
          else if constexpr (
            std::is_same_v<ObjectT, structure::arrangement::Marker>)
            {
              if (markerTrack == nullptr)
                {
                  z_warning ("Cannot paste a marker without a marker track");
                  return;
                }
              owner_var = static_cast<
                structure::arrangement::ArrangerObjectOwner<ObjectT> *> (
                markerTrack);
            }
          else if constexpr (
            std::is_same_v<ObjectT, structure::arrangement::ScaleObject>
            || std::is_same_v<ObjectT, structure::arrangement::ChordClip>)
            {
              if (chordTrack == nullptr)
                {
                  z_warning ("Cannot paste without a chord track");
                  return;
                }
              owner_var = static_cast<
                structure::arrangement::ArrangerObjectOwner<ObjectT> *> (
                chordTrack);
            }
          else if constexpr (
            std::is_same_v<ObjectT, structure::arrangement::TempoObject>
            || std::is_same_v<
              ObjectT, structure::arrangement::TimeSignatureObject>)
            {
              if (tempoObjectManager == nullptr)
                {
                  z_warning ("Cannot paste without a tempo object manager");
                  return;
                }
              owner_var = static_cast<
                structure::arrangement::ArrangerObjectOwner<ObjectT> *> (
                tempoObjectManager);
            }
          else
            {
              // Automation clips need their automation track and editor
              // objects their clip: neither is known on the timeline
              z_warning (
                "{} objects cannot be pasted on the timeline", obj->type ());
            }
        },
        obj_var);
      return owner_var;
    };
}

QVariantList
ArrangerObjectSelectionOperator::pasteObjectsOnTimeline (
  structure::tracks::Track *                   targetTrack,
  structure::tracks::MarkerTrack *             markerTrack,
  structure::tracks::ChordTrack *              chordTrack,
  structure::arrangement::TempoObjectManager * tempoObjectManager,
  double                                       playheadTicks)
{
  const auto paste_target = prepare_paste (units::ticks (playheadTicks));
  if (!paste_target.has_value ())
    return {};

  return attach_paste_targets (
    *paste_target,
    timeline_paste_owner_resolver (
      targetTrack, markerTrack, chordTrack, tempoObjectManager));
}

QVariantList
ArrangerObjectSelectionOperator::attach_paste_targets (
  const PreparedPaste      &paste,
  const PasteOwnerResolver &resolve_owner)
{
  struct PasteTarget
  {
    structure::arrangement::ArrangerObjectUuidReference root_ref;
    ArrangerObjectOwnerPtrVariant                       owner_var;
  };
  std::vector<PasteTarget> targets;
  std::vector<QUuid>       pasted_root_ids;
  // Set when a root must refuse the whole paste; handled after the loop:
  // the collected root references are dropped first so the discard can
  // sweep the imports
  bool    paste_refused = false;
  QString refusal_reason;

  for (const auto &root_id : paste.payload.roots ())
    {
      const auto root_uuid =
        structure::arrangement::ArrangerObjectUuid (root_id);
      structure::arrangement::ArrangerObjectUuidReference root_ref{
        root_uuid, project_registry_
      };
      if (root_ref.get () == nullptr)
        {
          z_warning ("Clipboard: imported object {} not found", root_id);
          continue;
        }

      auto obj_var = utils::convert_to_variant_qobj<
        structure::arrangement::ArrangerObjectPtrVariant> (root_ref.get ());
      const auto owner_var = resolve_owner (paste, root_uuid, obj_var);
      if (!owner_var.has_value ())
        continue;

      // The payload's anchor is untrusted metadata: validate the
      // position the root will actually hold after the shift — editor
      // objects shift in clip-relative ticks, timeline objects in
      // absolute ticks (the same rule interactive moves follow)
      if (
        root_ref.get ()->position ()->ticks () + paste.delta.in (units::ticks)
        < 0.0)
        {
          z_warning (
            "Cannot paste: object {} would land before the start of the "
            "destination",
            root_id.toString (QUuid::WithoutBraces));
          paste_refused = true;
          refusal_reason = QObject::tr (
            "Objects cannot be placed before the start of the destination");
          break;
        }

      // Time signatures are only allowed at bar boundaries
      if (
        !time_signature_lands_on_bar (obj_var, dsp::TimelineTick{ paste.delta }))
        {
          z_warning ("Cannot paste: a time signature would land off a bar");
          paste_refused = true;
          refusal_reason = QObject::tr (
            "Time signatures can only be placed at bar boundaries");
          break;
        }

      targets.push_back ({ std::move (root_ref), *owner_var });
      pasted_root_ids.push_back (root_id);
    }

  if (paste_refused)
    {
      // Drop the references of already-collected roots first: their
      // destruction releases (and cascades away) the imported objects, so
      // the discard below only needs to sweep what is left
      targets.clear ();
      paste.payload.cleanup_failed_import (
        project_registry_, paste.imported_ids);
      refuse_operation (refusal_reason);
      return {};
    }

  if (targets.empty ())
    {
      paste.payload.cleanup_failed_import (
        project_registry_, paste.imported_ids);
      z_warning ("None of the clipboard objects could be pasted here");
      refuse_operation (
        QObject::tr ("The clipboard objects could not be pasted here"));
      return {};
    }

  // Shift the validated roots into place
  for (auto &target : targets)
    {
      target.root_ref.get ()->position ()->addTicks (
        paste.delta.in (units::ticks));
    }

  // Sweep the imports the pasted roots do not need. Copied payloads
  // carry exactly their closure, so this is a no-op for them; payloads
  // carrying surplus entries get them deregistered instead of leaving
  // unowned objects behind
  paste.payload.discard_imports_except (
    project_registry_, paste.imported_ids, pasted_root_ids);

  QVariantList                 new_ids;
  undo::UndoStack::ScopedMacro macro (
    undo_stack_, QObject::tr ("Paste %1 Objects").arg (targets.size ()));
  for (auto &target : targets)
    {
      std::visit (
        [&] (auto &owner) {
          undo_stack_.push (
            new commands::AddArrangerObjectCommand (*owner, target.root_ref));
        },
        target.owner_var);
      new_ids.push_back (
        type_safe::get (target.root_ref.id ()).toString (QUuid::WithoutBraces));
    }

  // Surface cross-project content loss after a successful paste (the
  // pasted remainder is intact; what is gone is gone either way)
  if (paste.dropped_audio_objects > 0 || paste.severed_references > 0)
    {
      QStringList parts;
      if (paste.dropped_audio_objects > 0)
        parts << QObject::tr (
          "%n audio item(s) were dropped", nullptr,
          static_cast<int> (paste.dropped_audio_objects));
      if (paste.severed_references > 0)
        parts << QObject::tr (
          "%n routing(s) were severed", nullptr,
          static_cast<int> (paste.severed_references));
      const auto summary =
        parts.join (QStringLiteral (" · "))
        + QObject::tr (
          " (content from another project could not be resolved here)");
      z_warning (
        "Paste modified cross-project content: {}", summary.toStdString ());
      Q_EMIT pasteContentModified (summary);
    }

  return new_ids;
}

ArrangerObjectSelectionOperator::PasteOwnerResolver
ArrangerObjectSelectionOperator::clip_paste_owner_resolver (
  structure::arrangement::Clip * clip)
{
  return
    [clip] (
      const PreparedPaste & /*paste*/,
      const structure::arrangement::ArrangerObjectUuid & /*root_uuid*/,
      structure::arrangement::ArrangerObjectPtrVariant obj_var)
      -> std::optional<ArrangerObjectOwnerPtrVariant> {
      std::optional<ArrangerObjectOwnerPtrVariant> owner_var = std::nullopt;
      std::visit (
        [&] (const auto * obj) {
          using ObjectT = utils::base_type<decltype (obj)>;
          if constexpr (
            std::is_same_v<ObjectT, structure::arrangement::MidiNote>
            || std::is_same_v<ObjectT, structure::arrangement::MidiControlEvent>)
            {
              if (
                auto * target_clip =
                  qobject_cast<structure::arrangement::MidiClip *> (clip))
                owner_var = static_cast<
                  structure::arrangement::ArrangerObjectOwner<ObjectT> *> (
                  target_clip);
            }
          else if constexpr (
            std::is_same_v<ObjectT, structure::arrangement::AutomationPoint>)
            {
              if (
                auto * target_clip =
                  qobject_cast<structure::arrangement::AutomationClip *> (clip))
                owner_var = static_cast<
                  structure::arrangement::ArrangerObjectOwner<ObjectT> *> (
                  target_clip);
            }
          else if constexpr (
            std::is_same_v<ObjectT, structure::arrangement::ChordObject>)
            {
              if (
                auto * target_clip =
                  qobject_cast<structure::arrangement::ChordClip *> (clip))
                owner_var = static_cast<
                  structure::arrangement::ArrangerObjectOwner<ObjectT> *> (
                  target_clip);
            }
          else if constexpr (
            std::is_same_v<ObjectT, structure::arrangement::AudioSourceObject>)
            {
              if (
                auto * target_clip =
                  qobject_cast<structure::arrangement::AudioClip *> (clip))
                owner_var = static_cast<
                  structure::arrangement::ArrangerObjectOwner<ObjectT> *> (
                  target_clip);
            }
          if (!owner_var.has_value ())
            {
              z_warning (
                "{} objects cannot be pasted into this clip", obj->type ());
            }
        },
        obj_var);
      return owner_var;
    };
}

QVariantList
ArrangerObjectSelectionOperator::pasteObjectsIntoClip (
  structure::arrangement::Clip * clip,
  double                         positionTicks)
{
  if (clip == nullptr)
    {
      z_warning ("No clip given to paste into");
      return {};
    }

  const auto paste_target = prepare_paste (units::ticks (positionTicks));
  if (!paste_target.has_value ())
    return {};

  return attach_paste_targets (*paste_target, clip_paste_owner_resolver (clip));
}

auto
ArrangerObjectSelectionOperator::prepare_paste (units::precise_tick_t position)
  -> std::optional<PreparedPaste>
{
  const auto &clipboard_payload = clipboard_.payload ();
  if (!clipboard_payload.has_value ())
    {
      z_debug ("No clipboard payload to paste");
      return std::nullopt;
    }
  if (
    clipboard_payload->type ()
    != structure::project::ClipboardPayload::Type::ArrangerObjects)
    {
      z_warning ("The clipboard does not contain arranger objects");
      refuse_operation (
        QObject::tr ("The clipboard contents could not be pasted"));
      return std::nullopt;
    }

  // Arranger-object pastes cannot attach plugins or tracks: refuse
  // payloads that carry any, or their objects would stay unowned
  // (import_into()'s instantiation-wait contract is the caller's job)
  {
    const auto &registry = clipboard_payload->registry_json ();
    for (
      const auto bucket_key :
      { structure::project::ProjectRegistry::kPluginsKey,
        structure::project::ProjectRegistry::kTracksKey })
      {
        const auto bucket_it = registry.find (bucket_key);
        if (bucket_it != registry.end () && !bucket_it->empty ())
          {
            z_warning (
              "The clipboard payload contains {} and cannot be pasted as "
              "arranger objects",
              bucket_key);
            refuse_operation (
              QObject::tr ("The clipboard contents could not be pasted"));
            return std::nullopt;
          }
      }
  }

  PreparedPaste prepared;
  try
    {
      auto filtered = clipboard_payload->filtered_for_target (project_registry_);
      if (!filtered.payload.has_value ())
        {
          z_warning (
            "The clipboard contents cannot be pasted into this project");
          refuse_operation (
            QObject::tr ("The clipboard contents could not be pasted"));
          return std::nullopt;
        }
      prepared.dropped_audio_objects = filtered.dropped_audio_objects;
      prepared.severed_references = filtered.severed_references;

      prepared.payload = structure::project::ClipboardPayload::
        with_regenerated_uuids (std::move (*filtered.payload));
      const auto anchor = units::ticks (prepared.payload.metadata ().value (
        structure::project::ClipboardPayload::kAnchorTicksMetadataKey, 0.0));
      prepared.delta = position - anchor;
      if (
        prepared.payload.metadata ().contains (
          structure::project::ClipboardPayload::kLaneIndicesMetadataKey))
        {
          for (
            const auto &[key, value] :
            prepared.payload.metadata ()
              .at (structure::project::ClipboardPayload::kLaneIndicesMetadataKey)
              .items ())
            {
              if (value.is_number_unsigned ())
                {
                  prepared.lane_indices.emplace (
                    structure::arrangement::ArrangerObjectUuid (
                      QUuid::fromString (QString::fromStdString (key))),
                    value.get<std::size_t> ());
                }
            }
        }
      prepared.imported_ids = prepared.payload.import_into (project_registry_);
    }
  catch (const std::exception &e)
    {
      z_warning (
        "Failed to prepare the clipboard contents for pasting: {}", e.what ());
      refuse_operation (
        QObject::tr ("The clipboard contents could not be pasted"));
      return std::nullopt;
    }
  return prepared;
}

bool
ArrangerObjectSelectionOperator::selection_in_single_position_space (
  const SelectedObjectsVector &objects)
{
  bool saw_timeline_object = false;
  bool saw_editor_object = false;
  for (const auto &obj_ref : objects)
    {
      auto obj_var = utils::convert_to_variant_qobj<
        structure::arrangement::ArrangerObjectPtrVariant> (obj_ref.get ());
      std::visit (
        [&] (const auto * obj) {
          using ObjectT = utils::base_type<decltype (obj)>;
          if constexpr (structure::arrangement::TimelineObject<ObjectT>)
            saw_timeline_object = true;
          else if constexpr (structure::arrangement::EditorObject<ObjectT>)
            saw_editor_object = true;
        },
        obj_var);
    }
  return !saw_timeline_object || !saw_editor_object;
}

std::vector<units::precise_tick_t>
ArrangerObjectSelectionOperator::object_positions (
  const SelectedObjectsVector &objects)
{
  return objects | std::views::transform ([] (const auto &obj_ref) {
           auto obj_var = utils::convert_to_variant_qobj<
             structure::arrangement::ArrangerObjectPtrVariant> (obj_ref.get ());
           return std::visit (
             [] (const auto * obj) {
               return obj->position ()->asTick ().asQuantity ();
             },
             obj_var);
         })
         | std::ranges::to<std::vector> ();
}

units::precise_tick_t
ArrangerObjectSelectionOperator::object_end_ticks (
  structure::arrangement::ArrangerObjectPtrVariant obj_var)
{
  return std::visit (
    [] (const auto * obj) -> units::precise_tick_t {
      using ObjectT = utils::base_type<decltype (obj)>;
      if constexpr (structure::arrangement::ClipObject<ObjectT>)
        {
          return structure::arrangement::timeline_end_ticks (*obj).asQuantity ();
        }
      else if constexpr (structure::arrangement::BoundedObject<ObjectT>)
        {
          return (obj->position ()->asTick () + obj->length ()->asTick ())
            .asQuantity ();
        }
      else
        {
          return obj->position ()->asTick ().asQuantity ();
        }
    },
    obj_var);
}

bool
ArrangerObjectSelectionOperator::toggleMute (
  QItemSelectionModel * selectionModel)
{
  auto selected_objects = extractSelectedObjects (selectionModel);
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
  QItemSelectionModel *          selectionModel,
  dsp::StretchOptions::Algorithm algorithm)
{
  auto selected_objects = extractSelectedObjects (selectionModel);
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
ArrangerObjectSelectionOperator::setTimebaseOverride (
  QItemSelectionModel * selectionModel,
  dsp::Timebase         timebase)
{
  auto selected_objects = extractSelectedObjects (selectionModel);
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
ArrangerObjectSelectionOperator::clearTimebaseOverride (
  QItemSelectionModel * selectionModel)
{
  auto selected_objects = extractSelectedObjects (selectionModel);
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
ArrangerObjectSelectionOperator::selectionHasTimebaseProviders (
  QItemSelectionModel * selectionModel) const
{
  auto selected_objects = extractSelectedObjects (selectionModel);
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
ArrangerObjectSelectionOperator::moveAutomationPointsByDelta (
  QItemSelectionModel * selectionModel,
  double                delta)
{
  return process_vertical_move (selectionModel, delta);
}

bool
ArrangerObjectSelectionOperator::process_vertical_move (
  QItemSelectionModel * selectionModel,
  double                delta)
{

  if (utils::math::floats_equal (delta, 0.0))
    {
      // No movement needed
      return true;
    }

  // Extract selected objects from selection model
  auto selected_objects = extractSelectedObjects (selectionModel);
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
  QItemSelectionModel *     selectionModel,
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
  auto selected_objects = extractSelectedObjects (selectionModel);
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
ArrangerObjectSelectionOperator::extractSelectedObjects (
  const QItemSelectionModel * selectionModel) -> SelectedObjectsVector
{
  SelectedObjectsVector objects;

  if (selectionModel == nullptr)
    {
      z_debug ("No selection model given; selection is empty");
      return objects;
    }
  const auto selected_indexes = selectionModel->selectedIndexes ();
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
ArrangerObjectSelectionOperator::time_signature_lands_on_bar (
  structure::arrangement::ArrangerObjectPtrVariant obj_var,
  dsp::TimelineTick                                shift)
{
  return std::visit (
    [&] (const auto * obj) {
      using ObjectT = utils::base_type<decltype (obj)>;
      if constexpr (
        std::is_same_v<ObjectT, structure::arrangement::TimeSignatureObject>)
        {
          const auto new_timeline =
            structure::arrangement::timeline_ticks (*obj) + shift;
          const auto &tempo_map = obj->get_tempo_map ();
          const auto  musical_pos = tempo_map.tick_to_musical_position (
            au::round_as<int64_t> (units::ticks, new_timeline.asQuantity ()));
          return musical_pos.beat == 1 && musical_pos.sixteenth == 1
                 && musical_pos.tick == 0;
        }
      return true;
    },
    obj_var);
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
