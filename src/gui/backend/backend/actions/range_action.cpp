// SPDX-FileCopyrightText: © 2020-2022, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "common/utils/rt_thread_id.h"
#include "gui/backend/backend/actions/range_action.h"
#include "gui/backend/backend/project.h"
#include "gui/backend/backend/zrythm.h"

#include <glib/gi18n.h>

RangeAction::RangeAction () : UndoableAction (UndoableAction::Type::Range) { }

void
RangeAction::init_after_cloning (const RangeAction &other)
{
  UndoableAction::copy_members_from (other);
  start_pos_ = other.start_pos_;
  end_pos_ = other.end_pos_;
  type_ = other.type_;
  sel_before_ = other.sel_before_->clone_unique ();
  sel_after_ = other.sel_after_->clone_unique ();
  transport_ = other.transport_->clone_unique ();
  first_run_ = other.first_run_;
}

RangeAction::RangeAction (Type type, Position start_pos, Position end_pos)
    : UndoableAction (UndoableAction::Type::Range), start_pos_ (start_pos),
      end_pos_ (end_pos), type_ (type), first_run_ (true)
{
  z_return_if_fail (start_pos.validate () && end_pos.validate ());

  /* create selections for overlapping objects */
  Position inf;
  inf.set_to_bar (*TRANSPORT, POSITION_MAX_BAR);
  sel_before_ = std::make_unique<TimelineSelections> (start_pos, inf);
  sel_after_ = std::make_unique<TimelineSelections> ();

  transport_ = TRANSPORT->clone_unique ();
}

#define _MOVE_TRANSPORT_MARKER(x, _ticks, _do) \
  if ( \
    type_ == RangeAction::Type::Remove && *TRANSPORT->x >= start_pos_ \
    && *TRANSPORT->x <= end_pos_) \
    { \
      /* move position to range start or back to  original pos */ \
      if (_do) \
        { \
          TRANSPORT->x->set_position_rtsafe (start_pos_); \
        } \
      else \
        { \
          TRANSPORT->x->set_position_rtsafe (*transport_->x); \
        } \
    } \
  else if (*TRANSPORT->x >= start_pos_) \
    { \
      TRANSPORT->x->addTicks (_ticks); \
    }

#define MOVE_TRANSPORT_MARKER(x, _do) \
  _MOVE_TRANSPORT_MARKER (x, range_size_ticks, _do)

#define UNMOVE_TRANSPORT_MARKER(x, _do) \
  _MOVE_TRANSPORT_MARKER (x, -range_size_ticks, _do)

#define MOVE_TRANSPORT_MARKERS(_do) \
  MOVE_TRANSPORT_MARKER (playhead_pos_, _do); \
  MOVE_TRANSPORT_MARKER (cue_pos_, _do); \
  MOVE_TRANSPORT_MARKER (loop_start_pos_, _do); \
  MOVE_TRANSPORT_MARKER (loop_end_pos_, _do)

#define UNMOVE_TRANSPORT_MARKERS(_do) \
  UNMOVE_TRANSPORT_MARKER (playhead_pos_, _do); \
  UNMOVE_TRANSPORT_MARKER (cue_pos_, _do); \
  UNMOVE_TRANSPORT_MARKER (loop_start_pos_, _do); \
  UNMOVE_TRANSPORT_MARKER (loop_end_pos_, _do)

void
RangeAction::add_to_sel_after (
  std::vector<ArrangerObject *>                &prj_objs,
  std::vector<std::shared_ptr<ArrangerObject>> &after_objs_for_prj,
  ArrangerObject                               &prj_obj,
  std::shared_ptr<ArrangerObject>             &&after_obj)
{
  prj_objs.push_back (&prj_obj);
  z_debug ("adding to sel after (num prj objs {})", prj_objs.size ());
  prj_obj.print ();
  after_objs_for_prj.emplace_back (std::move (after_obj));
  sel_after_->add_object_owned (clone_unique_with_variant<ArrangerObjectVariant> (
    after_objs_for_prj.back ().get ()));
}

