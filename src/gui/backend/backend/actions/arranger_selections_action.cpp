// SPDX-FileCopyrightText: © 2019-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <ranges>

# include "gui/dsp/audio_region.h"
# include "gui/dsp/automation_region.h"
# include "gui/dsp/automation_track.h"
# include "gui/dsp/chord_track.h"
# include "gui/dsp/control_port.h"
# include "gui/dsp/laned_track.h"
# include "gui/dsp/marker_track.h"
# include "gui/dsp/port_identifier.h"
# include "gui/dsp/router.h"
# include "gui/dsp/track.h"
# include "gui/dsp/tracklist.h"
#include "utils/gtest_wrapper.h"
#include "utils/math.h"
#include "gui/backend/backend/actions/arranger_selections_action.h"
#include "gui/backend/backend/arranger_selections.h"
#include "gui/backend/backend/project.h"
#include "gui/backend/backend/settings_manager.h"
#include "gui/backend/backend/zrythm.h"

using namespace zrythm::gui::actions;

ArrangerSelectionsAction::ArrangerSelectionsAction ()
    : UndoableAction (UndoableAction::Type::ArrangerSelections)
{
}

template <FinalArrangerSelectionsSubclass T>
CreateOrDeleteArrangerSelectionsAction::CreateOrDeleteArrangerSelectionsAction (
  const T &sel,
  bool     create)
    : ArrangerSelectionsAction (sel, create ? Type::Create : Type::Delete)
{
  z_return_if_fail (sel.has_any ());

  if (sel.contains_undeletable_object ())
    {
      throw ZrythmException (
        QObject::tr ("Arranger selections contain an undeletable object"));
    }
}

void
ArrangerSelectionsAction::init_after_cloning (
  const ArrangerSelectionsAction &other)
{
  UndoableAction::copy_members_from (other);
  type_ = other.type_;
  if (other.sel_)
    {
      std::visit (
        [&] (auto &&sel) {
          auto * clone = sel->clone_raw_ptr ();
          clone->setParent (this);
          sel_ = clone;
        },
        other.sel_.value ());
    }
  if (other.sel_after_)
    {
      std::visit (
        [&] (auto &&sel) {
          auto * clone = sel->clone_raw_ptr ();
          clone->setParent (this);
          sel_after_ = clone;
        },
        other.sel_after_.value ());
    }
  edit_type_ = other.edit_type_;
  ticks_ = other.ticks_;
  delta_tracks_ = other.delta_tracks_;
  delta_lanes_ = other.delta_lanes_;
  delta_chords_ = other.delta_chords_;
  delta_pitch_ = other.delta_pitch_;
  delta_vel_ = other.delta_vel_;
  delta_normalized_amount_ = other.delta_normalized_amount_;
  if (other.target_port_ != nullptr)
    {
      target_port_ = other.target_port_->clone_raw_ptr ();
      target_port_->setParent (this);
    }
  str_ = other.str_;
  pos_ = other.pos_;
  for (const auto &r_var : other.r1_)
    {
      std::visit (
        [&] (auto &&r) {
          auto * clone = r->clone_raw_ptr ();
          clone->setParent (this);
          r1_.push_back (clone);
        },
        r_var);
    }
  for (const auto &r_var : other.r2_)
    {
      std::visit (
        [&] (auto &&r) {
          auto * clone = r->clone_raw_ptr ();
          clone->setParent (this);
          r2_.push_back (clone);
        },
        r_var);
    }
  num_split_objs_ = other.num_split_objs_;
  first_run_ = other.first_run_;
  if (other.opts_)
    opts_ = std::make_unique<dsp::QuantizeOptions> (*other.opts_);
  if (other.region_before_)
    {
      std::visit (
        [&] (auto &&region_before) {
          auto * clone = region_before->clone_raw_ptr ();
          clone->setParent (this);
          region_before_ = clone;
        },
        other.region_before_.value ());
    }
  if (other.region_after_)
    {
      std::visit (
        [&] (auto &&region_after) {
          auto * clone = region_after->clone_raw_ptr ();
          clone->setParent (this);
          region_after_ = clone;
        },
        other.region_after_.value ());
    }
  resize_type_ = other.resize_type_;
}

void
ArrangerSelectionsAction::init_loaded_impl ()
{
  if (sel_)
    {
      std::visit (
        [&] (auto &&sel) { sel->init_loaded (false, frames_per_tick_); },
        sel_.value ());
    };
  if (sel_after_)
    {
      std::visit (
        [&] (auto &&sel) { sel->init_loaded (false, frames_per_tick_); },
        sel_after_.value ());
    }

  z_return_if_fail (num_split_objs_ == r1_.size ());
  z_return_if_fail (num_split_objs_ == r2_.size ());
  for (size_t j = 0; j < num_split_objs_; j++)
    {
      std::visit (
        [&] (auto &&r1, auto &&r2) {
          r1->init_loaded ();
          r2->init_loaded ();
        },
        r1_.at (j), r2_.at (j));
    }

  if (region_before_)
    {
      std::visit (
        [&] (auto &&r) { r->init_loaded (); }, region_before_.value ());
    }
  if (region_after_)
    {
      std::visit ([&] (auto &&r) { r->init_loaded (); }, region_after_.value ());
    }
}

template <FinalArrangerSelectionsSubclass T>
void
ArrangerSelectionsAction::set_before_selections (const T &src)
{
  auto * clone = src.clone_raw_ptr ();
  clone->setParent (this);
  sel_ = clone;

  if (ZRYTHM_TESTING)
    {
      src.validate ();
      clone->validate ();
    }
}

template <FinalArrangerSelectionsSubclass T>
void
ArrangerSelectionsAction::set_after_selections (const T &src)
{
  auto * clone = src.clone_raw_ptr ();
  clone->setParent (this);
  sel_after_ = clone;

  if (ZRYTHM_TESTING)
    {
      src.validate ();
      clone->validate ();
    }
}

bool
ArrangerSelectionsAction::needs_transport_total_bar_update (bool perform) const
{
  if (
    (perform && type_ == Type::Create) || (!perform && type_ == Type::Delete)
    || (perform && type_ == Type::Duplicate) || (perform && type_ == Type::Link))
    return false;

  return true;
}

ArrangerSelectionsPtrVariant
ArrangerSelectionsAction::get_actual_arranger_selections () const
{
  return std::visit (
    [&] (auto &&sel) { return ArrangerSelections::get_for_type (sel->type_); },
    *sel_);
}

template <FinalArrangerSelectionsSubclass T>
ArrangerSelectionsAction::MoveOrDuplicateAction::MoveOrDuplicateAction (
  const T               &sel,
  bool                   move,
  double                 ticks,
  int                    delta_chords,
  int                    delta_pitch,
  int                    delta_tracks,
  int                    delta_lanes,
  double                 delta_normalized_amount,
  const PortIdentifier * tgt_port_id,
  bool                   already_moved)
    : ArrangerSelectionsAction (sel, move ? Type::Move : Type::Duplicate)
{
  z_return_if_fail (sel.has_any ());

  first_run_ = already_moved;
  ticks_ = ticks;
  delta_chords_ = delta_chords;
  delta_pitch_ = delta_pitch;
  delta_tracks_ = delta_tracks;
  delta_lanes_ = delta_lanes;
  delta_normalized_amount_ = delta_normalized_amount;

  /* validate */
  if (!move)
    {
      if (sel.contains_unclonable_object ())
        {
          throw ZrythmException (QObject::tr (
            "Arranger selections contain an object that cannot be duplicated"));
        }
    }

  if (!move)
    {
      set_after_selections (sel);
    }

  if (tgt_port_id)
    {
      target_port_ = tgt_port_id->clone_raw_ptr ();
      target_port_->setParent (this);
    }
}

template <FinalArrangerSelectionsSubclass T>
ArrangerSelectionsAction::LinkAction::LinkAction (
  const T &sel_before,
  const T &sel_after,
  double   ticks,
  int      delta_tracks,
  int      delta_lanes,
  bool     already_moved)
    : ArrangerSelectionsAction (sel_before, Type::Link)
{
  first_run_ = already_moved;
  ticks_ = ticks;
  delta_tracks_ = delta_tracks;
  delta_lanes_ = delta_lanes;

  set_after_selections (sel_after);
}

template <FinalArrangerSelectionsSubclass T>
ArrangerSelectionsAction::RecordAction::
  RecordAction (const T &sel_before, const T &sel_after, bool already_recorded)
    : ArrangerSelectionsAction (sel_before, Type::Record)
{
  first_run_ = !already_recorded;
  set_after_selections (sel_after);
}

template <FinalArrangerSelectionsSubclass T>
EditArrangerSelectionsAction::EditArrangerSelectionsAction (
  const T  &sel_before,
  const T * sel_after,
  EditType  type,
  bool      already_edited)
    : ArrangerSelectionsAction (sel_before, Type::Edit)
{
  first_run_ = already_edited;
  edit_type_ = type;
  if (type == EditType::Name && sel_before.contains_unrenamable_object ())
    {
      throw ZrythmException (QObject::tr ("Cannot rename selected object(s)"));
    }

  set_before_selections (sel_before);

  if (sel_after)
    {
      set_after_selections (*sel_after);
    }
  else
    {
      if (already_edited)
        {
          z_error ("sel_after must be passed or already edited must be false");
        }

      set_after_selections (sel_before);
    }
}

