// SPDX-FileCopyrightText: © 2020-2022, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "utils/rt_thread_id.h"
#include "gui/backend/backend/actions/range_action.h"
#include "gui/backend/backend/project.h"
#include "gui/backend/backend/zrythm.h"

using namespace zrythm::gui::actions;

RangeAction::RangeAction (QObject * parent)
    : QObject (parent), UndoableAction (UndoableAction::Type::Range)
{
}

RangeAction::
  RangeAction (Type type, Position start_pos, Position end_pos, QObject * parent)
    : RangeAction (parent)
{
  start_pos_ = start_pos;
  end_pos_ = end_pos;
  type_ = type;
  first_run_ = true;

  z_return_if_fail (start_pos.validate () && end_pos.validate ());

  /* create selections for overlapping objects */
  Position inf;
  inf.set_to_bar (Position::POSITION_MAX_BAR, TRANSPORT->ticks_per_bar_, AUDIO_ENGINE->frames_per_tick_);
  affected_objects_before_ = std::ranges::to<std::vector> (
    TRACKLIST->get_timeline_objects_in_range (std::make_pair (start_pos, inf))
    | std::views::transform (ArrangerObjectSpan::uuid_projection));

  transport_ = TRANSPORT->clone_unique ();
}

void
RangeAction::init_after_cloning (
  const RangeAction &other,
  ObjectCloneType    clone_type)
{
  UndoableAction::copy_members_from (other, clone_type);
  start_pos_ = other.start_pos_;
  end_pos_ = other.end_pos_;
  type_ = other.type_;
  affected_objects_before_ = other.affected_objects_before_;
  objects_removed_ = other.objects_removed_;
  objects_added_ = other.objects_added_;
  objects_moved_ = other.objects_moved_;
  transport_ = other.transport_->clone_unique ();
  first_run_ = other.first_run_;
}

ArrangerObjectRegistrySpan
RangeAction::get_before_objects () const
{
  return ArrangerObjectRegistrySpan{
    PROJECT->get_arranger_object_registry (), affected_objects_before_
  };
}

bool
RangeAction::contains_clip (const AudioClip &clip) const
{
  return get_before_objects ().contains_clip (clip)
         || ArrangerObjectRegistrySpan{ PROJECT->get_arranger_object_registry (),
                                        objects_added_ }
              .contains_clip (clip);
}

void
RangeAction::init_loaded_impl ()
{
  get_before_objects ().init_loaded (false, frames_per_tick_);
  ArrangerObjectRegistrySpan{
    PROJECT->get_arranger_object_registry (), objects_added_
  }
    .init_loaded (false, frames_per_tick_);
  transport_->init_loaded (nullptr, nullptr);
}

#define MOVE_TRANSPORT_MARKER(_action, x, _ticks, _do) \
  if ( \
    _action.type_ == RangeAction::Type::Remove \
    && *TRANSPORT->x >= action.start_pos_ && *TRANSPORT->x <= action.end_pos_) \
    { \
      /* move position to range start or back to  original pos */ \
      if (_do) \
        { \
          TRANSPORT->x->set_position_rtsafe (action.start_pos_); \
        } \
      else \
        { \
          TRANSPORT->x->set_position_rtsafe (*action.transport_->x); \
        } \
    } \
  else if (*TRANSPORT->x >= action.start_pos_) \
    { \
      TRANSPORT->x->addTicks (_ticks); \
    }

constexpr auto MOVE_TRANSPORT_MARKERS = [] (RangeAction &action, bool do_it) {
  const auto range_size_ticks = action.get_range_size_in_ticks ();
  MOVE_TRANSPORT_MARKER (action, playhead_pos_, range_size_ticks, do_it);
  MOVE_TRANSPORT_MARKER (action, cue_pos_, range_size_ticks, do_it);
  MOVE_TRANSPORT_MARKER (action, loop_start_pos_, range_size_ticks, do_it);
  MOVE_TRANSPORT_MARKER (action, loop_end_pos_, range_size_ticks, do_it);
};

constexpr auto UNMOVE_TRANSPORT_MARKERS = [] (RangeAction &action, bool do_it) {
  const auto range_size_ticks = action.get_range_size_in_ticks ();
  MOVE_TRANSPORT_MARKER (action, playhead_pos_, -range_size_ticks, do_it);
  MOVE_TRANSPORT_MARKER (action, cue_pos_, -range_size_ticks, do_it);
  MOVE_TRANSPORT_MARKER (action, loop_start_pos_, -range_size_ticks, do_it);
  MOVE_TRANSPORT_MARKER (action, loop_end_pos_, -range_size_ticks, do_it);
};