void
RangeAction::perform_impl ()
{
  /* sort the selections in ascending order */
  sel_before_->sort_by_indices (false);
  sel_after_->sort_by_indices (false);

  auto  &before_objs_arr = sel_before_->objects_;
  auto  &after_objs_arr = sel_after_->objects_;
  double range_size_ticks = end_pos_.ticks_ - start_pos_.ticks_;

  /* temporary place to store project objects, so we can get their final
   * identifiers at the end */
  std::vector<ArrangerObject *> prj_objs;
  prj_objs.reserve (before_objs_arr.size () * 2);

  /* after objects corresponding to the above */
  std::vector<std::shared_ptr<ArrangerObject>> after_objs_for_prj;
  after_objs_for_prj.reserve (before_objs_arr.size () * 2);

  auto add_after =
    [&] (ArrangerObject &prj_obj, std::shared_ptr<ArrangerObject> &&after_obj) {
      add_to_sel_after (
        prj_objs, after_objs_for_prj, prj_obj, std::move (after_obj));
    };

  /* returns whether the object is hit (starts before or at and ends after) at
   * the given position (and thus we need to split the object at the range start
   * position) */
  auto need_split_at_pos = [] (ArrangerObject &prj_obj, Position start_pos) {
    auto prj_obj_lo = dynamic_cast<LengthableObject *> (&prj_obj);
    return prj_obj_lo && prj_obj_lo->is_hit (start_pos);
  };

  switch (type_)
    {
    case Type::InsertSilence:
      if (first_run_)
        {
          for (auto &obj : std::ranges::reverse_view (before_objs_arr))
            {
              z_debug (
                "looping backwards. current object {}:", obj->print_to_str ());

              obj->flags_ |= ArrangerObject::Flags::NonProject;

              /* get project object */
              auto prj_obj = obj->find_in_project ();

              /* if need split, split at range start */
              if (need_split_at_pos (*prj_obj, start_pos_))
                {
                  std::visit (
                    [&] (auto &&prj_obj_ptr) {
                      using T = base_type<decltype (prj_obj_ptr)>;
                      auto lo_prj_obj = dynamic_pointer_cast<T> (prj_obj);

                      /* split at range start */
                      auto [part1, part2] = LengthableObject::split (
                        *lo_prj_obj, start_pos_, false, false);

                      /* move part2 by the range amount */
                      part2->move (range_size_ticks);

                      /* remove previous object */
                      prj_obj->remove_from_project ();

                      /* create clones and add to project */
                      auto prj_part1 = part1->add_clone_to_project (false);
                      auto prj_part2 = part2->add_clone_to_project (false);

                      z_debug (
                        "object split and moved into the following objects");
                      prj_part1->print ();
                      prj_part2->print ();

                      add_after (*prj_part1, std::move (part1));
                      add_after (*prj_part2, std::move (part2));
                    },
                    convert_to_variant<LengthableObjectPtrVariant> (
                      prj_obj.get ()));
                }
              /* object starts at or after range
               * start - only needs a move */
              else
                {
                  prj_obj->move (range_size_ticks);

                  z_debug ("moved to object:");
                  prj_obj->print ();

                  /* clone and add to sel_after */
                  add_after (
                    *prj_obj,
                    clone_shared_with_variant<ArrangerObjectVariant> (
                      prj_obj.get ()));
                }
            }
          first_run_ = false;
        }
      else /* not first run */
        {
          /* remove all matching project objects from sel_before_ */
          for (auto &obj : std::ranges::reverse_view (before_objs_arr))
            {
              auto prj_obj = obj->find_in_project ();
              prj_obj->remove_from_project ();
            }
          /* insert clones of all objects from sel_after_ to the project */
          for (auto &obj : after_objs_arr)
            {
              obj->insert_clone_to_project ();
            }
        }

      /* move transport markers */
      MOVE_TRANSPORT_MARKERS (true);
      break;
    case Type::Remove:
      if (first_run_)
        {
          for (auto &obj : std::ranges::reverse_view (before_objs_arr))
            {
              z_debug (
                "looping backwards. current object {}", obj->print_to_str ());

              obj->flags_ |= ArrangerObject::Flags::NonProject;

              /* get project object */
              auto prj_obj = obj->find_in_project ();

              bool ends_inside_range = false;
              if (prj_obj->has_length ())
                {
                  auto prj_lo_obj =
                    dynamic_pointer_cast<LengthableObject> (prj_obj);
                  ends_inside_range =
                    prj_obj->pos_ >= start_pos_
                    && prj_lo_obj->end_pos_ < end_pos_;
                }
              else
                {
                  ends_inside_range =
                    prj_obj->pos_ >= start_pos_ && prj_obj->pos_ < end_pos_;
                }

              /* object starts before the range and ends after the range start -
               * split at range start */
              if (need_split_at_pos (*prj_obj, start_pos_))
                {
                  std::visit (
                    [&] (auto &&prj_obj_ptr) {
                      using T = base_type<decltype (prj_obj_ptr)>;
                      auto lo_obj = dynamic_cast<T *> (obj.get ());

                      /* split at range start */
                      auto [part1, part2] = LengthableObject::split (
                        *lo_obj, start_pos_, false, false);

                      /* if part 2 extends beyond the range end, split it and
                       * remove the part before range end */
                      if (need_split_at_pos (*part2, end_pos_))
                        {
                          // part3 will be discared
                          auto [part3, part4] = LengthableObject::split (
                            *part2, end_pos_, false, false);
                          part2.reset ();
                          part2 = std::move (part4);
                        }
                      /* otherwise remove the whole part2 */
                      else
                        {
                          part2.reset ();
                        }

                      /* if a part2 exists, move it back */
                      if (part2)
                        {
                          part2->move (-range_size_ticks);
                        }

                      /* remove previous object */
                      prj_obj->remove_from_project ();

                      /* create clones and add to project */
                      auto prj_part1 = part1->add_clone_to_project (false);
                      add_after (*prj_part1, std::move (part1));

                      if (part2)
                        {
                          auto prj_part2 = part2->add_clone_to_project (false);
                          z_debug ("object split to the following:");
                          prj_part1->print ();
                          prj_part2->print ();

                          add_after (*prj_part2, std::move (part2));
                        }
                      else
                        {
                          z_debug ("object split to just:");
                          prj_part1->print ();
                        }
                    },
                    convert_to_variant<LengthableObjectPtrVariant> (
                      prj_obj.get ()));
                }
              /* object starts before the range end and ends after the range end
               * - split at range end */
              else if (need_split_at_pos (*prj_obj, end_pos_))
                {
                  std::visit (
                    [&] (auto &&prj_obj_ptr) {
                      using T = base_type<decltype (prj_obj_ptr)>;

                      /* split at range end */
                      auto lo_obj = dynamic_cast<T *> (obj.get ());
                      // part1 will be discarded
                      auto [part1, part2] = LengthableObject::split (
                        *lo_obj, end_pos_, false, false);

                      /* move part2 by the range amount */
                      part2->move (-range_size_ticks);

                      /* remove previous object */
                      prj_obj->remove_from_project ();

                      /* create clones and add to project */
                      auto prj_part2 = part2->add_clone_to_project (false);
                      add_after (*prj_part2, std::move (part2));

                      z_debug ("object split to just:");
                      prj_part2->print ();
                    },
                    convert_to_variant<LengthableObjectPtrVariant> (
                      prj_obj.get ()));
                }
              /* object starts and ends inside range and not marker start/end -
               * delete */
              else if (ends_inside_range 
              && !(prj_obj->is_marker() && (dynamic_cast<Marker&>(*prj_obj).marker_type_ == Marker::Type::Start || dynamic_cast<Marker&>(*prj_obj).marker_type_ == Marker::Type::End)))
                {
                  z_debug ("removing object:");
                  prj_obj->print ();
                  prj_obj->remove_from_project ();
                }
              /* object starts at or after range start - only needs a move */
              else
                {
                  prj_obj->move (-range_size_ticks);

                  /* move objects to bar 1 if negative pos */
                  Position init_pos;
                  if (prj_obj->pos_ < init_pos)
                    {
                      z_debug ("moving object back");
                      prj_obj->move (-prj_obj->pos_.ticks_);
                    }

                  /* clone and add to sel_after */
                  add_after (
                    *prj_obj,
                    clone_shared_with_variant<ArrangerObjectVariant> (
                      prj_obj.get ()));
                }
            }
          first_run_ = false;
        }
      else /* not first run */
           /* remove all matching project objects from sel_before_ */
        for (auto &obj : std::ranges::reverse_view (before_objs_arr))
          {
            auto prj_obj = obj->find_in_project ();
            prj_obj->remove_from_project ();
          }
      /* insert clones of all objects from sel_after_ to the project */
      for (auto &obj : after_objs_arr)
        {
          obj->insert_clone_to_project ();
        }

      /* move transport markers */
      UNMOVE_TRANSPORT_MARKERS (true);
      break;
    default:
      break;
    }

  for (size_t i = 0; i < prj_objs.size (); ++i)
    {
      z_debug ("copying {}", i);
      prj_objs[i]->print ();
      after_objs_for_prj[i]->copy_identifier (*prj_objs[i]);
    }

  /* EVENTS_PUSH (EventType::ET_ARRANGER_SELECTIONS_ACTION_FINISHED, nullptr); */
  /* EVENTS_PUSH (EventType::ET_PLAYHEAD_POS_CHANGED_MANUALLY, nullptr); */
}