template <FinalArrangerObjectSubclass T>
std::unique_ptr<EditArrangerSelectionsAction>
EditArrangerSelectionsAction::create (
  const T &obj_before,
  const T &obj_after,
  EditType type,
  bool     already_edited)
{
  auto sel_before = obj_before.create_arranger_selections_from_this ();
  auto sel_after = obj_after.create_arranger_selections_from_this ();
  return std::visit (
    [&] (auto &&sel_before_casted, auto &&sel_after_casted)
      -> std::unique_ptr<EditArrangerSelectionsAction> {
      using SelT = base_type<decltype (sel_before_casted)>;
      if constexpr (std::is_same_v<SelT, base_type<decltype (sel_after_casted)>>)
        {
          return std::make_unique<EditArrangerSelectionsAction> (
            *sel_before_casted, sel_after_casted, type, already_edited);
        }
      else
        {
          z_return_val_if_reached (nullptr);
        }
    },
    convert_to_variant<ArrangerSelectionsPtrVariant> (sel_before.get ()),
    convert_to_variant<ArrangerSelectionsPtrVariant> (sel_after.get ()));
}

std::unique_ptr<EditArrangerSelectionsAction>
EditArrangerSelectionsAction::create (
  const MidiSelections &sel_before,
  MidiFunctionType      midi_func_type,
  MidiFunctionOpts      opts)
{
  auto sel_after = sel_before.clone_unique ();

  midi_function_apply (*sel_after, midi_func_type, opts);

  return std::make_unique<EditArrangerSelectionsAction> (
    sel_before, sel_after.get (), EditType::EditorFunction, false);
}

std::unique_ptr<EditArrangerSelectionsAction>
EditArrangerSelectionsAction::create (
  const AutomationSelections &sel_before,
  AutomationFunctionType      automation_func_type)
{
  auto sel_after = sel_before.clone_unique ();

  automation_function_apply (*sel_after, automation_func_type);

  return std::make_unique<EditArrangerSelectionsAction> (
    sel_before, sel_after.get (),
    ArrangerSelectionsAction::EditType::EditorFunction, false);
}

std::unique_ptr<EditArrangerSelectionsAction>
EditArrangerSelectionsAction::create (
  const AudioSelections     &sel_before,
  AudioFunctionType          audio_func_type,
  AudioFunctionOpts          opts,
  std::optional<std::string> uri)
{
  /* prepare selections before */
  auto sel_before_clone = sel_before.clone_unique ();

  z_debug ("saving file before applying audio func...");
  audio_function_apply (
    *sel_before_clone, AudioFunctionType::Invalid, opts, uri);

  auto sel_after = sel_before.clone_unique ();
  z_debug ("applying actual audio func...");
  audio_function_apply (*sel_after, audio_func_type, opts, uri);

  return std::make_unique<EditArrangerSelectionsAction> (
    *sel_before_clone, sel_after.get (), EditType::EditorFunction, false);
}

template <FinalArrangerSelectionsSubclass T>
ArrangerSelectionsAction::SplitAction::SplitAction (const T &sel, Position pos)
    : ArrangerSelectionsAction (sel, Type::Split)
{
  pos_ = pos;
  num_split_objs_ = sel.get_num_objects ();
  pos_.update_frames_from_ticks (0.0);
}

template <FinalArrangerSelectionsSubclass T>
ArrangerSelectionsAction::ResizeAction::ResizeAction (
  const T   &sel_before,
  const T *  sel_after,
  ResizeType type,
  double     ticks)
    : ArrangerSelectionsAction (sel_before, Type::Resize)
{
  resize_type_ = type;
  ticks_ = ticks;
  /* validate */
  bool have_unresizable = sel_before.contains_object_with_property (
    ArrangerSelections::Property::HasLength, false);
  if (have_unresizable)
    {
      throw ZrythmException (
        QObject::tr ("Attempted to resize unresizable objects"));
    }

  bool have_looped = sel_before.contains_object_with_property (
    ArrangerSelections::Property::HasLooped, true);
  if (
    have_looped
    && (type == ResizeType::L || type == ResizeType::R || type == ResizeType::LStretch || type == ResizeType::RStretch))
    {
      throw ZrythmException (format_qstr (
        QObject::tr (
          "Cannot perform {} resize - selections contain looped objects"),
        type));
    }

  bool have_unloopable = sel_before.contains_object_with_property (
    ArrangerSelections::Property::CanLoop, false);
  if (
    have_unloopable && (type == ResizeType::LLoop || type == ResizeType::RLoop))
    {
      throw ZrythmException (format_qstr (
        QObject::tr (
          "Cannot perform {} resize - selections contain unloopable objects"),
        type));
    }

  if (sel_after)
    {
      set_after_selections (*sel_after);
    }
}

template <FinalRegionSubclass RegionT>
ArrangerSelectionsAction::AutomationFillAction::AutomationFillAction (
  const RegionT &region_before,
  const RegionT &region_after,
  bool           already_changed)
    : ArrangerSelectionsAction ()
{
  type_ = Type::AutomationFill;

  auto * clone = region_before.clone_raw_ptr ();
  clone->setParent (this);
  region_before_ = clone;

  clone = region_after.clone_raw_ptr ();
  clone->setParent (this);
  region_after_ = clone;
  first_run_ = already_changed;
}

void
ArrangerSelectionsAction::update_region_link_groups (const auto &objects)
{
  /* handle children of linked regions */
  for (auto &_obj_var : objects)
    {
      std::visit (
        [&] (auto &&_obj) {
          using ObjT = base_type<decltype (_obj)>;
          /* get the actual object from the project */
          auto obj_var = _obj->find_in_project ();
          z_return_if_fail (obj_var);
          const auto * obj = std::get<ObjT *> (*obj_var);

          if constexpr (std::derived_from<ObjT, RegionOwnedObject>)
            {
              Region * region = obj->get_region ();
              z_return_if_fail (region);

              /* shift all linked objects */
              region->update_link_group ();
            }
        },
        _obj_var);
    }
}

void
ArrangerSelectionsAction::do_or_undo_move (bool do_it)
{
  std::visit (
    [&] (auto &&sel) {
      sel->sort_by_indices (!do_it);

      double ticks = do_it ? ticks_ : -ticks_;
      int    delta_tracks = do_it ? delta_tracks_ : -delta_tracks_;
      int    delta_lanes = do_it ? delta_lanes_ : -delta_lanes_;
      int    delta_chords = do_it ? delta_chords_ : -delta_chords_;
      int    delta_pitch = do_it ? delta_pitch_ : -delta_pitch_;
      double delta_normalized_amt =
        do_it ? delta_normalized_amount_ : -delta_normalized_amount_;

      /* this is used for automation points to keep track of which automation
       * point in the project matches which automation point in the cached
       * selections key: project object, value: own object (from sel_->objects_)
       */
      std::unordered_map<AutomationPoint *, AutomationPoint *> obj_map;

      if (!first_run_)
        {
          for (auto &own_obj : sel->objects_)
            {
              std::visit (
                [&] (auto &&own_obj_ptr) {
                  using ObjT = base_type<decltype (own_obj_ptr)>;
                  own_obj_ptr->flags_ |= ArrangerObject::Flags::NonProject;

                  /* get the actual object from the project */
                  auto prj_obj =
                    std::get<ObjT *> (*own_obj_ptr->find_in_project ());

                  /* remember if automation point */
                  if constexpr (std::is_same_v<ObjT, AutomationPoint>)
                    {
                      obj_map[prj_obj] = own_obj_ptr;
                    }

                  if (!math_doubles_equal (ticks, 0.0))
                    {
                      /* shift the actual object */
                      prj_obj->move (ticks);

                      /* also shift the copy */
                      own_obj_ptr->move (ticks);
                    }

                  if (delta_tracks != 0 || delta_lanes != 0)
                    {
                      /* shift the actual object */
                      move_obj_by_tracks_and_lanes (
                        prj_obj, delta_tracks, delta_lanes, false, -1);

                      /* remember new info in own copy */
                      own_obj_ptr->copy_identifier (*prj_obj);
                    }

                  if (delta_pitch != 0)
                    {
                      if constexpr (std::is_same_v<ObjT, MidiNote>)
                        {
                          /* shift the actual object */
                          prj_obj->shift_pitch (delta_pitch);

                          /* also shift the copy so they can match */
                          own_obj_ptr->shift_pitch (delta_pitch);
                        }
                      else
                        {
                          z_return_if_reached ();
                        }
                    }

                  if (delta_chords != 0)
                    {
                      if constexpr (std::is_same_v<ObjT, ChordObject>)
                        {
                          /* shift the actual object */
                          prj_obj->chord_index_ += delta_chords;

                          /* also shift the copy so they can match */
                          own_obj_ptr->chord_index_ += delta_chords;
                        }
                      else
                        {
                          z_return_if_reached ();
                        }
                    }

                  /* if moving automation */
                  if (target_port_)
                    {
                      if constexpr (std::is_same_v<ObjT, AutomationRegion>)
                        {
                          auto cur_at = prj_obj->get_automation_track ();
                          z_return_if_fail (cur_at);
                          auto port = Port::find_from_identifier<ControlPort> (
                            *target_port_);
                          z_return_if_fail (port);
                          auto track = dynamic_cast<AutomatableTrack *> (
                            port->get_track (true));
                          z_return_if_fail (track);
                          AutomationTrack * at =
                            AutomationTrack::find_from_port (*port, track, true);
                          z_return_if_fail (at);

                          /* move the actual object */
                          prj_obj->move_to_track (track, at->index_, -1);

                          /* remember info in identifier */
                          own_obj_ptr->id_ = prj_obj->id_;

                          target_port_ = cur_at->port_id_->clone_raw_ptr ();
                          target_port_->setParent (this);
                        }
                      else
                        {
                          z_return_if_reached ();
                        }
                    }

                  if (!math_doubles_equal (delta_normalized_amt, 0.0))
                    {
                      if constexpr (std::is_same_v<ObjT, AutomationPoint>)
                        {
                          /* shift the actual object */
                          prj_obj->set_fvalue (
                            prj_obj->normalized_val_
                              + static_cast<float> (delta_normalized_amt),
                            true, false);

                          /* also shift the copy so they can match */
                          own_obj_ptr->set_fvalue (
                            own_obj_ptr->normalized_val_
                              + static_cast<float> (delta_normalized_amt),
                            true, false);
                        }
                      else
                        {
                          z_return_if_reached ();
                        }
                    }
                },
                own_obj);
            }

          /* if moving automation points, re-sort the region and remember the
           * new indices */
          auto first_own_obj_var = sel->objects_.front ();
          std::visit (
            [&] (auto &&first_own_obj) {
              using ObjT = base_type<decltype (first_own_obj)>;
              if constexpr (std::is_same_v<ObjT, AutomationPoint>)
                {
                  auto obj = std::get<AutomationPoint *> (
                    *first_own_obj->find_in_project ());
                  z_return_if_fail (obj);

                  auto region =
                    dynamic_cast<AutomationRegion *> (obj->get_region ());
                  z_return_if_fail (region);
                  region->force_sort ();

                  for (auto &[prj_ap, cached_ap] : obj_map)
                    {
                      auto * prj_ap_cast =
                        dynamic_cast<AutomationPoint *> (prj_ap);
                      auto * cached_ap_cast =
                        dynamic_cast<AutomationPoint *> (cached_ap);
                      cached_ap_cast->set_region_and_index (
                        *region, prj_ap_cast->index_);
                    }
                }
            },
            first_own_obj_var);
        }

      update_region_link_groups (sel->objects_);

      /* validate */
      CLIP_EDITOR->get_region ();

      // ArrangerSelections * sel = get_actual_arranger_selections ();
      /* EVENTS_PUSH (EventType::ET_ARRANGER_SELECTIONS_CHANGED, sel); */
      /* EVENTS_PUSH (EventType::ET_ARRANGER_SELECTIONS_MOVED, sel); */

      first_run_ = false;
    },
    *sel_);
}

