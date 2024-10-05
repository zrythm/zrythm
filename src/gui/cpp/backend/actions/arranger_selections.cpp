// SPDX-FileCopyrightText: © 2019-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "common/dsp/audio_region.h"
#include "common/dsp/automation_region.h"
#include "common/dsp/automation_track.h"
#include "common/dsp/chord_track.h"
#include "common/dsp/control_port.h"
#include "common/dsp/laned_track.h"
#include "common/dsp/marker_track.h"
#include "common/dsp/port_identifier.h"
#include "common/dsp/router.h"
#include "common/dsp/track.h"
#include "common/dsp/tracklist.h"
#include "common/utils/gtest_wrapper.h"
#include "common/utils/math.h"
#include "gui/cpp/backend/actions/arranger_selections.h"
#include "gui/cpp/backend/arranger_selections.h"
#include "gui/cpp/backend/event.h"
#include "gui/cpp/backend/event_manager.h"
#include "gui/cpp/backend/project.h"
#include "gui/cpp/backend/settings/g_settings_manager.h"
#include "gui/cpp/gtk_widgets/arranger.h"
#include "gui/cpp/gtk_widgets/zrythm_app.h"

#include <glib/gi18n.h>

ArrangerSelectionsAction::ArrangerSelectionsAction ()
    : UndoableAction (UndoableAction::Type::ArrangerSelections)
{
}

ArrangerSelectionsAction::ArrangerSelectionsAction (
  const ArrangerSelections &sel,
  Type                      type)
    : UndoableAction (UndoableAction::Type::ArrangerSelections), type_ (type),
      first_run_ (true)
{
  set_before_selections (sel);
}

CreateOrDeleteArrangerSelectionsAction::CreateOrDeleteArrangerSelectionsAction (
  const ArrangerSelections &sel,
  bool                      create)
    : ArrangerSelectionsAction (sel, create ? Type::Create : Type::Delete)
{
  z_return_if_fail (sel.has_any ());

  if (sel.contains_undeletable_object ())
    {
      throw ZrythmException (
        _ ("Arranger selections contain an undeletable object"));
    }
}

void
ArrangerSelectionsAction::init_after_cloning (
  const ArrangerSelectionsAction &other)
{
  UndoableAction::copy_members_from (other);
  type_ = other.type_;
  if (other.sel_)
    sel_ =
      clone_unique_with_variant<ArrangerSelectionsVariant> (other.sel_.get ());
  if (other.sel_after_)
    sel_after_ = clone_unique_with_variant<ArrangerSelectionsVariant> (
      other.sel_after_.get ());
  edit_type_ = other.edit_type_;
  ticks_ = other.ticks_;
  delta_tracks_ = other.delta_tracks_;
  delta_lanes_ = other.delta_lanes_;
  delta_chords_ = other.delta_chords_;
  delta_pitch_ = other.delta_pitch_;
  delta_vel_ = other.delta_vel_;
  delta_normalized_amount_ = other.delta_normalized_amount_;
  if (other.target_port_)
    target_port_ = std::make_unique<PortIdentifier> (*other.target_port_);
  str_ = other.str_;
  pos_ = other.pos_;
  for (auto &r : other.r1_)
    {
      r1_.push_back (
        clone_unique_with_variant<LengthableObjectVariant> (r.get ()));
    }
  for (auto &r : other.r2_)
    {
      r2_.push_back (
        clone_unique_with_variant<LengthableObjectVariant> (r.get ()));
    }
  num_split_objs_ = other.num_split_objs_;
  first_run_ = other.first_run_;
  if (other.opts_)
    opts_ = std::make_unique<QuantizeOptions> (*other.opts_);
  if (other.region_before_)
    region_before_ =
      clone_unique_with_variant<RegionVariant> (other.region_before_.get ());
  if (other.region_after_)
    region_after_ =
      clone_unique_with_variant<RegionVariant> (other.region_after_.get ());
  resize_type_ = other.resize_type_;
}

void
ArrangerSelectionsAction::init_loaded_impl ()
{
  if (sel_)
    {
      sel_->init_loaded (false, this);
    };
  if (sel_after_)
    {
      sel_after_->init_loaded (false, this);
    }

  z_return_if_fail (num_split_objs_ == r1_.size ());
  z_return_if_fail (num_split_objs_ == r2_.size ());
  for (size_t j = 0; j < num_split_objs_; j++)
    {
      if (r1_[j])
        {
          r1_[j]->init_loaded ();
          r2_[j]->init_loaded ();
        }
    }

  if (region_before_)
    {
      region_before_->init_loaded ();
    }
  if (region_after_)
    {
      region_after_->init_loaded ();
    }
}

void
ArrangerSelectionsAction::set_before_selections (const ArrangerSelections &src)
{
  sel_ = clone_unique_with_variant<ArrangerSelectionsVariant> (&src);

  if (ZRYTHM_TESTING)
    {
      src.validate ();
      sel_->validate ();
    }
}

void
ArrangerSelectionsAction::set_after_selections (const ArrangerSelections &src)
{
  sel_after_ = clone_unique_with_variant<ArrangerSelectionsVariant> (&src);

  if (ZRYTHM_TESTING)
    {
      src.validate ();
      sel_after_->validate ();
    }
}

ArrangerSelections *
ArrangerSelectionsAction::get_actual_arranger_selections () const
{
  return ArrangerSelections::get_for_type (sel_->type_);
}

ArrangerSelectionsAction::MoveOrDuplicateAction::MoveOrDuplicateAction (
  const ArrangerSelections &sel,
  bool                      move,
  double                    ticks,
  int                       delta_chords,
  int                       delta_pitch,
  int                       delta_tracks,
  int                       delta_lanes,
  double                    delta_normalized_amount,
  const PortIdentifier *    tgt_port_id,
  bool                      already_moved)
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
          throw ZrythmException (_ (
            "Arranger selections contain an object that cannot be duplicated"));
        }
    }

  if (!move)
    {
      set_after_selections (sel);
    }

  if (tgt_port_id)
    {
      target_port_ = std::make_unique<PortIdentifier> (*tgt_port_id);
    }
}