void
RangeAction::perform_impl ()
{
  /* sort the selections in ascending order FIXME: needed? */
  // sel_before_->sort_by_indices (false);
  // sel_after_->sort_by_indices (false);

  const auto range_size_ticks = get_range_size_in_ticks ();

  /* returns whether the object is hit (starts before or at and ends after) at
   * the given position (and thus we need to split the object at the range start
   * position) */
  const auto need_to_split_object_at_pos =
    [] (auto &prj_obj, const auto &start_pos) {
      using ObjT = base_type<decltype (prj_obj)>;
      if constexpr (
        std::derived_from<ObjT, BoundedObject>
        && std::derived_from<ObjT, TimelineObject>)
        {
          return prj_obj.is_hit (start_pos);
        }
      return false;
    };

  constexpr auto handle_not_first_run = [&] () {
    // TODO:
    // remove objects that should be removed

    // move objects that should be moved

    // add objects that should be added
  };

  if (!first_run_)
    {
      handle_not_first_run ();
    }
  else
    {
      switch (type_)
        {
        case Type::InsertSilence:
          for (
            const auto &obj_var :
            ArrangerObjectRegistrySpan{
              PROJECT->get_arranger_object_registry (), affected_objects_before_ }
              | std::views::reverse)
            {
              std::visit (
                [&] (auto &&obj) {
                  using ObjT = base_type<decltype (obj)>;
                  z_debug ("looping backwards. current object {}:", *obj);

                  /* if need split, split at range start */
                  if (need_to_split_object_at_pos (*obj, start_pos_))
                    {
                      if constexpr (std::derived_from<ObjT, BoundedObject>)
                        {
                          /* split at range start */
                          auto [part1, part2] =
                            ArrangerObjectSpan::split_bounded_object (
                              *obj, *ArrangerObjectFactory::get_instance (),
                              start_pos_, AUDIO_ENGINE->frames_per_tick_);

                          /* move part2 by the range amount */
                          std::get<ObjT *> (part2.get_object ())
                            ->move (range_size_ticks);

                          /* remove previous object */
                          obj->remove_from_project (true);
                          objects_removed_.push_back (obj->get_uuid ());

                          // TODO: add to project
                          // part1->add_to_project ();
                          // part2->add_to_project ();
                          objects_added_.push_back (part1.id ());
                          objects_added_.push_back (part2.id ());
                        }
                      else
                        {
                          z_return_if_reached ();
                        }
                    }
                  /* object starts at or after range start - only needs a move */
                  else
                    {
                      obj->move (range_size_ticks);
                      objects_moved_.push_back (obj->get_uuid ());
                      z_debug ("moved to object: {}", *obj);
                    }
                },
                obj_var);
            }
      break;
        case Type::Remove:
          if (first_run_)
            {
              for (
                const auto &obj_var :
                get_before_objects () | std::views::reverse)
                {
                  std::visit (
                    [&] (auto &&obj) {
                      using ObjT = base_type<decltype (obj)>;
                      z_debug ("looping backwards. current object {}", *obj);

                      bool ends_inside_range = false;
                      if constexpr (std::derived_from<ObjT, BoundedObject>)
                        {
                          ends_inside_range =
                            *obj->pos_ >= start_pos_
                            && *obj->end_pos_ < end_pos_;
                        }
                      else
                        {
                          ends_inside_range =
                            *obj->pos_ >= start_pos_ && *obj->pos_ < end_pos_;
                        }

                      /* object starts before the range and ends after the range
                       * start - split at range start */
                      if (need_to_split_object_at_pos (*obj, start_pos_))
                        {
                          if constexpr (std::derived_from<ObjT, BoundedObject>)
                            {
                              /* split at range start */
                              auto [part1_ref, part2_ref] =
                                ArrangerObjectSpan::split_bounded_object (
                                  *obj, *ArrangerObjectFactory::get_instance (),
                                  start_pos_, AUDIO_ENGINE->frames_per_tick_);

                              auto part2_opt = std::make_optional (part2_ref);

                              /* if part 2 extends beyond the range end, split
                               * it and remove the part before range end */
                              if (
                                need_to_split_object_at_pos (
                                  *std::get<ObjT *> (part2_ref.get_object ()),
                                  end_pos_))
                                {
                                  // part3 will be discared
                                  auto [part3, part4] =
                                    ArrangerObjectSpan::split_bounded_object (
                                      *std::get<ObjT *> (part2_ref.get_object ()),
                                      *ArrangerObjectFactory::get_instance (),
                                      end_pos_, AUDIO_ENGINE->frames_per_tick_);
// TODO...
#if 0
                                      PROJECT->get_arranger_object_registry ()
                                    .delete_object_by_id (part3->get_uuid ());
                                  PROJECT->get_arranger_object_registry ()
                                    .delete_object_by_id (part2->get_uuid ());
#endif
                                  part2_ref = part4;
                                }
                              /* otherwise remove the whole part2 */
                              else
                                {
// TODO
#if 0
                                  PROJECT->get_arranger_object_registry ()
                                    .delete_object_by_id (part2->get_uuid ());
#endif
                                  part2_opt.reset ();
                                }

                              /* if a part2 exists, move it back */
                              if (part2_opt)
                                {
                                  std::get<ObjT *> (part2_ref.get_object ())
                                    ->move (-range_size_ticks);
                                }

                              /* remove previous object */
                              // TODO: actually remove from project
                              objects_removed_.push_back (obj->get_uuid ());

                              // add new object(s) to project
                              // TODO: actually add to project
                              objects_added_.push_back (part1_ref.id ());

                              if (part2_opt)
                                {
                                  // TODO: actually add to project
                                  objects_added_.push_back (part2_ref.id ());
                                }
                            }
                        }
                      /* object starts before the range end and ends after the
                       * range end
                       * - split at range end */
                      else if (need_to_split_object_at_pos (*obj, end_pos_))
                        {
                          if constexpr (std::derived_from<ObjT, BoundedObject>)
                            {
                              /* split at range end */
                              // part1 will be discarded
                              auto [part1, part2] =
                                ArrangerObjectSpan::split_bounded_object (
                                  *obj, *ArrangerObjectFactory::get_instance (),
                                  end_pos_, AUDIO_ENGINE->frames_per_tick_);
// TODO
#if 0
                                  PROJECT->get_arranger_object_registry ()
                                .delete_object_by_id (part1->get_uuid ());
#endif

                              // move part2 by the range amount and add to
                              // project
                              // TODO: actually add
                              std::get<ObjT *> (part2.get_object ())
                                ->move (-range_size_ticks);
                              objects_added_.push_back (part2.id ());

                              // TODO: actually remove object
                              objects_removed_.push_back (obj->get_uuid ());
                            }
                        }
                      /* if object starts and ends inside the range and is
                       * deletable, delete */
                      else if (ends_inside_range && obj->is_deletable ())
                        {
                          // TODO: actually remove
                          objects_removed_.push_back (obj->get_uuid ());
                        }
                      /* object starts at or after range start - only needs a
                       * move */
                      else
                        {
                          obj->move (-range_size_ticks);
                          objects_moved_.push_back (obj->get_uuid ());

                          /* move objects to bar 1 if negative pos */
                          const Position init_pos;
                          if (*obj->pos_ < init_pos)
                            {
                              z_debug ("moving object back");
                              obj->move (-obj->pos_->ticks_);
                            }

                          ArrangerObjectFactory::get_instance ()
                            ->get_selection_manager_for_object (*obj)
                            .append_to_selection (obj->get_uuid ());
                        }
                    },
                    obj_var);
                }
            }
          break;
        default:
          break;
        }
    } // endif not first run

  switch (type_)
    {
    case Type::InsertSilence:
      MOVE_TRANSPORT_MARKERS (*this, true);
      break;
    case Type::Remove:
      UNMOVE_TRANSPORT_MARKERS (*this, true);
      break;
    }

  first_run_ = false;
  /* EVENTS_PUSH (EventType::ET_ARRANGER_SELECTIONS_ACTION_FINISHED, nullptr); */
  /* EVENTS_PUSH (EventType::ET_PLAYHEAD_POS_CHANGED_MANUALLY, nullptr); */
}