void
ArrangerSelectionsAction::move_obj_by_tracks_and_lanes (
  ArrangerObjectWithoutVelocityPtrVariant obj_var,
  const int                               tracks_diff,
  const int                               lanes_diff,
  bool                                    use_index_in_prev_lane,
  int                                     index_in_prev_lane)
{
  std::visit (
    [&] (auto &&obj) {
      using ObjT = base_type<decltype (obj)>;
      z_debug (
        "Moving object {}: tracks_diff={}, lanes_diff={}", *obj, tracks_diff,
        lanes_diff);

      if (tracks_diff)
        {
          if constexpr (std::derived_from<ObjT, Region>)
            {
              const auto track_before_var = obj->get_track ();
              std::visit (
                [&] (auto &&track_before) {
                  auto _track_to_move_to =
                    TRACKLIST->get_visible_track_after_delta (
                      *track_before, tracks_diff);
                  z_return_if_fail (_track_to_move_to);
                  auto * track_to_move_to =
                    Track::from_variant (_track_to_move_to.value ());
                  z_trace (
                    "Moving from track {} ({}) to track: {} ({})",
                    track_before->get_name (), track_before->get_name_hash (),
                    track_to_move_to->get_name (),
                    track_to_move_to->get_name_hash ());

                  /* shift the actual object by tracks */
                  if (
                    ENUM_BITSET_TEST (
                      ArrangerObjectFlags, obj->flags_,
                      ArrangerObject::Flags::NonProject))
                    {
                      obj->id_.track_name_hash_ =
                        track_to_move_to->get_name_hash ();
                      obj->track_name_hash_ = track_to_move_to->get_name_hash ();
                      z_trace ("Updated track name hash for non-project object");
                    }
                  else
                    {
                      obj->move_to_track (
                        track_to_move_to, -1,
                        use_index_in_prev_lane ? index_in_prev_lane : -1);
                      z_trace ("Moved project object to track");
                    }
                },
                track_before_var);
            }
        }
      if (lanes_diff)
        {
          if constexpr (std::derived_from<ObjT, Region>)
            {
              int new_lane_pos = obj->id_.lane_pos_ + lanes_diff;
              z_return_if_fail (new_lane_pos >= 0);
              z_trace ("New lane position: {}", new_lane_pos);

              /* shift the actual object by lanes */
              if (
                ENUM_BITSET_TEST (
                  ArrangerObject::Flags, obj->flags_,
                  ArrangerObject::Flags::NonProject))
                {
                  obj->id_.lane_pos_ = new_lane_pos;
                  z_trace ("Updated lane position for non-project object");
                }
              else
                {
                  if constexpr (std::derived_from<ObjT, LaneOwnedObject>)
                    {
                      std::visit (
                        [&] (auto &&r_track) {
                          using TrackT = base_type<decltype (r_track)>;
                          if constexpr (std::derived_from<TrackT, LanedTrack>)
                            {
                              r_track->create_missing_lanes (new_lane_pos);
                              z_trace (
                                "Created missing lanes up to {}", new_lane_pos);
                              obj->move_to_track (
                                r_track, new_lane_pos,
                                use_index_in_prev_lane ? index_in_prev_lane : -1);
                              z_trace ("Moved project object to new lane");
                            }
                        },
                        obj->get_track ());
                    }
                }
            }
        }
      z_debug ("Object move completed");
    },
    obj_var);
}