ArrangerSelectionsAction::LinkAction::LinkAction (
  const ArrangerSelections &sel_before,
  const ArrangerSelections &sel_after,
  double                    ticks,
  int                       delta_tracks,
  int                       delta_lanes,
  bool                      already_moved)
    : ArrangerSelectionsAction (sel_before, Type::Link)
{
  first_run_ = already_moved;
  ticks_ = ticks;
  delta_tracks_ = delta_tracks;
  delta_lanes_ = delta_lanes;

  set_after_selections (sel_after);
}

ArrangerSelectionsAction::RecordAction::RecordAction (
  const ArrangerSelections &sel_before,
  const ArrangerSelections &sel_after,
  bool                      already_recorded)
    : ArrangerSelectionsAction (sel_before, Type::Record)
{
  first_run_ = !already_recorded;
  set_after_selections (sel_after);
}

EditArrangerSelectionsAction::EditArrangerSelectionsAction (
  const ArrangerSelections  &sel_before,
  const ArrangerSelections * sel_after,
  EditType                   type,
  bool                       already_edited)
    : ArrangerSelectionsAction (sel_before, Type::Edit)
{
  first_run_ = already_edited;
  edit_type_ = type;
  if (type == EditType::Name && sel_before.contains_unrenamable_object ())
    {
      throw ZrythmException (_ ("Cannot rename selected object(s)"));
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

std::unique_ptr<EditArrangerSelectionsAction>
EditArrangerSelectionsAction::create (
  const ArrangerObject &obj_before,
  const ArrangerObject &obj_after,
  EditType              type,
  bool                  already_edited)
{
  auto sel_before = obj_before.create_arranger_selections_from_this ();
  auto sel_after = obj_after.create_arranger_selections_from_this ();
  return std::make_unique<EditArrangerSelectionsAction> (
    *sel_before, sel_after.get (), type, already_edited);
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
  const AudioSelections &sel_before,
  AudioFunctionType      audio_func_type,
  AudioFunctionOpts      opts,
  const std::string *    uri)
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

ArrangerSelectionsAction::SplitAction::SplitAction (
  const ArrangerSelections &sel,
  Position                  pos)
    : ArrangerSelectionsAction (sel, Type::Split)
{
  pos_ = pos;
  num_split_objs_ = sel.get_num_objects ();
  pos_.update_frames_from_ticks (0.0);
}

ArrangerSelectionsAction::ResizeAction::ResizeAction (
  const ArrangerSelections  &sel_before,
  const ArrangerSelections * sel_after,
  ResizeType                 type,
  double                     ticks)
    : ArrangerSelectionsAction (sel_before, Type::Resize)
{
  resize_type_ = type;
  ticks_ = ticks;
  /* validate */
  bool have_unresizable = sel_before.contains_object_with_property (
    ArrangerSelections::Property::HasLength, false);
  if (have_unresizable)
    {
      throw ZrythmException (_ ("Attempted to resize unresizable objects"));
    }

  bool have_looped = sel_before.contains_object_with_property (
    ArrangerSelections::Property::HasLooped, true);
  if (
    have_looped
    && (type == ResizeType::L || type == ResizeType::R || type == ResizeType::LStretch || type == ResizeType::RStretch))
    {
      throw ZrythmException (format_str (
        _ ("Cannot perform {} resize - selections contain looped objects"),
        type));
    }

  bool have_unloopable = sel_before.contains_object_with_property (
    ArrangerSelections::Property::CanLoop, false);
  if (
    have_unloopable && (type == ResizeType::LLoop || type == ResizeType::RLoop))
    {
      throw ZrythmException (format_str (
        _ ("Cannot perform {} resize - selections contain unloopable objects"),
        type));
    }

  if (sel_after)
    {
      set_after_selections (*sel_after);
    }
}

ArrangerSelectionsAction::AutomationFillAction::AutomationFillAction (
  const Region &region_before,
  const Region &region_after,
  bool          already_changed)
    : ArrangerSelectionsAction ()
{
  type_ = Type::AutomationFill;
  region_before_ = clone_unique_with_variant<RegionVariant> (&region_before);
  region_after_ = clone_unique_with_variant<RegionVariant> (&region_after);
  first_run_ = already_changed;
}

void
ArrangerSelectionsAction::update_region_link_groups (const auto &objects)
{
  /* handle children of linked regions */
  for (auto &_obj : objects)
    {
      /* get the actual object from the project */
      const auto obj = _obj->find_in_project ();
      z_return_if_fail (obj);

      if (obj->owned_by_region ())
        {
          std::visit (
            [&] (auto &&owned_obj) {
              Region * region = owned_obj->get_region ();
              z_return_if_fail (region);

              /* shift all linked objects */
              region->update_link_group ();
            },
            convert_to_variant<RegionOwnedObjectPtrVariant> (obj.get ()));
        }
    }
}

void
ArrangerSelectionsAction::do_or_undo_move (bool do_it)
{
  sel_->sort_by_indices (!do_it);

  double ticks = do_it ? ticks_ : -ticks_;
  int    delta_tracks = do_it ? delta_tracks_ : -delta_tracks_;
  int    delta_lanes = do_it ? delta_lanes_ : -delta_lanes_;
  int    delta_chords = do_it ? delta_chords_ : -delta_chords_;
  int    delta_pitch = do_it ? delta_pitch_ : -delta_pitch_;
  double delta_normalized_amt =
    do_it ? delta_normalized_amount_ : -delta_normalized_amount_;

  /* this is used for automation points to keep track of which automation point
   * in the project matches which automation point in the cached selections key:
   * project object, value: own object (from sel_->objects_) */
  std::unordered_map<AutomationPoint *, AutomationPoint *> obj_map;

  if (!first_run_)
    {
      for (auto &own_obj : sel_->objects_)
        {
          std::visit (
            [&] (auto &&own_obj_ptr) {
              using ObjT = base_type<decltype (own_obj_ptr)>;
              own_obj_ptr->flags_ |= ArrangerObject::Flags::NonProject;

              /* get the actual object from the project */
              auto prj_obj = std::dynamic_pointer_cast<ObjT> (
                own_obj_ptr->find_in_project ());
              z_return_if_fail (prj_obj);

              /* remember if automation point */
              if constexpr (std::is_same_v<ObjT, AutomationPoint>)
                {
                  obj_map[prj_obj.get ()] = own_obj_ptr;
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
                    *prj_obj, delta_tracks, delta_lanes, false, -1);

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
                      auto port =
                        Port::find_from_identifier<ControlPort> (*target_port_);
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

                      target_port_ =
                        std::make_unique<PortIdentifier> (cur_at->port_id_);
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
            convert_to_variant<ArrangerObjectPtrVariant> (own_obj.get ()));
        }

      /* if moving automation points, re-sort the region and remember the new
       * indices */
      auto first_own_obj = sel_->objects_[0].get ();
      if (first_own_obj->type_ == ArrangerObject::Type::AutomationPoint)
        {
          auto obj = dynamic_pointer_cast<AutomationPoint> (
            first_own_obj->find_in_project ());
          z_return_if_fail (obj);

          auto region = dynamic_cast<AutomationRegion *> (obj->get_region ());
          z_return_if_fail (region);
          region->force_sort ();

          for (auto &[prj_ap, cached_ap] : obj_map)
            {
              auto * prj_ap_cast = dynamic_cast<AutomationPoint *> (prj_ap);
              auto * cached_ap_cast =
                dynamic_cast<AutomationPoint *> (cached_ap);
              cached_ap_cast->set_region_and_index (
                *region, prj_ap_cast->index_);
            }
        }
    }

  update_region_link_groups (sel_->objects_);

  /* validate */
  CLIP_EDITOR->get_region ();

  ArrangerSelections * sel = get_actual_arranger_selections ();
  EVENTS_PUSH (EventType::ET_ARRANGER_SELECTIONS_CHANGED, sel);
  EVENTS_PUSH (EventType::ET_ARRANGER_SELECTIONS_MOVED, sel);

  first_run_ = false;
}

void
ArrangerSelectionsAction::move_obj_by_tracks_and_lanes (
  ArrangerObject &obj,
  const int       tracks_diff,
  const int       lanes_diff,
  bool            use_index_in_prev_lane,
  int             index_in_prev_lane)
{
  z_debug (
    "Moving object {}: tracks_diff={}, lanes_diff={}", fmt::ptr (&obj),
    tracks_diff, lanes_diff);

  if (tracks_diff)
    {
      auto r = dynamic_cast<Region *> (&obj);
      z_return_if_fail (r);

      const auto * track_before = r->get_track ();
      auto         track_to_move_to = TRACKLIST->get_visible_track_after_delta (
        *r->get_track (), tracks_diff);
      z_trace (
        "Moving from track {} ({}) to track: {} ({})",
        track_before->get_name (), track_before->get_name_hash (),
        track_to_move_to->get_name (), track_to_move_to->get_name_hash ());

      /* shift the actual object by tracks */
      if (ENUM_BITSET_TEST (
            ArrangerObjectFlags, obj.flags_, ArrangerObject::Flags::NonProject))
        {
          r->id_.track_name_hash_ = track_to_move_to->get_name_hash ();
          r->track_name_hash_ = track_to_move_to->get_name_hash ();
          z_trace ("Updated track name hash for non-project object");
        }
      else
        {
          std::visit (
            [&] (auto &&casted_r) {
              casted_r->move_to_track (
                track_to_move_to, -1,
                use_index_in_prev_lane ? index_in_prev_lane : -1);
              z_trace ("Moved project object to track");
            },
            convert_to_variant<RegionPtrVariant> (r));
        }
    }
  if (lanes_diff)
    {
      auto r = dynamic_cast<Region *> (&obj);
      z_return_if_fail (r);

      int new_lane_pos = r->id_.lane_pos_ + lanes_diff;
      z_return_if_fail (new_lane_pos >= 0);
      z_trace ("New lane position: {}", new_lane_pos);

      /* shift the actual object by lanes */
      if (
        ENUM_BITSET_TEST (
          ArrangerObject::Flags, r->flags_, ArrangerObject::Flags::NonProject))
        {
          r->id_.lane_pos_ = new_lane_pos;
          z_trace ("Updated lane position for non-project object");
        }
      else
        {
          std::visit (
            [&] (auto &&casted_r) {
              using RegionT = base_type<decltype (casted_r)>;
              if constexpr (std::derived_from<RegionT, LaneOwnedObject>)
                {
                  auto r_track =
                    casted_r->template get_track_as<LanedTrackImpl<RegionT>> ();
                  r_track->create_missing_lanes (new_lane_pos);
                  z_trace ("Created missing lanes up to {}", new_lane_pos);
                  casted_r->move_to_track (
                    r_track, new_lane_pos,
                    use_index_in_prev_lane ? index_in_prev_lane : -1);
                  z_trace ("Moved project object to new lane");
                }
            },
            convert_to_variant<RegionPtrVariant> (r));
        }
    }
  z_debug ("Object move completed");
}

void
ArrangerSelectionsAction::do_or_undo_duplicate_or_link (bool link, bool do_it)
{
  sel_->sort_by_indices (!do_it);
  sel_after_->sort_by_indices (!do_it);
  if (ZRYTHM_TESTING)
    {
      sel_after_->validate ();
    }
  if (sel_after_->type_ == ArrangerSelections::Type::Timeline)
    {
      const auto * tl_sel_after =
        dynamic_cast<TimelineSelections *> (sel_after_.get ());
      z_debug ("{}", *tl_sel_after);
    }
  ArrangerSelections * sel = get_actual_arranger_selections ();
  z_return_if_fail (sel);

  const double ticks = do_it ? ticks_ : -ticks_;
  const int    delta_tracks = do_it ? delta_tracks_ : -delta_tracks_;
  const int    delta_lanes = do_it ? delta_lanes_ : -delta_lanes_;
  const int    delta_chords = do_it ? delta_chords_ : -delta_chords_;
  const int    delta_pitch = do_it ? delta_pitch_ : -delta_pitch_;
  const double delta_normalized_amount =
    do_it ? delta_normalized_amount_ : -delta_normalized_amount_;

  /* clear current selections in the project */
  sel->clear (false);

  /* this is used for automation points to keep track of which automation point
   * in the project matches which automation point in the cached selections
   * key: project automation point, value: cached automation point */
  std::map<AutomationPoint *, AutomationPoint *> ap_map;

  // this essentially sets the NonProject flag if not performing on first run
  for (
    auto it = sel_after_->objects_.rbegin ();
    it != sel_after_->objects_.rend (); ++it)
    {
      size_t i = std::distance (sel_after_->objects_.begin (), it.base ()) - 1;
      std::visit (
        [&] (auto &&unused) {
          using ObjT = base_type<decltype (unused)>;
          auto own_obj = std::dynamic_pointer_cast<ObjT> (*it);
          auto own_orig_obj =
            std::dynamic_pointer_cast<ObjT> (sel_->objects_.at (i));
          own_obj->flags_ |= ArrangerObject::Flags::NonProject;
          own_orig_obj->flags_ |= ArrangerObject::Flags::NonProject;

          /* on first run, we need to first move the original object backwards
           * (the project object too) */
          if (do_it && first_run_)
            {
              auto obj =
                std::dynamic_pointer_cast<ObjT> (own_obj->find_in_project ());
              z_return_if_fail (obj);

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
                        *obj, -delta_tracks_, -delta_lanes_, true,
                        own_obj->index_in_prev_lane_);

                      z_trace ("moving own obj");
                      RegionIdentifier own_id_before_move = own_obj->id_;
                      move_obj_by_tracks_and_lanes (
                        *own_obj, -delta_tracks_, -delta_lanes_, true,
                        own_obj->index_in_prev_lane_);

                      /* since the object moved outside of its lane, decrement
                       * the index inside the lane for all of our cached objects
                       * in the same lane */
                      for (
                        size_t j = i + 1; j < sel_after_->objects_.size (); j++)
                        {
                          if (
                            own_id_before_move.track_name_hash_
                              == own_obj->id_.track_name_hash_
                            && own_id_before_move.lane_pos_ == own_obj->id_.lane_pos_
                            && own_id_before_move.at_idx_ == own_obj->id_.at_idx_)
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
                        obj->normalized_val_ - (float) delta_normalized_amount_,
                        true, false);
                      own_obj->set_fvalue (
                        own_obj->normalized_val_
                          - (float) delta_normalized_amount_,
                        true, false);
                    }
                }
            } /* if do and first run */
        },
        convert_to_variant<ArrangerObjectPtrVariant> ((*it).get ()));
    }

  // debug
  if (sel_after_->type_ == ArrangerSelections::Type::Timeline)
    {
      const auto * tl_sel_after =
        dynamic_cast<TimelineSelections *> (sel_after_.get ());
      z_debug ("{}", *tl_sel_after);
    }

  for (size_t i = 0; i < sel_after_->objects_.size (); i++)
    {
      std::visit (
        [&] (auto &&unused) {
          using ObjT = base_type<decltype (unused)>;
          auto own_obj =
            std::dynamic_pointer_cast<ObjT> (sel_after_->objects_.at (i));
          auto own_orig_obj =
            std::dynamic_pointer_cast<ObjT> (sel_->objects_.at (i));

          if (do_it)
            {
              auto add_adjusted_clone_to_project = [] (const auto &obj_to_clone) {
                /* create a temporary clone */
                auto obj = obj_to_clone->clone_unique ();

                /* if region, clear the remembered index so that the region
                 * gets appended instead of inserted */
                if constexpr (RegionSubclass<ObjT>)
                  {
                    obj->id_.idx_ = -1;
                  }

                /* add to track. */
                return std::dynamic_pointer_cast<ObjT> (
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
                    *added_obj_ref, delta_tracks, 0, false, -1);
                  move_obj_by_tracks_and_lanes (
                    *own_obj, delta_tracks, 0, false, -1);
                }
              if (delta_lanes != 0)
                {
                  move_obj_by_tracks_and_lanes (
                    *added_obj_ref, 0, delta_lanes, false, -1);
                  move_obj_by_tracks_and_lanes (
                    *own_obj, 0, delta_lanes, false, -1);
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
                      const auto * port =
                        Port::find_from_identifier<ControlPort> (*target_port_);
                      z_return_if_fail (port);
                      auto * track = dynamic_cast<AutomatableTrack *> (
                        port->get_track (true));
                      z_return_if_fail (track);
                      const auto * at =
                        AutomationTrack::find_from_port (*port, track, true);
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
                      auto orig_r = dynamic_pointer_cast<ObjT> (
                        own_orig_obj->find_in_project ());
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
                          const auto id =
                            AUDIO_POOL->duplicate_clip (clip->pool_id_, true);
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
                  ap_map[added_obj_ref.get ()] = own_obj.get ();
                }

              /* select it */
              added_obj_ref->select (true, true, false);

              /* remember the identifier */
              own_obj->copy_identifier (*added_obj_ref);

            } /* endif do */
          else /* if undo */
            {
              /* find the actual object */
              auto obj =
                std::dynamic_pointer_cast<ObjT> (own_obj->find_in_project ());
              z_return_if_fail (obj);

              /* if the object was created with linking, delete the links */
              if constexpr (RegionSubclass<ObjT>)
                {
                  if (link)
                    {
                      /* remove link from created object (this will also
                       * automatically remove the link from the parent region if
                       * it is the only region in the link group) */
                      z_return_if_fail (obj->has_link_group ());
                      obj->unlink ();
                      z_return_if_fail (obj->id_.link_group_ == -1);

                      /* unlink remembered link groups */
                      own_orig_obj->unlink ();
                      own_obj->unlink ();
                    }
                }

              /* remove it */
              obj->remove_from_project ();

              /* set the copies back to original state */
              if (!math_doubles_equal (ticks, 0.0))
                {
                  own_obj->move (ticks);
                }
              if (delta_tracks != 0)
                {
                  move_obj_by_tracks_and_lanes (
                    *own_obj, delta_tracks, 0, false, -1);
                }
              if (delta_lanes != 0)
                {
                  move_obj_by_tracks_and_lanes (
                    *own_obj, 0, delta_lanes, false, -1);
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
        convert_to_variant<ArrangerObjectPtrVariant> (
          sel_after_->objects_.at (i).get ()));
    }

  /* if copy-moving automation points, re-sort the region and remember the new
   * indices */
  if (
    sel_after_->objects_.front ()->type_
    == ArrangerObject::Type::AutomationPoint)
    {
      /* get the actual object from the project */
      const auto * obj = ap_map.begin ()->first;
      if (obj != nullptr)
        {
          auto * region = dynamic_cast<AutomationRegion *> (obj->get_region ());
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

  sel = get_actual_arranger_selections ();
  if (do_it)
    {
      TRANSPORT->recalculate_total_bars (sel_after_.get ());

      EVENTS_PUSH (EventType::ET_ARRANGER_SELECTIONS_CREATED, sel);
    }
  else
    {
      EVENTS_PUSH (EventType::ET_ARRANGER_SELECTIONS_REMOVED, sel);
    }

  first_run_ = false;
}

void
ArrangerSelectionsAction::do_or_undo_create_or_delete (bool do_it, bool create)
{
  sel_->sort_by_indices (create ? !do_it : do_it);
  ArrangerSelections * sel = get_actual_arranger_selections ();
  z_return_if_fail (sel);

  if (!first_run_ || !create)
    {
      /* clear current selections in the project */
      sel->clear (false);

      for (auto &own_obj : sel_->objects_)
        {
          own_obj->flags_ |= ArrangerObject::Flags::NonProject;

          /* if doing in a create action or undoing in a delete action */
          if ((do_it && create) || (!do_it && !create))
            {
              /* add a clone to the project */
              ArrangerObject::ArrangerObjectPtr prj_obj;
              if (create)
                {
                  prj_obj = own_obj->add_clone_to_project (false);
                }
              else
                {
                  prj_obj = own_obj->insert_clone_to_project ();
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
              auto obj = own_obj->find_in_project ();
              z_return_if_fail (obj);

              /* if region, remove link */
              if (obj->type_ == ArrangerObject::Type::Region)
                {
                  auto region = dynamic_pointer_cast<Region> (obj);
                  std::visit (
                    [&] (auto &&casted_r) {
                      if (casted_r->has_link_group ())
                        {
                          casted_r->unlink ();
                        }
                    },
                    convert_to_variant<RegionPtrVariant> (region.get ()));
                }

              /* remove it */
              obj->remove_from_project ();
            }
        }
    }

  /* if first time creating the object, save the length for use by SnapGrid */
  if (
    ZRYTHM_HAVE_UI && first_run_ && create && do_it
    && sel_->objects_.size () == 1)
    {
      auto obj = sel_->objects_[0]->find_in_project ();
      z_return_if_fail (obj);
      if (obj->has_length ())
        {
          auto   obj_lo = dynamic_pointer_cast<LengthableObject> (obj);
          double ticks = obj_lo->get_length_in_ticks ();
          auto   arranger = obj->get_arranger ();
          if (arranger->type == ArrangerWidgetType::Timeline)
            {
              g_settings_set_double (S_UI, "timeline-last-object-length", ticks);
            }
          else
            {
              g_settings_set_double (S_UI, "editor-last-object-length", ticks);
            }
        }
    }

  /* if creating */
  if ((do_it && create) || (!do_it && !create))
    {
      update_region_link_groups (sel_->objects_);

      TRANSPORT->recalculate_total_bars (sel_.get ());

      EVENTS_PUSH (EventType::ET_ARRANGER_SELECTIONS_CREATED, sel);
    }
  /* if deleting */
  else
    {
      EVENTS_PUSH (EventType::ET_ARRANGER_SELECTIONS_REMOVED, sel);
    }

  P_MARKER_TRACK->validate ();
  P_CHORD_TRACK->validate ();
  REGION_LINK_GROUP_MANAGER.validate ();

  first_run_ = false;

  if (sel_->type_ == ArrangerSelections::Type::Timeline)
    {
      auto &ts = dynamic_cast<TimelineSelections &> (*sel_);
      bool  have_automation_region = false;
      for (auto &obj : ts.objects_)
        {
          if (obj->is_region ())
            {
              auto &r = dynamic_cast<Region &> (*obj);
              if (r.id_.type_ == RegionType::Automation)
                {
                  have_automation_region = true;
                  break;
                }
            }
        }

      if (have_automation_region)
        {
          ROUTER->recalc_graph (false);
        }
    }
}

void
ArrangerSelectionsAction::do_or_undo_record (bool do_it)
{
  sel_->sort_by_indices (!do_it);
  sel_after_->sort_by_indices (!do_it);
  ArrangerSelections * sel = get_actual_arranger_selections ();
  z_return_if_fail (sel);

  if (!first_run_)
    {
      /* clear current selections in the project */
      sel->clear (false);

      /* if do/redoing */
      if (do_it)
        {
          /* create the newly recorded objects */
          for (auto &own_after_obj : sel_after_->objects_)
            {
              own_after_obj->flags_ |= ArrangerObject::Flags::NonProject;

              /* add a clone to the project */
              auto prj_obj = own_after_obj->add_clone_to_project (false);

              /* select it */
              prj_obj->select (true, true, false);

              /* remember new info */
              own_after_obj->copy_identifier (*prj_obj);
            }

          /* delete the previous objects */
          for (auto &own_before_obj : sel_->objects_)
            {
              own_before_obj->flags_ |= ArrangerObject::Flags::NonProject;

              /* get the actual object from the project */
              auto obj = own_before_obj->find_in_project ();
              z_return_if_fail (obj);

              /* remove it */
              obj->remove_from_project ();
            }
        }

      /* if undoing */
      else
        {
          /* delete the newly recorded objects */
          for (auto &own_after_obj : sel_after_->objects_)
            {
              own_after_obj->flags_ |= ArrangerObject::Flags::NonProject;

              /* get the actual object from the project */
              auto obj = own_after_obj->find_in_project ();
              z_return_if_fail (obj);

              /* remove it */
              obj->remove_from_project ();
            }

          /* add the objects before the recording */
          for (auto &own_before_obj : sel_->objects_)
            {
              own_before_obj->flags_ |= ArrangerObject::Flags::NonProject;

              /* add a clone to the project */
              auto prj_obj = own_before_obj->add_clone_to_project (false);

              /* select it */
              prj_obj->select (true, true, false);

              /* remember new info */
              own_before_obj->copy_identifier (*prj_obj);
            }
        }
    }

  EVENTS_PUSH (EventType::ET_ARRANGER_SELECTIONS_CREATED, sel);
  EVENTS_PUSH (EventType::ET_ARRANGER_SELECTIONS_REMOVED, sel);

  first_run_ = false;

  if (sel_->type_ == ArrangerSelections::Type::Timeline)
    {
      auto &ts = dynamic_cast<TimelineSelections &> (*sel_);
      bool  have_automation_region = false;
      for (auto &obj : ts.objects_)
        {
          if (dynamic_cast<AutomationRegion *> (obj.get ()) != nullptr)
            {
              have_automation_region = true;
              break;
            }
        }

      if (have_automation_region)
        {
          ROUTER->recalc_graph (false);
        }
    }
}

void
ArrangerSelectionsAction::do_or_undo_edit (bool do_it)
{
  auto &src_sel = do_it ? sel_ : sel_after_;
  auto &dest_sel = do_it ? sel_after_ : sel_;

  if (!first_run_)
    {
      if (
        edit_type_ == EditType::EditorFunction
        && sel_->type_ == ArrangerSelections::Type::Audio)
        {
          auto src_audio_sel = dynamic_cast<AudioSelections *> (dest_sel.get ());
          z_return_if_fail (src_audio_sel);
          auto r = RegionImpl<AudioRegion>::find (src_audio_sel->region_id_);
          z_return_if_fail (r);
          auto src_clip = AUDIO_POOL->get_clip (src_audio_sel->pool_id_);
          z_return_if_fail (src_clip);

          /* adjust the positions */
          Position start = src_audio_sel->sel_start_;
          Position end = src_audio_sel->sel_end_;
          start.add_frames (-r->pos_.frames_);
          end.add_frames (-r->pos_.frames_);
          auto num_frames =
            static_cast<unsigned_frame_t> (end.frames_ - start.frames_);
          z_return_if_fail (num_frames == src_clip->num_frames_);

          auto src_clip_path = src_clip->get_path_in_pool (false);
          z_debug (
            "replacing audio region {} frames with {} frames", r->name_,
            src_clip_path);

          /* replace the frames in the region */
          r->replace_frames (
            src_clip->frames_.getReadPointer (0), start.frames_, num_frames,
            false);
        }
      else /* not audio function */
        {
          for (size_t i = 0; i < src_sel->objects_.size (); i++)
            {
              auto own_src_obj = src_sel->objects_[i].get ();
              auto own_dest_obj = dest_sel->objects_[i].get ();
              z_return_if_fail (own_src_obj);
              z_return_if_fail (own_dest_obj);
              own_src_obj->flags_ |= ArrangerObject::Flags::NonProject;
              own_dest_obj->flags_ |= ArrangerObject::Flags::NonProject;

              /* find the actual object */
              auto obj = own_src_obj->find_in_project ();
              z_return_if_fail (obj);

              /* change the parameter */
              switch (edit_type_)
                {
                case EditType::Name:
                  {
                    z_return_if_fail (
                      ArrangerObject::type_has_name (obj->type_));
                    auto name =
                      dynamic_cast<NameableObject *> (own_dest_obj)->name_;
                    dynamic_pointer_cast<NameableObject> (obj)->set_name (
                      name, false);
                  }
                  break;
                case EditType::Position:
                  obj->pos_ = own_dest_obj->pos_;
                  if (obj->has_length ())
                    {
                      auto obj_lo = dynamic_pointer_cast<LengthableObject> (obj);
                      auto own_dest_obj_lo =
                        dynamic_cast<LengthableObject *> (own_dest_obj);
                      obj_lo->end_pos_ = own_dest_obj_lo->end_pos_;
                    }
                  if (obj->can_loop ())
                    {
                      auto obj_lo = dynamic_pointer_cast<LoopableObject> (obj);
                      auto own_dest_obj_lo =
                        dynamic_cast<LoopableObject *> (own_dest_obj);
                      obj_lo->loop_start_pos_ = own_dest_obj_lo->loop_start_pos_;
                      obj_lo->loop_end_pos_ = own_dest_obj_lo->loop_end_pos_;
                      obj_lo->clip_start_pos_ = own_dest_obj_lo->clip_start_pos_;
                    }
                  break;
                case EditType::Fades:
                  if (obj->can_fade ())
                    {
                      auto obj_f = dynamic_pointer_cast<FadeableObject> (obj);
                      auto own_dest_obj_f =
                        dynamic_cast<FadeableObject *> (own_dest_obj);
                      obj_f->fade_in_pos_ = own_dest_obj_f->fade_in_pos_;
                      obj_f->fade_out_pos_ = own_dest_obj_f->fade_out_pos_;
                      obj_f->fade_in_opts_ = own_dest_obj_f->fade_in_opts_;
                      obj_f->fade_out_opts_ = own_dest_obj_f->fade_out_opts_;
                    }
                  break;
                case EditType::Primitive:
#define SET_PRIMITIVE(cc, member) \
  dynamic_pointer_cast<cc> (obj)->member = \
    dynamic_cast<cc *> (own_dest_obj)->member

                  switch (obj->type_)
                    {
                    case ArrangerObject::Type::Region:
                      {
                        SET_PRIMITIVE (MuteableObject, muted_);
                        SET_PRIMITIVE (Region, color_);
                        SET_PRIMITIVE (Region, use_color_);
                        SET_PRIMITIVE (AudioRegion, musical_mode_);
                        SET_PRIMITIVE (AudioRegion, gain_);
                      }
                      break;
                    case ArrangerObject::Type::MidiNote:
                      {
                        SET_PRIMITIVE (MidiNote, muted_);
                        SET_PRIMITIVE (MidiNote, val_);

                        /* set velocity and cache vel */
                        auto mn = dynamic_pointer_cast<MidiNote> (obj);
                        auto dest_mn = dynamic_cast<MidiNote *> (own_dest_obj);
                        mn->vel_->set_val (dest_mn->vel_->vel_);
                      }
                      break;
                    case ArrangerObject::Type::AutomationPoint:
                      {
                        SET_PRIMITIVE (AutomationPoint, curve_opts_);
                        SET_PRIMITIVE (AutomationPoint, fvalue_);
                        SET_PRIMITIVE (AutomationPoint, normalized_val_);
                      }
                      break;
                    default:
                      break;
                    }
                  break;
                case EditType::EditorFunction:
                  obj->pos_ = own_dest_obj->pos_;
                  SET_PRIMITIVE (LengthableObject, end_pos_);
                  if (obj->can_loop ())
                    {
                      SET_PRIMITIVE (LoopableObject, clip_start_pos_);
                      SET_PRIMITIVE (LoopableObject, loop_start_pos_);
                      SET_PRIMITIVE (LoopableObject, loop_end_pos_);
                    }
                  switch (obj->type_)
                    {
                    case ArrangerObject::Type::MidiNote:
                      {
                        SET_PRIMITIVE (MidiNote, muted_);
                        SET_PRIMITIVE (MidiNote, val_);

                        /* set velocity and cache vel */
                        auto mn = dynamic_pointer_cast<MidiNote> (obj);
                        auto dest_mn = dynamic_cast<MidiNote *> (own_dest_obj);
                        mn->vel_->set_val (dest_mn->vel_->vel_);
                      }
                      break;
                    case ArrangerObject::Type::AutomationPoint:
                      {
                        SET_PRIMITIVE (AutomationPoint, curve_opts_);
                        SET_PRIMITIVE (AutomationPoint, fvalue_);
                        SET_PRIMITIVE (AutomationPoint, normalized_val_);
                      }
                      break;
                    default:
                      break;
                    }
                  break;
                case EditType::Scale:
                  {
                    /* set the new scale */
                    SET_PRIMITIVE (ScaleObject, scale_);
                  }
                  break;
                case EditType::Mute:
                  {
                    /* set the new status */
                    SET_PRIMITIVE (MuteableObject, muted_);
                  }
                  break;
                default:
                  break;
                }
#undef SET_PRIMITIVE
            }
        } /* endif audio function */
    } /* endif not first run */

  update_region_link_groups (dest_sel->objects_);

  auto sel = get_actual_arranger_selections ();
  EVENTS_PUSH (EventType::ET_ARRANGER_SELECTIONS_CHANGED_REDRAW_EVERYTHING, sel);

  first_run_ = false;
}

void
ArrangerSelectionsAction::do_or_undo_automation_fill (bool do_it)
{
  if (!first_run_)
    {
      /* clear current selections in the project */
      TL_SELECTIONS->clear (false);

      /* get the actual object from the project */
      auto region = dynamic_pointer_cast<AutomationRegion> (
        do_it ? region_before_->find_in_project ()
              : region_after_->find_in_project ());
      z_return_if_fail (region);

      /* remove link */
      if (region->has_link_group ())
        {
          region->unlink ();

          /* unlink remembered link groups */
          auto &own_region = do_it ? region_before_ : region_after_;
          dynamic_cast<AutomationRegion *> (own_region.get ())->unlink ();
        }

      /* remove it */
      region->remove_from_project ();

      /* add a clone to the project */
      auto prj_obj =
        (do_it ? region_after_ : region_before_)->add_clone_to_project (false);

      /* select it */
      prj_obj->select (true, true, false);

      /* remember new info */
      do_it
        ? region_after_->copy_identifier (*prj_obj)
        : region_before_->copy_identifier (*prj_obj);
    }

  first_run_ = false;
}

void
ArrangerSelectionsAction::do_or_undo_split (bool do_it)
{
  int i = -1;
  for (auto &own_obj : sel_->objects_)
    {
      ++i;
      own_obj->flags_ |= ArrangerObject::Flags::NonProject;

      std::visit (
        [&] (auto &&own_obj_raw_ptr) {
          using T = base_type<decltype (own_obj_raw_ptr)>;

          if (do_it)
            {
              /* find the actual object */
              auto obj = dynamic_pointer_cast<T> (own_obj->find_in_project ());
              z_return_if_fail (obj);

              /* split */
              auto [r1, r2] = LengthableObject::split (*obj, pos_, false, true);

              /* r1 and r2 are now inside the project, clone them to keep copies
               */
              z_return_if_fail (r1 && r2);
              r1_[i] = r1->clone_unique ();
              r2_[i] = r2->clone_unique ();
            }
          /* else if undoing split */
          else
            {
              /* find the actual objects */
              auto r1 = dynamic_pointer_cast<T> (r1_[i]->find_in_project ());
              auto r2 = dynamic_pointer_cast<T> (r2_[i]->find_in_project ());
              z_return_if_fail (r1 && r2);

              /* unsplit */
              auto obj = LengthableObject::unsplit (*r1, *r2, false);

              if (obj->type_ == ArrangerObject::Type::Region)
                {
                  auto own_region = dynamic_cast<Region *> (own_obj.get ());
                  dynamic_pointer_cast<NameableObject> (obj)->set_name (
                    own_region->name_, false);
                }

              /* re-insert object at its original position */
              obj->remove_from_project ();
              own_obj->insert_clone_to_project ();

              /* free the copies created in _do */
              r1_[i].reset ();
              r2_[i].reset ();
            }
        },
        convert_to_variant<LengthableObjectPtrVariant> (own_obj.get ()));
    }

  if (do_it)
    num_split_objs_ = sel_->objects_.size ();
  else
    num_split_objs_ = 0;

  ArrangerSelections * sel = get_actual_arranger_selections ();
  EVENTS_PUSH (EventType::ET_ARRANGER_SELECTIONS_CHANGED, sel);

  first_run_ = false;
}

void
ArrangerSelectionsAction::do_or_undo_merge (bool do_it)
{
  /* if first run, merge */
  if (first_run_)
    {
      sel_after_ =
        clone_unique_with_variant<ArrangerSelectionsVariant> (sel_.get ());
      sel_after_->merge ();
    }

  sel_->sort_by_indices (!do_it);
  sel_after_->sort_by_indices (!do_it);

  auto &before_objs = do_it ? sel_->objects_ : sel_after_->objects_;
  auto &after_objs = do_it ? sel_after_->objects_ : sel_->objects_;

  /* remove the before objects from the project */
  for (auto it = before_objs.rbegin (); it != before_objs.rend (); ++it)
    {
      auto &own_before_obj = *it;
      own_before_obj->flags_ |= ArrangerObject::Flags::NonProject;

      /* find the actual object */
      auto prj_obj = own_before_obj->find_in_project ();
      z_return_if_fail (prj_obj);

      /* remove */
      prj_obj->remove_from_project ();
    }

  /* add the after objects to the project */
  for (auto it = after_objs.rbegin (); it != after_objs.rend (); ++it)
    {
      auto &own_after_obj = *it;
      own_after_obj->flags_ |= ArrangerObject::Flags::NonProject;

      auto prj_obj = own_after_obj->add_clone_to_project (false);

      /* remember positions */
      own_after_obj->copy_identifier (*prj_obj);
    }

  ArrangerSelections * sel = get_actual_arranger_selections ();
  EVENTS_PUSH (EventType::ET_ARRANGER_SELECTIONS_CHANGED, sel);

  first_run_ = false;
}

void
ArrangerSelectionsAction::do_or_undo_resize (bool do_it)
{
  auto &objs_before = sel_->objects_;

  bool sel_after_existed = sel_after_ != nullptr;
  if (!sel_after_)
    {
      /* create the "after" selections here if not already given (e.g., when the
       * objects are already edited) */
      set_after_selections (*sel_);
    }

  auto &objs_after = sel_after_->objects_;

  double ticks = do_it ? ticks_ : -ticks_;

  /* if objects are already edited and this is the first run nothing needs to be
   * done */
  if (!sel_after_existed || !first_run_)
    {
      for (size_t i = 0; i < objs_before.size (); i++)
        {
          auto &own_obj_before = objs_before[i];
          own_obj_before->flags_ |= ArrangerObject::Flags::NonProject;
          auto &own_obj_after = objs_after[i];
          own_obj_after->flags_ |= ArrangerObject::Flags::NonProject;

          /* find the actual object */
          auto obj =
            do_it ? own_obj_before->find_in_project ()
                  : own_obj_after->find_in_project ();
          z_return_if_fail (obj);

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

          /* on first do, resize both the project object and our own "after"
           * object */
          if (do_it && first_run_)
            {
              dynamic_pointer_cast<LengthableObject> (obj)->resize (
                left, type, ticks, false);
              dynamic_cast<LengthableObject *> (own_obj_after.get ())
                ->resize (left, type, ticks, false);
            } /* endif do and first run */

          /* remove the project object and add a clone of our corresponding
           * object */
          obj->remove_from_project ();
          obj = nullptr;
          auto new_obj =
            (do_it ? own_obj_after : own_obj_before)->insert_clone_to_project ();

          /* select it */
          new_obj->select (true, true, false);
        } /* endforeach object */
    }

  update_region_link_groups (do_it ? objs_after : objs_before);

  ArrangerSelections * sel = get_actual_arranger_selections ();
  EVENTS_PUSH (EventType::ET_ARRANGER_SELECTIONS_CHANGED, sel);
  EVENTS_PUSH (EventType::ET_ARRANGER_SELECTIONS_ACTION_FINISHED, sel);

  first_run_ = false;
}

void
ArrangerSelectionsAction::do_or_undo_quantize (bool do_it)
{
  auto &objs = sel_->objects_;
  auto &quantized_objs = sel_after_->objects_;

  for (size_t i = 0; i < objs.size (); i++)
    {
      auto &own_obj = objs[i];
      auto &own_quantized_obj = quantized_objs[i];

      own_obj->flags_ |= ArrangerObject::Flags::NonProject;
      own_quantized_obj->flags_ |= ArrangerObject::Flags::NonProject;

      /* find the actual object */
      auto obj =
        do_it ? own_obj->find_in_project ()
              : own_quantized_obj->find_in_project ();
      z_return_if_fail (obj);

      if (do_it)
        {
          /* quantize it */
          if (opts_->adj_start_)
            {
              double ticks = opts_->quantize_position (&obj->pos_);
              dynamic_pointer_cast<LengthableObject> (obj)->end_pos_.add_ticks (
                ticks);
            }
          if (opts_->adj_end_)
            {
              opts_->quantize_position (
                &dynamic_pointer_cast<LengthableObject> (obj)->end_pos_);
            }
          obj->pos_setter (&obj->pos_);
          dynamic_pointer_cast<LengthableObject> (obj)->end_pos_setter (
            &dynamic_pointer_cast<LengthableObject> (obj)->end_pos_);

          /* remember the quantized position so we
           * can find the object when undoing */
          own_quantized_obj->pos_ = obj->pos_;
          dynamic_cast<LengthableObject &> (*own_quantized_obj).end_pos_ =
            dynamic_pointer_cast<LengthableObject> (obj)->end_pos_;
        }
      else
        {
          /* unquantize it */
          obj->pos_setter (&own_obj->pos_);
          dynamic_pointer_cast<LengthableObject> (obj)->end_pos_setter (
            &dynamic_cast<LengthableObject &> (*own_obj).end_pos_);
        }
    }

  ArrangerSelections * sel = get_actual_arranger_selections ();
  EVENTS_PUSH (EventType::ET_ARRANGER_SELECTIONS_QUANTIZED, sel);

  first_run_ = false;
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
  EVENTS_PUSH (EventType::ET_TRACK_LANES_VISIBILITY_CHANGED, nullptr);
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
  if (sel_ && sel_->contains_clip (clip))
    {
      return true;
    }
  if (sel_after_ && sel_after_->contains_clip (clip))
    {
      return true;
    }

  /* check split regions (if any) */
  for (size_t i = 0; i < num_split_objs_; i++)
    {
      auto r1 = dynamic_cast<AudioRegion *> (r1_[i].get ());
      auto r2 = dynamic_cast<AudioRegion *> (r2_[i].get ());
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

std::string
ArrangerSelectionsAction::to_string () const
{
  switch (type_)
    {
    case Type::Create:
      z_return_val_if_fail (sel_, "");
      switch (sel_->type_)
        {
        case ArrangerSelections::Type::Timeline:
          return _ ("Create timeline selections");
        case ArrangerSelections::Type::Audio:
          return _ ("Create audio selections");
        case ArrangerSelections::Type::Automation:
          return _ ("Create automation selections");
        case ArrangerSelections::Type::Chord:
          return _ ("Create chord selections");
        case ArrangerSelections::Type::Midi:
          return _ ("Create MIDI selections");
        default:
          z_return_val_if_reached ("");
        }
    case Type::Delete:
      return _ ("Delete arranger selections");
    case Type::Duplicate:
      return _ ("Duplicate arranger selections");
    case Type::Move:
      return _ ("Move arranger selections");
    case Type::Link:
      return _ ("Link arranger selections");
    case Type::Record:
      return _ ("Record arranger selections");
    case Type::Edit:
      return _ ("Edit arranger selections");
    case Type::AutomationFill:
      return _ ("Automation fill");
    case Type::Split:
      return _ ("Split arranger selections");
    case Type::Merge:
      return _ ("Merge arranger selections");
    case Type::Resize:
      {
        return format_str (_ ("Resize arranger selections - {}"), resize_type_);
      }
    case Type::Quantize:
      return _ ("Quantize arranger selections");
    default:
      break;
    }

  z_return_val_if_reached ("");
}