void
RangeAction::undo_impl ()
{
  /* sort the selections in ascending order */
  sel_before_->sort_by_indices (false);
  sel_after_->sort_by_indices (false);

  auto  &before_objs_arr = sel_before_->objects_;
  auto  &after_objs_arr = sel_after_->objects_;
  double range_size_ticks = end_pos_.ticks_ - start_pos_.ticks_;

  /* remove all matching project objects from sel_after */
  for (auto &obj : std::ranges::reverse_view (after_objs_arr))
    {
      /* get project object and remove it from the project */
      auto prj_obj = obj->find_in_project ();
      prj_obj->remove_from_project ();

      z_debug ("removing");
      obj->print ();
    }
  /* add all objects from sel_before */
  for (auto &obj : before_objs_arr)
    {
      /* clone object and add to project */
      auto prj_obj = obj->insert_clone_to_project ();

      z_debug ("adding");
      obj->print ();
      z_debug ("after adding");
      prj_obj->print ();
    }

  /* move transport markers */
  switch (type_)
    {
    case Type::InsertSilence:
      UNMOVE_TRANSPORT_MARKERS (false);
      break;
    case Type::Remove:
      MOVE_TRANSPORT_MARKERS (false);
      break;
    default:
      break;
    }

  /* EVENTS_PUSH (EventType::ET_ARRANGER_SELECTIONS_ACTION_FINISHED, nullptr); */
  /* EVENTS_PUSH (EventType::ET_PLAYHEAD_POS_CHANGED_MANUALLY, nullptr); */
}

std::string
RangeAction::to_string () const
{
  switch (type_)
    {
    case Type::InsertSilence:
      return _ ("Insert silence");
    case Type::Remove:
      return _ ("Delete range");
    default:
      z_return_val_if_reached ("");
    }
}