void
ArrangerSelectionsAction::do_or_undo_duplicate_or_link (bool link, bool do_it)
{
  std::visit (
    [&] (auto &&sel, auto &&sel_after) {
      using SelT = base_type<decltype (sel)>;
      sel->sort_by_indices (!do_it);
      sel_after->sort_by_indices (!do_it);
      if (ZRYTHM_TESTING)
        {
          sel_after->validate ();
        }

      auto * actual_sel = std::get<SelT *> (get_actual_arranger_selections ());

      const double ticks = do_it ? ticks_ : -ticks_;
      const int    delta_tracks = do_it ? delta_tracks_ : -delta_tracks_;
      const int    delta_lanes = do_it ? delta_lanes_ : -delta_lanes_;
      const int    delta_chords = do_it ? delta_chords_ : -delta_chords_;
      const int    delta_pitch = do_it ? delta_pitch_ : -delta_pitch_;
      const double delta_normalized_amount =
        do_it ? delta_normalized_amount_ : -delta_normalized_amount_;

      /* clear current selections in the project */
      actual_sel->clear (false);

      /* this is used for automation points to keep track of which automation
       * point in the project matches which automation point in the cached
       * selections key: project automation point, value: cached automation
       * point */
      std::map<AutomationPoint *, AutomationPoint *> ap_map;

      // this essentially sets the NonProject flag if not performing on first run
      for (
        auto it = sel_after->objects_.rbegin ();
        it != sel_after->objects_.rend (); ++it)
        {
          size_t i =
            std::distance (sel_after->objects_.begin (), it.base ()) - 1;
          std::visit (
            [&] (auto &&own_obj) {
              using ObjT = base_type<decltype (own_obj)>;
              auto own_orig_obj = std::get<ObjT *> (sel->objects_.at (i));
              own_obj->flags_ |= ArrangerObject::Flags::NonProject;
              own_orig_obj->flags_ |= ArrangerObject::Flags::NonProject;

              /* on first run, we need to first move the original object
               * backwards (the project object too) */
              if (do_it && first_run_)
                {
                  auto obj = std::get<ObjT *> (*own_obj->find_in_project ());

                  z_debug ("{} moving original object backwards", i);

                  /* ticks */
                  if (!math_doubles_equal (ticks_, 0.0))
                    {
                      obj->move (-ticks_);
                      own_obj->move (-ticks_);
                    }

                  /* tracks & lanes */
                  if (delta_tracks_ || delta_lanes_)
                    {
                      if constexpr (std::derived_from<ObjT, LaneOwnedObject>)
                        {
                          z_trace ("moving prj obj");
                          move_obj_by_tracks_and_lanes (
                            obj, -delta_tracks_, -delta_lanes_, true,
                            own_obj->index_in_prev_lane_);

                          z_trace ("moving own obj");
                          RegionIdentifier own_id_before_move = own_obj->id_;
                          move_obj_by_tracks_and_lanes (
                            own_obj, -delta_tracks_, -delta_lanes_, true,
                            own_obj->index_in_prev_lane_);

                          /* since the object moved outside of its lane,
                           * decrement the index inside the lane for all of our
                           * cached objects in the same lane */
                          for (
                            size_t j = i + 1; j < sel_after->objects_.size ();
                            j++)
                            {
                              if (
                                own_id_before_move.track_name_hash_
                                  == own_obj->id_.track_name_hash_
                                && own_id_before_move.lane_pos_
                                     == own_obj->id_.lane_pos_
                                && own_id_before_move.at_idx_
                                     == own_obj->id_.at_idx_)
                                {
                                  own_obj->id_.idx_--;
                                }
                            }
                        }
                    }

                  /* pitch */
                  if (delta_pitch_)
                    {
                      if constexpr (std::is_same_v<ObjT, MidiNote>)
                        {
                          obj->shift_pitch (-delta_pitch_);
                          own_obj->shift_pitch (-delta_pitch_);
                        }
                    }

                  /* chords */
                  if (delta_chords_ != 0)
                    {
                      if constexpr (std::is_same_v<ObjT, ChordObject>)
                        {
                          /* shift the actual object */
                          obj->chord_index_ -= delta_chords_;

                          /* also shift the copy so they can match */
                          own_obj->chord_index_ -= delta_chords_;
                        }
                    }

                  /* if moving automation */
                  if (target_port_)
                    {
                      z_error (
                        "not supported - automation must not be moved before performing the action");
                    }

                  /* automation value */
                  if (!math_doubles_equal (delta_normalized_amount_, 0.0))
                    {
                      if constexpr (std::is_same_v<ObjT, AutomationPoint>)
                        {
                          obj->set_fvalue (
                            obj->normalized_val_
                              - (float) delta_normalized_amount_,
                            true, false);
                          own_obj->set_fvalue (
                            own_obj->normalized_val_
                              - (float) delta_normalized_amount_,
                            true, false);
                        }
                    }
                } /* if do and first run */
            },
            (*it));
        }

      for (size_t i = 0; i < sel_after->objects_.size (); i++)
        {
          std::visit (
            [&] (auto &&own_obj) {
              using ObjT = base_type<decltype (own_obj)>;
              auto own_orig_obj = std::get<ObjT *> (sel->objects_.at (i));

              if (do_it)
                {
                  auto add_adjusted_clone_to_project =
                    [] (const auto &obj_to_clone) {
                      /* create a temporary clone */
                      auto obj = obj_to_clone->clone_unique ();

                      /* if region, clear the remembered index so that the
                       * region gets appended instead of inserted */
                      if constexpr (RegionSubclass<ObjT>)
                        {
                          obj->id_.idx_ = -1;
                        }

                      /* add to track. */
                      return std::get<ObjT *> (
                        obj->add_clone_to_project (false));
                    };

                  auto added_obj_ref = add_adjusted_clone_to_project (own_obj);

                  /* edit both project object and the copy */
                  if (!math_doubles_equal (ticks, 0.0))
                    {
                      added_obj_ref->move (ticks);
                      own_obj->move (ticks);
                    }
                  if (delta_tracks != 0)
                    {
                      move_obj_by_tracks_and_lanes (
                        added_obj_ref, delta_tracks, 0, false, -1);
                      move_obj_by_tracks_and_lanes (
                        own_obj, delta_tracks, 0, false, -1);
                    }
                  if (delta_lanes != 0)
                    {
                      move_obj_by_tracks_and_lanes (
                        added_obj_ref, 0, delta_lanes, false, -1);
                      move_obj_by_tracks_and_lanes (
                        own_obj, 0, delta_lanes, false, -1);
                    }
                  if (delta_pitch != 0)
                    {
                      if constexpr (std::is_same_v<ObjT, MidiNote>)
                        {
                          added_obj_ref->shift_pitch (delta_pitch);
                          own_obj->shift_pitch (delta_pitch);
                        }
                    }
                  if (delta_chords != 0)
                    {
                      if constexpr (std::is_same_v<ObjT, ChordObject>)
                        {
                          added_obj_ref->chord_index_ += delta_chords;
                          own_obj->chord_index_ += delta_chords;
                        }
                    }
                  if (target_port_)
                    {
                      if constexpr (std::is_same_v<ObjT, AutomationRegion>)
                        {
                          auto cur_at = added_obj_ref->get_automation_track ();
                          z_return_if_fail (cur_at);
                          const auto * port = Port::find_from_identifier<
                            ControlPort> (*target_port_);
                          z_return_if_fail (port);
                          auto * track = dynamic_cast<AutomatableTrack *> (
                            port->get_track (true));
                          z_return_if_fail (track);
                          const auto * at = AutomationTrack::find_from_port (
                            *port, track, true);
                          z_return_if_fail (at);

                          /* move the actual object */
                          added_obj_ref->move_to_track (track, at->index_, -1);
                        }
                    }
                  if (!math_floats_equal (delta_normalized_amount, 0.f))
                    {
                      if constexpr (std::is_same_v<ObjT, AutomationPoint>)
                        {
                          added_obj_ref->set_fvalue (
                            added_obj_ref->normalized_val_
                              + static_cast<float> (delta_normalized_amount),
                            true, false);
                          own_obj->set_fvalue (
                            own_obj->normalized_val_
                              + static_cast<float> (delta_normalized_amount),
                            true, false);
                        }
                    }

                  if constexpr (RegionSubclass<ObjT>)
                    {
                      /* if we are linking, create the necessary links */
                      if (link)
                        {
                          /* add link group to original object if necessary */
                          auto orig_r = std::get<ObjT *> (
                            *own_orig_obj->find_in_project ());
                          z_return_if_fail (orig_r->id_.idx_ >= 0);

                          orig_r->create_link_group_if_none ();
                          const auto link_group = orig_r->id_.link_group_;

                          /* add link group to clone */
                          z_return_if_fail (
                            added_obj_ref->id_.type_ == orig_r->id_.type_);
                          z_return_if_fail (added_obj_ref->id_.idx_ >= 0);
                          added_obj_ref->set_link_group (link_group, true);

                          /* remember link groups */
                          own_orig_obj->set_link_group (link_group, true);
                          own_obj->set_link_group (link_group, true);

                          REGION_LINK_GROUP_MANAGER.validate ();
                        }
                      else /* else if we are not linking */
                        {
                          /* remove link group if first run */
                          if (first_run_)
                            {
                              if (added_obj_ref->has_link_group ())
                                {
                                  added_obj_ref->unlink ();
                                }
                            }

                          /* if this is an audio region, duplicate the clip */
                          if constexpr (std::is_same_v<ObjT, AudioRegion>)
                            {
                              auto *     clip = added_obj_ref->get_clip ();
                              const auto id = AUDIO_POOL->duplicate_clip (
                                clip->pool_id_, true);
                              if (id < 0)
                                {
                                  throw ZrythmException (
                                    "Failed to duplicate audio clip");
                                }
                              clip = AUDIO_POOL->get_clip (id);
                              z_return_if_fail (clip);
                              added_obj_ref->pool_id_ = clip->pool_id_;
                            }
                        }
                    } /* endif region */

                  /* add the mapping to the hashtable */
                  if constexpr (std::is_same_v<ObjT, AutomationPoint>)
                    {
                      ap_map[added_obj_ref] = own_obj;
                    }

                  /* select it */
                  added_obj_ref->select (true, true, false);

                  /* remember the identifier */
                  own_obj->copy_identifier (*added_obj_ref);

                } /* endif do */
              else /* if undo */
                {
                  /* find the actual object */
                  auto obj = std::get<ObjT *> (*own_obj->find_in_project ());

                  /* if the object was created with linking, delete the links */
                  if constexpr (RegionSubclass<ObjT>)
                    {
                      if (link)
                        {
                          /* remove link from created object (this will also
                           * automatically remove the link from the parent region
                           * if it is the only region in the link group) */
                          z_return_if_fail (obj->has_link_group ());
                          obj->unlink ();
                          z_return_if_fail (obj->id_.link_group_ == -1);

                          /* unlink remembered link groups */
                          own_orig_obj->unlink ();
                          own_obj->unlink ();
                        }
                    }

                  /* remove it */
                  obj->remove_from_project (true);

                  /* set the copies back to original state */
                  if (!math_doubles_equal (ticks, 0.0))
                    {
                      own_obj->move (ticks);
                    }
                  if (delta_tracks != 0)
                    {
                      move_obj_by_tracks_and_lanes (
                        own_obj, delta_tracks, 0, false, -1);
                    }
                  if (delta_lanes != 0)
                    {
                      move_obj_by_tracks_and_lanes (
                        own_obj, 0, delta_lanes, false, -1);
                    }
                  if (delta_pitch != 0)
                    {
                      if constexpr (std::is_same_v<ObjT, MidiNote>)
                        {
                          own_obj->shift_pitch (delta_pitch);
                        }
                    }
                  if (delta_chords != 0)
                    {
                      if constexpr (std::is_same_v<ObjT, ChordObject>)
                        {
                          own_obj->chord_index_ += delta_chords;
                        }
                    }
                  if (target_port_)
                    {
                      /* nothing needed */
                    }
                  if (!math_floats_equal (delta_normalized_amount, 0.f))
                    {
                      if constexpr (std::is_same_v<ObjT, AutomationPoint>)
                        {
                          own_obj->set_fvalue (
                            own_obj->normalized_val_
                              + static_cast<float> (delta_normalized_amount),
                            true, false);
                        }
                    }

                } /* endif undo */

              REGION_LINK_GROUP_MANAGER.validate ();
            },
            sel_after->objects_.at (i));
        }

      /* if copy-moving automation points, re-sort the region and remember the
       * new indices */
      if (
        std::holds_alternative<AutomationPoint *> (sel_after->objects_.front ()))
        {
          /* get the actual object from the project */
          const auto * obj = ap_map.begin ()->first;
          if (obj != nullptr)
            {
              auto * region =
                dynamic_cast<AutomationRegion *> (obj->get_region ());
              z_return_if_fail (region);
              region->force_sort ();

              for (auto &pair : ap_map)
                {
                  const auto * prj_ap = pair.first;
                  auto *       cached_ap = pair.second;
                  cached_ap->set_region_and_index (*region, prj_ap->index_);
                }
            }
        }

      /* validate */
      P_MARKER_TRACK->validate ();
      P_CHORD_TRACK->validate ();
      REGION_LINK_GROUP_MANAGER.validate ();
      CLIP_EDITOR->get_region ();

      actual_sel = std::get<SelT *> (get_actual_arranger_selections ());
      if (do_it)
        {
          TRANSPORT->recalculate_total_bars (sel_after);

          /* EVENTS_PUSH (EventType::ET_ARRANGER_SELECTIONS_CREATED, sel); */
        }
      else
        {
          /* EVENTS_PUSH (EventType::ET_ARRANGER_SELECTIONS_REMOVED, sel); */
        }

      first_run_ = false;
    },
    *sel_, *sel_after_);
}