void
RangeAction::undo_impl ()
{
  /* sort the selections in ascending order */
  // sel_before_->sort_by_indices (false);
  // sel_after_->sort_by_indices (false);

  const auto range_size_ticks = get_range_size_in_ticks ();

  /* remove all objects added during perform() */
  for (
    const auto &obj_var : std::ranges::reverse_view (ArrangerObjectRegistrySpan{
      PROJECT->get_arranger_object_registry (), objects_added_ }))
    {
      std::visit (
        [&] (auto &&obj) { obj->remove_from_project (true); }, obj_var);
    }

  // move all moved objects backwards
  for (
    const auto &obj_var : std::ranges::reverse_view (ArrangerObjectRegistrySpan{
      PROJECT->get_arranger_object_registry (), objects_moved_ }))
    {
      std::visit ([&] (auto &&obj) { obj->move (-range_size_ticks); }, obj_var);
    }
  // add back objects taht were removed
  for (
    const auto &obj_var : ArrangerObjectRegistrySpan{
      PROJECT->get_arranger_object_registry (), objects_removed_ })
    {
      std::visit (
        [&] (auto &&obj) {
          // TODO: add to project
        },
        obj_var);
    }

  /* move transport markers */
  switch (type_)
    {
    case Type::InsertSilence:
      UNMOVE_TRANSPORT_MARKERS (*this, false);
      break;
    case Type::Remove:
      MOVE_TRANSPORT_MARKERS (*this, false);
      break;
    default:
      break;
    }

  /* EVENTS_PUSH (EventType::ET_ARRANGER_SELECTIONS_ACTION_FINISHED, nullptr); */
  /* EVENTS_PUSH (EventType::ET_PLAYHEAD_POS_CHANGED_MANUALLY, nullptr); */
}

QString
RangeAction::to_string () const
{
  switch (type_)
    {
    case Type::InsertSilence:
      return QObject::tr ("Insert silence");
    case Type::Remove:
      return QObject::tr ("Delete range");
    default:
      z_return_val_if_reached ({});
    }
}

#undef _MOVE_TRANSPORT_MARKER