void
ArrangerSelectionsAction::do_or_undo_create_or_delete (bool do_it, bool create)
{
  std::visit (
    [&] (auto &&sel) {
      using SelT = base_type<decltype (sel)>;
      sel->sort_by_indices (create ? !do_it : do_it);
      auto * actual_sel = std::get<SelT *> (get_actual_arranger_selections ());
      z_return_if_fail (actual_sel);

      if (!first_run_ || !create)
        {
          /* clear current selections in the project */
          actual_sel->clear (false);

          for (auto &own_obj_var : sel->objects_)
            {
              std::visit (
                [&] (auto &&own_obj) {
                  using ObjT = base_type<decltype (own_obj)>;
                  own_obj->flags_ |= ArrangerObject::Flags::NonProject;

                  /* if doing in a create action or undoing in a delete action */
                  if ((do_it && create) || (!do_it && !create))
                    {
                      /* add a clone to the project */
                      ObjT * prj_obj = nullptr;
                      if (create)
                        {
                          prj_obj = std::get<ObjT *> (
                            own_obj->add_clone_to_project (false));
                        }
                      else
                        {
                          prj_obj = std::get<ObjT *> (
                            own_obj->insert_clone_to_project ());
                        }

                      /* select it */
                      prj_obj->select (true, true, false);

                      /* remember new info */
                      own_obj->copy_identifier (*prj_obj);
                    }

                  /* if removing */
                  else
                    {
                      /* get the actual object from the project */
                      auto obj_opt = own_obj->find_in_project ();
                      z_return_if_fail (obj_opt);
                      auto obj = std::get<ObjT *> (*obj_opt);

                      /* if region, remove link */
                      if constexpr (std::derived_from<ObjT, Region>)
                        {
                          if (obj->has_link_group ())
                            {
                              obj->unlink ();
                            }
                        }

                      /* remove it */
                      obj->remove_from_project (true);
                    }
                },
                own_obj_var);
            }
        }

      /* if first time creating the object, save the length for use by SnapGrid */
      if (
        ZRYTHM_HAVE_UI && first_run_ && create && do_it
        && sel->objects_.size () == 1)
        {
          std::visit (
            [&] (auto &&own_obj) {
              using ObjT = base_type<decltype (own_obj)>;
              auto obj = std::get<ObjT *> (*own_obj->find_in_project ());
              if constexpr (std::derived_from<ObjT, LengthableObject>)
                {
                  double ticks = obj->get_length_in_ticks ();
                  if constexpr (std::derived_from<ObjT, TimelineObject>)
                    {
                      gui::SettingsManager::get_instance ()
                        ->set_timelineLastCreatedObjectLengthInTicks (ticks);
                    }
                  else if constexpr (std::derived_from<ObjT, RegionOwnedObject>)
                    {
                      gui::SettingsManager::get_instance ()
                        ->set_editorLastCreatedObjectLengthInTicks (ticks);
                    }
                }
            },
            actual_sel->objects_.front ());
        }

      /* if creating */
      if ((do_it && create) || (!do_it && !create))
        {
          update_region_link_groups (sel->objects_);

          TRANSPORT->recalculate_total_bars (sel);

          /* EVENTS_PUSH (EventType::ET_ARRANGER_SELECTIONS_CREATED, sel); */
        }
      /* if deleting */
      else
        {
          /* EVENTS_PUSH (EventType::ET_ARRANGER_SELECTIONS_REMOVED, sel); */
        }

      P_MARKER_TRACK->validate ();
      P_CHORD_TRACK->validate ();
      REGION_LINK_GROUP_MANAGER.validate ();

      first_run_ = false;

      if constexpr (std::is_same_v<SelT, TimelineSelections>)
        {
          bool have_automation_region =
            std::ranges::any_of (sel->objects_, [] (const auto &obj_var) {
              return std::visit (
                [] (auto &&obj) {
                  using ObjT = base_type<decltype (obj)>;
                  return std::is_same_v<ObjT, AutomationRegion>;
                },
                obj_var);
            });

          if (have_automation_region)
            {
              ROUTER->recalc_graph (false);
            }
        }
    },
    *sel_);
}

void
ArrangerSelectionsAction::do_or_undo_record (bool do_it)
{
  std::visit (
    [&] (auto &&sel, auto &&sel_after) {
      using SelT = base_type<decltype (sel)>;

      sel->sort_by_indices (!do_it);
      sel_after->sort_by_indices (!do_it);
      auto * actual_sel = std::get<SelT *> (get_actual_arranger_selections ());
      z_return_if_fail (actual_sel);

      if (!first_run_)
        {
          /* clear current selections in the project */
          actual_sel->clear (false);

          /* if do/redoing */
          if (do_it)
            {
              /* create the newly recorded objects */
              for (auto &own_after_obj_var : sel_after->objects_)
                {
                  std::visit (
                    [&] (auto &&own_after_obj) {
                      using ObjT = base_type<decltype (own_after_obj)>;
                      own_after_obj->flags_ |= ArrangerObject::Flags::NonProject;

                      /* add a clone to the project */
                      auto prj_obj = std::get<ObjT *> (
                        own_after_obj->add_clone_to_project (false));

                      /* select it */
                      prj_obj->select (true, true, false);

                      /* remember new info */
                      own_after_obj->copy_identifier (*prj_obj);
                    },
                    own_after_obj_var);
                }

              /* delete the previous objects */
              for (auto &own_before_obj_var : sel->objects_)
                {
                  std::visit (
                    [&] (auto &&own_before_obj) {
                      using ObjT = base_type<decltype (own_before_obj)>;
                      own_before_obj->flags_ |=
                        ArrangerObject::Flags::NonProject;

                      /* get the actual object from the project */
                      auto obj =
                        std::get<ObjT *> (*own_before_obj->find_in_project ());

                      /* remove it */
                      obj->remove_from_project (true);
                    },
                    own_before_obj_var);
                }
            }

          /* if undoing */
          else
            {
              /* delete the newly recorded objects */
              for (auto &own_after_obj_var : sel_after->objects_)
                {
                  std::visit (
                    [&] (auto &&own_after_obj) {
                      using ObjT = base_type<decltype (own_after_obj)>;
                      own_after_obj->flags_ |= ArrangerObject::Flags::NonProject;

                      /* get the actual object from the project */
                      auto obj_opt = own_after_obj->find_in_project ();
                      z_return_if_fail (obj_opt);
                      auto * obj = std::get<ObjT *> (*obj_opt);

                      /* remove it */
                      obj->remove_from_project (true);
                    },
                    own_after_obj_var);
                }

              /* add the objects before the recording */
              for (auto &own_before_obj_var : sel->objects_)
                {
                  std::visit (
                    [&] (auto &&own_before_obj) {
                      using ObjT = base_type<decltype (own_before_obj)>;
                      own_before_obj->flags_ |=
                        ArrangerObject::Flags::NonProject;

                      /* add a clone to the project */
                      auto prj_obj = std::get<ObjT *> (
                        own_before_obj->add_clone_to_project (false));

                      /* select it */
                      prj_obj->select (true, true, false);

                      /* remember new info */
                      own_before_obj->copy_identifier (*prj_obj);
                    },
                    own_before_obj_var);
                }
            }
        }

      /* EVENTS_PUSH (EventType::ET_ARRANGER_SELECTIONS_CREATED, sel); */
      /* EVENTS_PUSH (EventType::ET_ARRANGER_SELECTIONS_REMOVED, sel); */

      first_run_ = false;

      if constexpr (std::is_same_v<SelT, TimelineSelections>)
        {
          bool have_automation_region =
            std::ranges::any_of (sel->objects_, [] (const auto &obj_var) {
              return std::visit (
                [] (auto &&obj) {
                  using ObjT = base_type<decltype (obj)>;
                  return std::is_same_v<ObjT, AutomationRegion>;
                },
                obj_var);
            });
          if (have_automation_region)
            {
              ROUTER->recalc_graph (false);
            }
        }
    },
    *sel_, *sel_after_);
}

void
ArrangerSelectionsAction::do_or_undo_edit (bool do_it)
{
  auto &src_sel_var = do_it ? sel_ : sel_after_;
  auto &dest_sel_var = do_it ? sel_after_ : sel_;
  std::visit (
    [&] (auto &&src_sel, auto &&dest_sel) {
      using SelT = base_type<decltype (src_sel)>;
      if constexpr (std::is_same_v<SelT, base_type<decltype (dest_sel)>>)
        {
          if (!first_run_)
            {
              if constexpr (std::is_same_v<SelT, AudioSelections>)
                {
                  z_return_if_fail (edit_type_ == EditType::EditorFunction);
                  auto src_audio_sel = dest_sel;
                  auto r =
                    RegionImpl<AudioRegion>::find (src_audio_sel->region_id_);
                  z_return_if_fail (r);
                  auto src_clip = AUDIO_POOL->get_clip (src_audio_sel->pool_id_);
                  z_return_if_fail (src_clip);

                  /* adjust the positions */
                  Position start = src_audio_sel->sel_start_;
                  Position end = src_audio_sel->sel_end_;
                  start.add_frames (-r->pos_->frames_, AUDIO_ENGINE->ticks_per_frame_);
                  end.add_frames (-r->pos_->frames_, AUDIO_ENGINE->ticks_per_frame_);
                  auto num_frames =
                    static_cast<unsigned_frame_t> (end.frames_ - start.frames_);
                  z_return_if_fail (num_frames == src_clip->num_frames_);

                  auto src_clip_path = src_clip->get_path_in_pool (false);
                  z_debug (
                    "replacing audio region {} frames with {} frames", r->name_,
                    src_clip_path);

                  /* replace the frames in the region */
                  r->replace_frames (
                    src_clip->frames_.getReadPointer (0), start.frames_,
                    num_frames, false);
                }
              else /* not audio function */
                {
                  for (size_t i = 0; i < src_sel->objects_.size (); i++)
                    {
                      auto own_src_obj_var = src_sel->objects_.at (i);
                      auto own_dest_obj_var = dest_sel->objects_.at (i);
                      std::visit (
                        [&] (auto &&own_src_obj, auto &&own_dest_obj) {
                          using ObjT = base_type<decltype (own_src_obj)>;
                          if constexpr (
                            std::is_same_v<
                              base_type<decltype (own_dest_obj)>, ObjT>)
                            {
                              own_src_obj->flags_ |=
                                ArrangerObject::Flags::NonProject;
                              own_dest_obj->flags_ |=
                                ArrangerObject::Flags::NonProject;

                              /* find the actual object */
                              auto obj = std::get<ObjT *> (
                                *own_src_obj->find_in_project ());

                              /* change the parameter */
                              switch (edit_type_)
                                {
                                case EditType::Name:
                                  {
                                    if constexpr (
                                      std::derived_from<ObjT, NameableObject>)
                                      {
                                        obj->set_name (
                                          own_dest_obj->name_, false);
                                      }
                                    else
                                      {
                                        throw ZrythmException (
                                          "Not a nameable object");
                                      }
                                  }
                                  break;
                                case EditType::Position:
                                  obj->pos_ = own_dest_obj->pos_;
                                  if constexpr (
                                    std::derived_from<ObjT, LengthableObject>)
                                    {
                                      obj->end_pos_ = own_dest_obj->end_pos_;
                                    }
                                  if constexpr (
                                    std::derived_from<ObjT, LoopableObject>)
                                    {
                                      obj->loop_start_pos_ =
                                        own_dest_obj->loop_start_pos_;
                                      obj->loop_end_pos_ =
                                        own_dest_obj->loop_end_pos_;
                                      obj->clip_start_pos_ =
                                        own_dest_obj->clip_start_pos_;
                                    }
                                  break;
                                case EditType::Fades:
                                  if constexpr (
                                    std::derived_from<ObjT, FadeableObject>)
                                    {
                                      obj->fade_in_pos_ =
                                        own_dest_obj->fade_in_pos_;
                                      obj->fade_out_pos_ =
                                        own_dest_obj->fade_out_pos_;
                                      obj->fade_in_opts_ =
                                        own_dest_obj->fade_in_opts_;
                                      obj->fade_out_opts_ =
                                        own_dest_obj->fade_out_opts_;
                                    }
                                  break;
                                case EditType::Primitive:
                                  if constexpr (
                                    std::derived_from<ObjT, MuteableObject>)
                                    {
                                      obj->muted_ = own_dest_obj->muted_;
                                    }
                                  if constexpr (
                                    std::derived_from<ObjT, ColoredObject>)
                                    {
                                      obj->color_ = own_dest_obj->color_;
                                      obj->use_color_ = own_dest_obj->use_color_;
                                    }
                                  if constexpr (
                                    std::is_same_v<ObjT, AudioRegion>)
                                    {
                                      obj->musical_mode_ =
                                        own_dest_obj->musical_mode_;
                                      obj->gain_ = own_dest_obj->gain_;
                                    }
                                  if constexpr (std::is_same_v<ObjT, MidiNote>)
                                    {
                                      obj->val_ = own_dest_obj->val_;
                                      obj->vel_->set_val (
                                        own_dest_obj->vel_->vel_);
                                    }
                                  if constexpr (
                                    std::is_same_v<ObjT, AutomationPoint>)
                                    {
                                      obj->curve_opts_ =
                                        own_dest_obj->curve_opts_;
                                      obj->fvalue_ = own_dest_obj->fvalue_;
                                      obj->normalized_val_ =
                                        own_dest_obj->normalized_val_;
                                    }
                                  break;
                                case EditType::EditorFunction:
                                  obj->pos_ = own_dest_obj->pos_;
                                  if constexpr (
                                    std::derived_from<ObjT, LengthableObject>)
                                    {
                                      obj->end_pos_ = own_dest_obj->end_pos_;
                                    }
                                  if constexpr (
                                    std::derived_from<ObjT, LoopableObject>)
                                    {
                                      obj->clip_start_pos_ =
                                        own_dest_obj->clip_start_pos_;
                                      obj->loop_start_pos_ =
                                        own_dest_obj->loop_start_pos_;
                                      obj->loop_end_pos_ =
                                        own_dest_obj->loop_end_pos_;
                                    }
                                  if constexpr (
                                    std::derived_from<ObjT, MuteableObject>)
                                    {
                                      obj->muted_ = own_dest_obj->muted_;
                                    }
                                  if constexpr (std::is_same_v<ObjT, MidiNote>)
                                    {
                                      obj->val_ = own_dest_obj->val_;
                                      obj->vel_->set_val (
                                        own_dest_obj->vel_->vel_);
                                    }
                                  if constexpr (
                                    std::is_same_v<ObjT, AutomationPoint>)
                                    {
                                      obj->curve_opts_ =
                                        own_dest_obj->curve_opts_;
                                      obj->fvalue_ = own_dest_obj->fvalue_;
                                      obj->normalized_val_ =
                                        own_dest_obj->normalized_val_;
                                    }
                                  break;
                                case EditType::Scale:
                                  {
                                    if constexpr (
                                      std::is_same_v<ObjT, ScaleObject>)
                                      {
                                        obj->scale_ = own_dest_obj->scale_;
                                      }
                                  }
                                  break;
                                case EditType::Mute:
                                  {
                                    if constexpr (
                                      std::derived_from<ObjT, MuteableObject>)
                                      {
                                        obj->muted_ = own_dest_obj->muted_;
                                      }
                                  }
                                  break;
                                default:
                                  break;
                                }
                            }
                          else
                            {
                              z_return_if_reached ();
                            }
                        },
                        own_src_obj_var, own_dest_obj_var);
                    }
                } /* endif audio function */
            } /* endif not first run */

          update_region_link_groups (dest_sel->objects_);

          // auto sel = get_actual_arranger_selections ();
          /* EVENTS_PUSH
           * (EventType::ET_ARRANGER_SELECTIONS_CHANGED_REDRAW_EVERYTHING, sel);
           */

          first_run_ = false;
        }
    },
    *src_sel_var, *dest_sel_var);
}

void
ArrangerSelectionsAction::do_or_undo_automation_fill (bool do_it)
{
  if (!first_run_)
    {
      /* clear current selections in the project */
      TL_SELECTIONS->clear (false);

      /* get the actual object from the project */
      auto region = std::get<AutomationRegion *> (*(
        std::get<AutomationRegion *> (
          do_it ? region_before_.value () : region_after_.value ())
          ->find_in_project ()));
      z_return_if_fail (region);

      /* remove link */
      if (region->has_link_group ())
        {
          region->unlink ();

          /* unlink remembered link groups */
          auto &own_region = std::get<AutomationRegion *> (
            do_it ? *region_before_ : *region_after_);
          own_region->unlink ();
        }

      /* remove it */
      region->remove_from_project (true);

      /* add a clone to the project */
      auto * prj_obj = std::get<AutomationRegion *> (
        std::get<AutomationRegion *> (do_it ? *region_after_ : *region_before_)
          ->add_clone_to_project (false));

      /* select it */
      prj_obj->select (true, true, false);

      /* remember new info */
      std::get<AutomationRegion *> (do_it ? *region_after_ : *region_before_)
        ->copy_identifier (*prj_obj);
    }

  first_run_ = false;
}

void
ArrangerSelectionsAction::do_or_undo_split (bool do_it)
{
  std::visit (
    [&] (auto &&sel, auto &&sel_after) {
      // using SelT = base_type<decltype (sel)>;
      int i = -1;
      for (auto &own_obj_var : sel->objects_)
        {
          std::visit (
            [&] (auto &&own_obj) {
              using ObjT = base_type<decltype (own_obj)>;
              ++i;
              own_obj->flags_ |= ArrangerObject::Flags::NonProject;
              if constexpr (std::derived_from<ObjT, LengthableObject>)
                {
                  if (do_it)
                    {
                      /* find the actual object */
                      auto obj = std::get<ObjT *> (*own_obj->find_in_project ());
                      z_return_if_fail (obj);

                      /* split */
                      auto [r1, r2] =
                        LengthableObject::split (*obj, pos_, false, true);

                      /* r1 and r2 are now inside the project, clone them to
                       * keep copies
                       */
                      z_return_if_fail (r1 && r2);
                      r1_[i] = r1->clone_raw_ptr ();
                      std::get<ObjT *> (r1_.at (i))->setParent (this);
                      r2_[i] = r2->clone_raw_ptr ();
                      std::get<ObjT *> (r2_.at (i))->setParent (this);
                    }
                  /* else if undoing split */
                  else
                    {
                      /* find the actual objects */
                      auto r1 = std::get<ObjT *> (
                        *std::get<ObjT *> (r1_.at (i))->find_in_project ());
                      auto r2 = std::get<ObjT *> (
                        *std::get<ObjT *> (r2_.at (i))->find_in_project ());
                      z_return_if_fail (r1 && r2);

                      /* unsplit */
                      auto obj = LengthableObject::unsplit (*r1, *r2, false);

                      if constexpr (std::derived_from<ObjT, Region>)
                        {
                          obj->set_name (own_obj->name_, false);
                        }

                      /* re-insert object at its original position */
                      obj->remove_from_project (true);
                      own_obj->insert_clone_to_project ();

                      /* free the copies created in _do */
                      std::get<ObjT *> (r1_.at (i))->deleteLater ();
                      std::get<ObjT *> (r2_.at (i))->deleteLater ();
                    }
                }
            },
            own_obj_var);
        }

      if (do_it)
        num_split_objs_ = sel->objects_.size ();
      else
        num_split_objs_ = 0;

      // ArrangerSelections * sel = get_actual_arranger_selections ();
      /* EVENTS_PUSH (EventType::ET_ARRANGER_SELECTIONS_CHANGED, sel); */

      first_run_ = false;
    },
    *sel_, *sel_after_);
}

void
ArrangerSelectionsAction::do_or_undo_merge (bool do_it)
{
  std::visit (
    [&] (auto &&sel, auto &&sel_after) {
      using SelT = base_type<decltype (sel)>;

      if constexpr (std::is_same_v<SelT, base_type<decltype (sel_after)>>)
        {
          /* if first run, merge */
          if (first_run_)
            {
              auto * clone = sel->clone_raw_ptr ();
              clone->setParent (this);
              if (sel_after_)
                {
                  std::get<SelT *> (sel_after_.value ())->deleteLater ();
                }
              sel_after_ = clone;
              sel_after = clone;
              sel_after->merge ();
            }

          sel->sort_by_indices (!do_it);
          sel_after->sort_by_indices (!do_it);

          auto &before_objs = do_it ? sel->objects_ : sel_after->objects_;
          auto &after_objs = do_it ? sel_after->objects_ : sel->objects_;

          /* remove the before objects from the project */
          for (
            auto &own_before_obj_var : std::ranges::reverse_view (before_objs))
            {
              std::visit (
                [&] (auto &&own_before_obj) {
                  using ObjT = base_type<decltype (own_before_obj)>;
                  own_before_obj->flags_ |= ArrangerObject::Flags::NonProject;

                  /* find the actual object */
                  auto prj_obj =
                    std::get<ObjT *> (*own_before_obj->find_in_project ());

                  /* remove */
                  prj_obj->remove_from_project (true);
                },
                own_before_obj_var);
            }

          /* add the after objects to the project */
          for (auto &own_after_obj_var : std::ranges::reverse_view (after_objs))
            {
              std::visit (
                [&] (auto &&own_after_obj) {
                  using ObjT = base_type<decltype (own_after_obj)>;
                  own_after_obj->flags_ |= ArrangerObject::Flags::NonProject;

                  auto prj_obj = std::get<ObjT *> (
                    own_after_obj->add_clone_to_project (false));

                  /* remember positions */
                  own_after_obj->copy_identifier (*prj_obj);
                },
                own_after_obj_var);
            }

          // ArrangerSelections * sel = get_actual_arranger_selections ();
          /* EVENTS_PUSH (EventType::ET_ARRANGER_SELECTIONS_CHANGED, sel); */

          first_run_ = false;
        }
    },
    *sel_, *sel_after_);
}

void
ArrangerSelectionsAction::do_or_undo_resize (bool do_it)
{
  bool sel_after_existed = sel_after_.has_value ();
  if (!sel_after_.has_value ())
    {
      std::visit (
        [&] (auto &&sel) {
          /* create the "after" selections here if not already given (e.g., when
           * the objects are already edited) */
          set_after_selections (*sel);
        },
        *sel_);
    }

  std::visit (
    [&] (auto &&sel, auto &&sel_after) {
      // using SelT = base_type<decltype (sel)>;

      auto &objs_before = sel->objects_;
      auto &objs_after = sel_after->objects_;

      double ticks = do_it ? ticks_ : -ticks_;

      /* if objects are already edited and this is the first run nothing needs
       * to be done */
      if (!sel_after_existed || !first_run_)
        {
          for (size_t i = 0; i < objs_before.size (); i++)
            {
              auto &own_obj_before_var = objs_before.at (i);
              auto &own_obj_after_var = objs_after.at (i);
              std::visit (
                [&] (auto &&own_obj_before, auto &&own_obj_after) {
                  using ObjT = base_type<decltype (own_obj_before)>;
                  if constexpr (
                    std::is_same_v<ObjT, base_type<decltype (own_obj_after)>>)
                    {
                      own_obj_before->flags_ |=
                        ArrangerObject::Flags::NonProject;
                      own_obj_after->flags_ |= ArrangerObject::Flags::NonProject;

                      /* find the actual object */
                      auto obj = std::get<ObjT *> (*(
                        do_it ? own_obj_before->find_in_project ()
                              : own_obj_after->find_in_project ()));

                      auto type = ArrangerObject::ResizeType::Normal;
                      bool left = false;
                      switch (resize_type_)
                        {
                        case ResizeType::L:
                          type = ArrangerObject::ResizeType::Normal;
                          left = true;
                          break;
                        case ResizeType::LStretch:
                          type = ArrangerObject::ResizeType::Stretch;
                          left = true;
                          break;
                        case ResizeType::LLoop:
                          left = true;
                          type = ArrangerObject::ResizeType::Loop;
                          break;
                        case ResizeType::LFade:
                          left = true;
                          type = ArrangerObject::ResizeType::Fade;
                          break;
                        case ResizeType::R:
                          type = ArrangerObject::ResizeType::Normal;
                          break;
                        case ResizeType::RStretch:
                          type = ArrangerObject::ResizeType::Stretch;
                          break;
                        case ResizeType::RLoop:
                          type = ArrangerObject::ResizeType::Loop;
                          break;
                        case ResizeType::RFade:
                          type = ArrangerObject::ResizeType::Fade;
                          break;
                        default:
                          z_warn_if_reached ();
                        }

                      /* on first do, resize both the project object and our own
                       * "after" object */
                      if (do_it && first_run_)
                        {
                          if constexpr (
                            std::derived_from<ObjT, LengthableObject>)
                            {
                              obj->resize (left, type, ticks, false);
                              own_obj_after->resize (left, type, ticks, false);
                            }
                          else
                            {
                              z_return_if_reached ();
                            }

                        } /* endif do and first run */

                      /* remove the project object and add a clone of our
                       * corresponding object */
                      obj->remove_from_project (true);
                      obj = nullptr;
                      auto new_obj = std::get<ObjT *> (
                        (do_it ? own_obj_after : own_obj_before)
                          ->insert_clone_to_project ());

                      /* select it */
                      new_obj->select (true, true, false);
                    }
                },
                own_obj_before_var, own_obj_after_var);

            } /* endforeach object */
        }

      update_region_link_groups (do_it ? objs_after : objs_before);

      // ArrangerSelections * sel = get_actual_arranger_selections ();
      /* EVENTS_PUSH (EventType::ET_ARRANGER_SELECTIONS_CHANGED, sel); */
      /* EVENTS_PUSH (EventType::ET_ARRANGER_SELECTIONS_ACTION_FINISHED, sel); */

      first_run_ = false;
    },
    *sel_, *sel_after_);
}

void
ArrangerSelectionsAction::do_or_undo_quantize (bool do_it)
{
  std::visit (
    [&] (auto &&sel, auto &&sel_after) {
      // using SelT = base_type<decltype (sel)>;
      auto &objs = sel->objects_;
      auto &quantized_objs = sel_after->objects_;

      for (size_t i = 0; i < objs.size (); i++)
        {
          auto &own_obj_var = objs.at (i);
          auto &own_quantized_obj_var = quantized_objs.at (i);
          std::visit (
            [&] (auto &&own_obj, auto &&own_quantized_obj) {
              using ObjT = base_type<decltype (own_obj)>;
              if constexpr (
                std::is_same_v<ObjT, base_type<decltype (own_quantized_obj)>>)
                {
                  own_obj->flags_ |= ArrangerObject::Flags::NonProject;
                  own_quantized_obj->flags_ |= ArrangerObject::Flags::NonProject;

                  /* find the actual object */
                  auto obj = std::get<ObjT *> (*(
                    do_it ? own_obj->find_in_project ()
                          : own_quantized_obj->find_in_project ()));

                  if (do_it)
                    {
                      /* quantize it */
                      if (opts_->adj_start_)
                        {
                          double ticks = opts_->quantize_position (obj->pos_);
                          if constexpr (
                            std::derived_from<ObjT, LengthableObject>)
                            {
                              obj->end_pos_->add_ticks (ticks, AUDIO_ENGINE->frames_per_tick_);
                            }
                        }
                      if (opts_->adj_end_)
                        {
                          if constexpr (
                            std::derived_from<ObjT, LengthableObject>)
                            {
                              opts_->quantize_position (obj->end_pos_);
                            }
                        }
                      obj->pos_setter (obj->pos_);
                      if constexpr (std::derived_from<ObjT, LengthableObject>)
                        {
                          obj->end_pos_setter (obj->end_pos_);
                        }

                      /* remember the quantized position so we can find the
                       * object when undoing */
                      own_quantized_obj->pos_->set_to_pos (*obj->pos_);
                      if constexpr (std::derived_from<ObjT, LengthableObject>)
                        {
                          own_quantized_obj->end_pos_->set_to_pos (
                            *obj->end_pos_);
                        }
                    }
                  else
                    {
                      /* unquantize it */
                      obj->pos_setter (own_obj->pos_);
                      if constexpr (std::derived_from<ObjT, LengthableObject>)
                        {
                          obj->end_pos_setter (own_obj->end_pos_);
                        }
                    }
                }
            },
            own_obj_var, own_quantized_obj_var);
        }

      // ArrangerSelections * sel = get_actual_arranger_selections ();
      /* EVENTS_PUSH (EventType::ET_ARRANGER_SELECTIONS_QUANTIZED, sel); */

      first_run_ = false;
    },
    *sel_, *sel_after_);
}

void
ArrangerSelectionsAction::do_or_undo (bool do_it)
{
  switch (type_)
    {
    case Type::Create:
      do_or_undo_create_or_delete (do_it, true);
      break;
    case Type::Delete:
      do_or_undo_create_or_delete (do_it, false);
      break;
    case Type::Duplicate:
      do_or_undo_duplicate_or_link (false, do_it);
      break;
    case Type::Move:
      do_or_undo_move (do_it);
      break;
    case Type::Link:
      do_or_undo_duplicate_or_link (true, do_it);
      break;
    case Type::Record:
      do_or_undo_record (do_it);
      break;
    case Type::Edit:
      do_or_undo_edit (do_it);
      break;
    case Type::AutomationFill:
      do_or_undo_automation_fill (do_it);
      break;
    case Type::Split:
      do_or_undo_split (do_it);
      break;
    case Type::Merge:
      do_or_undo_merge (do_it);
      break;
    case Type::Resize:
      do_or_undo_resize (do_it);
      break;
    case Type::Quantize:
      do_or_undo_quantize (do_it);
      break;
    default:
      z_return_if_reached ();
      break;
    }

  /* update playback caches */
  TRACKLIST->set_caches (CacheType::PlaybackSnapshots);

  /* reset new_lane_created */
  for (auto track : TRACKLIST->tracks_ | type_is<LanedTrack> ())
    {
      track->last_lane_created_ = 0;
      track->block_auto_creation_and_deletion_ = false;
      std::visit (
        [&] (auto &&t) {
          t->create_missing_lanes (t->lanes_.size () - 1);
          t->remove_empty_last_lanes ();
        },
        convert_to_variant<LanedTrackPtrVariant> (track));
    }

  /* this is only needed in a few cases but it's cheap so send the event here
   * anyway */
  /* EVENTS_PUSH (EventType::ET_TRACK_LANES_VISIBILITY_CHANGED, nullptr); */
}

void
ArrangerSelectionsAction::perform_impl ()
{
  do_or_undo (true);
}

void
ArrangerSelectionsAction::undo_impl ()
{
  do_or_undo (false);
}

bool
ArrangerSelectionsAction::contains_clip (const AudioClip &clip) const
{
  if (
    sel_
    && std::visit (
      [&] (auto &&sel) -> bool { return sel->contains_clip (clip); },
      sel_.value ()))
    {
      return true;
    }
  if (
    sel_after_
    && std::visit (
      [&] (auto &&sel) { return sel->contains_clip (clip); },
      sel_after_.value ()))
    {
      return true;
    }

  /* check split regions (if any) */
  for (size_t i = 0; i < num_split_objs_; i++)
    {
      auto r1 = std::get<AudioRegion *> (r1_.at (i));
      auto r2 = std::get<AudioRegion *> (r2_.at (i));
      if (r1 && r2)
        {
          if (r1->pool_id_ == clip.pool_id_ || r2->pool_id_ == clip.pool_id_)
            {
              return true;
            }
        }
    }

  return false;
}

QString
ArrangerSelectionsAction::to_string () const
{
  switch (type_)
    {
    case Type::Create:
      z_return_val_if_fail (sel_, {});
      return std::visit (
        [&] (auto &&sel) -> QString {
          using SelT = base_type<decltype (sel)>;
          if constexpr (std::is_same_v<SelT, TimelineSelections>)
            {
              return QObject::tr ("Create timeline selections");
            }
          else if constexpr (std::is_same_v<SelT, AudioSelections>)
            {
              return QObject::tr ("Create audio selections");
            }
          else if constexpr (std::is_same_v<SelT, AutomationSelections>)
            {
              return QObject::tr ("Create automation selections");
            }
          else if constexpr (std::is_same_v<SelT, ChordSelections>)
            {
              return QObject::tr ("Create chord selections");
            }
          else if constexpr (std::is_same_v<SelT, MidiSelections>)
            {
              return QObject::tr ("Create MIDI selections");
            }
          else
            {
              z_return_val_if_reached ({});
            }
        },
        sel_.value ());
    case Type::Delete:
      return QObject::tr ("Delete arranger selections");
    case Type::Duplicate:
      return QObject::tr ("Duplicate arranger selections");
    case Type::Move:
      return QObject::tr ("Move arranger selections");
    case Type::Link:
      return QObject::tr ("Link arranger selections");
    case Type::Record:
      return QObject::tr ("Record arranger selections");
    case Type::Edit:
      return QObject::tr ("Edit arranger selections");
    case Type::AutomationFill:
      return QObject::tr ("Automation fill");
    case Type::Split:
      return QObject::tr ("Split arranger selections");
    case Type::Merge:
      return QObject::tr ("Merge arranger selections");
    case Type::Resize:
      {
        return format_qstr (
          QObject::tr ("Resize arranger selections - {}"), resize_type_);
      }
    case Type::Quantize:
      return QObject::tr ("Quantize arranger selections");
    default:
      break;
    }

  z_return_val_if_reached ({});
}

template CreateArrangerSelectionsAction::CreateArrangerSelectionsAction (
  const TimelineSelections &sel);
template CreateArrangerSelectionsAction::CreateArrangerSelectionsAction (
  const MidiSelections &sel);
template CreateArrangerSelectionsAction::CreateArrangerSelectionsAction (
  const AutomationSelections &sel);
template CreateArrangerSelectionsAction::CreateArrangerSelectionsAction (
  const ChordSelections &sel);
template CreateArrangerSelectionsAction::CreateArrangerSelectionsAction (
  const AudioSelections &sel);

template DeleteArrangerSelectionsAction::DeleteArrangerSelectionsAction (
  const TimelineSelections &sel);
template DeleteArrangerSelectionsAction::DeleteArrangerSelectionsAction (
  const MidiSelections &sel);
template DeleteArrangerSelectionsAction::DeleteArrangerSelectionsAction (
  const AutomationSelections &sel);
template DeleteArrangerSelectionsAction::DeleteArrangerSelectionsAction (
  const ChordSelections &sel);
template DeleteArrangerSelectionsAction::DeleteArrangerSelectionsAction (
  const AudioSelections &sel);

template ArrangerSelectionsAction::RecordAction::RecordAction (
  const TimelineSelections &sel_before,
  const TimelineSelections &sel_after,
  bool                      already_recorded);
template ArrangerSelectionsAction::RecordAction::RecordAction (
  const MidiSelections &sel_before,
  const MidiSelections &sel_after,
  bool                  already_recorded);
template ArrangerSelectionsAction::RecordAction::RecordAction (
  const AutomationSelections &sel_before,
  const AutomationSelections &sel_after,
  bool                        already_recorded);
template ArrangerSelectionsAction::RecordAction::RecordAction (
  const ChordSelections &sel_before,
  const ChordSelections &sel_after,
  bool                   already_recorded);
template ArrangerSelectionsAction::RecordAction::RecordAction (
  const AudioSelections &sel_before,
  const AudioSelections &sel_after,
  bool                   already_recorded);

template ArrangerSelectionsAction::ResizeAction::ResizeAction (
  const TimelineSelections  &sel_before,
  const TimelineSelections * sel_after,
  ResizeType                 type,
  double                     ticks);
template ArrangerSelectionsAction::ResizeAction::ResizeAction (
  const MidiSelections  &sel_before,
  const MidiSelections * sel_after,
  ResizeType             type,
  double                 ticks);
template ArrangerSelectionsAction::ResizeAction::ResizeAction (
  const AutomationSelections  &sel_before,
  const AutomationSelections * sel_after,
  ResizeType                   type,
  double                       ticks);
template ArrangerSelectionsAction::ResizeAction::ResizeAction (
  const ChordSelections  &sel_before,
  const ChordSelections * sel_after,
  ResizeType              type,
  double                  ticks);
template ArrangerSelectionsAction::ResizeAction::ResizeAction (
  const AudioSelections  &sel_before,
  const AudioSelections * sel_after,
  ResizeType              type,
  double                  ticks);

template ArrangerSelectionsAction::QuantizeAction::QuantizeAction (
  const TimelineSelections &sel,
  const dsp::QuantizeOptions    &opts);
template ArrangerSelectionsAction::QuantizeAction::QuantizeAction (
  const MidiSelections  &sel,
  const dsp::QuantizeOptions &opts);
template ArrangerSelectionsAction::QuantizeAction::QuantizeAction (
  const AutomationSelections &sel,
  const dsp::QuantizeOptions      &opts);
template ArrangerSelectionsAction::QuantizeAction::QuantizeAction (
  const ChordSelections &sel,
  const dsp::QuantizeOptions &opts);
template ArrangerSelectionsAction::QuantizeAction::QuantizeAction (
  const AudioSelections &sel,
  const dsp::QuantizeOptions &opts);

template std::unique_ptr<EditArrangerSelectionsAction>
EditArrangerSelectionsAction::create (
  const MidiRegion &obj_before,
  const MidiRegion &obj_after,
  EditType          type,
  bool              already_edited);
template std::unique_ptr<EditArrangerSelectionsAction>
EditArrangerSelectionsAction::create (
  const AudioRegion &obj_before,
  const AudioRegion &obj_after,
  EditType           type,
  bool               already_edited);
template std::unique_ptr<EditArrangerSelectionsAction>
EditArrangerSelectionsAction::create (
  const ChordRegion &obj_before,
  const ChordRegion &obj_after,
  EditType           type,
  bool               already_edited);
template std::unique_ptr<EditArrangerSelectionsAction>
EditArrangerSelectionsAction::create (
  const AutomationRegion &obj_before,
  const AutomationRegion &obj_after,
  EditType                type,
  bool                    already_edited);
template std::unique_ptr<EditArrangerSelectionsAction>
EditArrangerSelectionsAction::create (
  const Marker &obj_before,
  const Marker &obj_after,
  EditType      type,
  bool          already_edited);
