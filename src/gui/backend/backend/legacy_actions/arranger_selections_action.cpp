// SPDX-FileCopyrightText: © 2019-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "engine/session/graph_dispatcher.h"
#include "gui/backend/backend/actions/arranger_selections_action.h"
#include "gui/backend/backend/project.h"
#include "gui/backend/backend/settings_manager.h"
#include "gui/backend/backend/zrythm.h"
#include "structure/tracks/track_all.h"
#include "structure/tracks/tracklist.h"
#include "utils/gtest_wrapper.h"
#include "utils/math.h"
#include "utils/views.h"

namespace zrythm::gui::actions
{
using namespace zrythm::structure::arrangement;
using namespace zrythm::structure::tracks;

ArrangerSelectionsAction::ArrangerSelectionsAction ()
    : UndoableAction (UndoableAction::Type::ArrangerSelections)
{
}

CreateOrDeleteArrangerSelectionsAction::CreateOrDeleteArrangerSelectionsAction (
  ArrangerObjectSpan sel_var,
  bool               create)
    : ArrangerSelectionsAction (sel_var, create ? Type::Create : Type::Delete)
{
  if (sel_var.contains_undeletable_object ())
    {
      throw ZrythmException (
        QObject::tr ("Arranger selections contain an undeletable object"));
    }
}

void
init_from (
  ArrangerSelectionsAction       &obj,
  const ArrangerSelectionsAction &other,
  utils::ObjectCloneType          clone_type)
{
  assert (clone_type == utils::ObjectCloneType::Snapshot);
  init_from (
    static_cast<UndoableAction &> (obj),
    static_cast<const UndoableAction &> (other), clone_type);
  obj.type_ = other.type_;

// TODO
#if 0
  auto clone_collection =
    [&] (auto &our_collection, const auto &other_collection) {
      if (other_collection.has_value ())
        {
          our_collection.emplace (std::vector<ArrangerObjectPtrVariant>{});
          for (const auto &obj_var : *other_collection)
            {
              std::visit (
                [&] (auto &&obj) {
                  auto * clone =
                    PROJECT->getArrangerObjectFactory ()
                      ->clone_object_snapshot (*obj, *this);
                  our_collection->push_back (clone);
                },
                obj_var);
            }
        }
    };

  clone_collection (sel_, other.sel_);
  clone_collection (sel_after_, other.sel_after_);
#endif

  obj.edit_type_ = other.edit_type_;
  obj.ticks_ = other.ticks_;
  obj.delta_tracks_ = other.delta_tracks_;
  obj.delta_lanes_ = other.delta_lanes_;
  obj.delta_chords_ = other.delta_chords_;
  obj.delta_pitch_ = other.delta_pitch_;
  obj.delta_vel_ = other.delta_vel_;
  obj.delta_normalized_amount_ = other.delta_normalized_amount_;
  obj.target_port_ = other.target_port_;
  obj.str_ = other.str_;
  obj.pos_ = other.pos_;
  obj.r1_ = other.r1_;
  obj.r2_ = other.r2_;
  obj.first_run_ = other.first_run_;
  if (other.opts_)
    obj.opts_ =
      std::make_unique<zrythm::gui::old_dsp::QuantizeOptions> (*other.opts_);
  // TODO
#if 0
    if (other.region_before_)
    {
      std::visit (
        [&] (auto &&region_before) {
          auto * clone = ArrangerObjectFactory::clone_object_snapshot (
            *region_before, *this, PROJECT->get_arranger_object_registry ());
          region_before_ = clone;
        },
        other.region_before_.value ());
    }
  if (other.region_after_)
    {
      std::visit (
        [&] (auto &&region_after) {
          auto * clone = ArrangerObjectFactory::clone_object_snapshot (
            *region_after, *this, PROJECT->get_arranger_object_registry ());
          region_after_ = clone;
        },
        other.region_after_.value ());
    }
#endif
  obj.resize_type_ = other.resize_type_;
}

auto
ArrangerSelectionsAction::get_arranger_object_registry () const
  -> ArrangerObjectRegistry &
{
  return PROJECT->get_arranger_object_registry ();
}

void
ArrangerSelectionsAction::init_loaded_impl ()
{
// TODO/delete
#if 0
  const auto object_init_loaded_visitor = [&] (auto &&o) {
    using ObjectT = base_type<decltype (o)>;
    if constexpr (std::derived_from<ObjectT, Region>)
      {
        if constexpr (
          std::is_same_v<ObjectT, structure::arrangement::AudioRegion>)
          {
            o->fix_positions (frames_per_tick_);
            o->validate (false, frames_per_tick_);
          }
        o->validate (false, frames_per_tick_);
      }
  };

  if (sel_)
    {
      for (const auto &obj_var : *sel_)
        {
          std::visit (object_init_loaded_visitor, obj_var);
        }
    };
  if (sel_after_)
    {
      for (const auto &obj_var : *sel_after_)
        {
          std::visit (object_init_loaded_visitor, obj_var);
        }
    }

#  if 0
  for (const auto&[r1_var, r2_var] : std::views::zip(r1_, r2_))
    {
      std::visit (
        [&] (auto &&r1, auto &&r2) {
          r1->init_loaded ();
          r2->init_loaded ();
        },
        r1_var, r2_var);
    }
#  endif
#endif
}

void
ArrangerSelectionsAction::set_before_selections (ArrangerObjectSpan src_var)
{
#if 0
  sel_ = src_var.create_snapshots (
    *PROJECT->getArrangerObjectFactory (), *this);
#endif
}

void
ArrangerSelectionsAction::set_after_selections (ArrangerObjectSpan src_var)
{
#if 0
  sel_after_ = src_var.create_snapshots (
    *PROJECT->getArrangerObjectFactory (), *this);
#endif
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

auto
ArrangerSelectionsAction::get_project_arranger_objects () const
  -> std::vector<ArrangerObjectPtrVariant>
{
  return *sel_ | std::views::transform ([] (auto &&obj_var) {
    return std::visit (
      [&] (auto &&obj) {
        return PROJECT->get_arranger_object_registry ().find_by_id_or_throw (
          obj->get_uuid ());
      },
      obj_var);
  }) | std::ranges::to<std::vector> ();
}

ArrangerSelectionsAction::MoveOrDuplicateAction::MoveOrDuplicateAction (
  ArrangerObjectSpan           sel_var,
  bool                         move,
  double                       ticks,
  int                          delta_chords,
  int                          delta_pitch,
  int                          delta_tracks,
  int                          delta_lanes,
  double                       delta_normalized_amount,
  std::optional<dsp::PortUuid> tgt_port_id,
  bool                         already_moved)
    : ArrangerSelectionsAction (sel_var, move ? Type::Move : Type::Duplicate)
{
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
      if (sel_var.contains_unclonable_object ())
        {
          throw ZrythmException (
            QObject::tr (
              "Arranger selections contain an object that cannot be duplicated"));
        }
    }

  if (!move)
    {
      set_after_selections (sel_var);
    }

  target_port_ = tgt_port_id;
}

ArrangerSelectionsAction::LinkAction::LinkAction (
  ArrangerObjectSpan sel_before,
  ArrangerObjectSpan sel_after,
  double             ticks,
  int                delta_tracks,
  int                delta_lanes,
  bool               already_moved)
    : ArrangerSelectionsAction (sel_before, Type::Link)
{
  first_run_ = already_moved;
  ticks_ = ticks;
  delta_tracks_ = delta_tracks;
  delta_lanes_ = delta_lanes;

  set_after_selections (sel_after);
}

ArrangerSelectionsAction::RecordAction::RecordAction (
  ArrangerObjectSpan sel_before,
  ArrangerObjectSpan sel_after,
  bool               already_recorded)
    : ArrangerSelectionsAction (sel_before, Type::Record)
{
  first_run_ = !already_recorded;
  set_after_selections (sel_after);
}

EditArrangerSelectionsAction::EditArrangerSelectionsAction (
  ArrangerObjectSpan                sel_before_var,
  std::optional<ArrangerObjectSpan> sel_after,
  EditType                          type,
  bool                              already_edited)
    : ArrangerSelectionsAction (sel_before_var, Type::Edit)
{
  first_run_ = already_edited;
  edit_type_ = type;
  if (type == EditType::Name && sel_before_var.contains_unrenamable_object ())
    {
      throw ZrythmException (QObject::tr ("Cannot rename selected object(s)"));
    }

  set_before_selections (sel_before_var);

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

      set_after_selections (sel_before_var);
    }
}

std::unique_ptr<EditArrangerSelectionsAction>
EditArrangerSelectionsAction::create (
  ArrangerObjectSpan sel_before_var,
  ArrangerObjectSpan sel_after_var,
  EditType           type,
  bool               already_edited)
{
  return std::make_unique<EditArrangerSelectionsAction> (
    sel_before_var, sel_after_var, type, already_edited);
}

EditArrangerSelectionsAction::EditArrangerSelectionsAction (
  ArrangerObjectSpan    sel_before_var,
  MidiFunction::Type    midi_func_type,
  MidiFunction::Options opts)
    : EditArrangerSelectionsAction (
        sel_before_var,
        std::nullopt,
        EditType::EditorFunction,
        false)
{
  set_after_selections (sel_before_var);
  MidiFunction::apply (ArrangerObjectSpan{ *sel_after_ }, midi_func_type, opts);
}

EditArrangerSelectionsAction::EditArrangerSelectionsAction (
  ArrangerObjectSpan       sel_before_var,
  AutomationFunction::Type automation_func_type)
    : EditArrangerSelectionsAction (
        sel_before_var,
        std::nullopt,
        EditType::EditorFunction,
        false)
{
  set_after_selections (sel_before_var);
  AutomationFunction::apply (
    ArrangerObjectSpan{ *sel_after_ }, automation_func_type);
}

EditArrangerSelectionsAction::EditArrangerSelectionsAction (
  ArrangerObject::Uuid                      region_id,
  const dsp::Position                      &sel_start,
  const dsp::Position                      &sel_end,
  structure::arrangement::AudioFunctionType audio_func_type,
  structure::arrangement::AudioFunctionOpts opts,
  std::optional<utils::Utf8String>          uri)
    : EditArrangerSelectionsAction (
        ArrangerObjectSpan{
          PROJECT->get_arranger_object_registry ().find_by_id_or_throw (
            region_id) },
        std::nullopt,
        EditType::EditorFunction,
        false)
{
  clip_editor_region_id_ = region_id;
  selected_positions_in_audio_editor_ = std::make_pair (sel_start, sel_end);

  z_debug ("saving file before applying audio func...");
  audio_function_apply (
    region_id, sel_start, sel_end, AudioFunctionType::Invalid, opts, uri);

  z_debug ("applying actual audio func...");
  audio_function_apply (
    region_id, sel_start, sel_end, audio_func_type, opts, uri);

  set_after_selections (
    ArrangerObjectSpan{
      PROJECT->get_arranger_object_registry ().find_by_id_or_throw (region_id) });
}

ArrangerSelectionsAction::SplitAction::SplitAction (
  ArrangerObjectSpan sel,
  Position           pos)
    : ArrangerSelectionsAction (sel, Type::Split)
{
  pos_ = pos;
  pos_.update_frames_from_ticks (frames_per_tick_);
}

ArrangerSelectionsAction::ResizeAction::ResizeAction (
  ArrangerObjectSpan                sel_before,
  std::optional<ArrangerObjectSpan> sel_after,
  ResizeType                        type,
  double                            ticks)
    : ArrangerSelectionsAction (sel_before, Type::Resize)
{
  resize_type_ = type;
  ticks_ = ticks;
  /* validate */
  auto sel_before_span = ArrangerObjectSpan{ *sel_ };
  bool have_unresizable = !std::ranges::all_of (
    sel_before_span, ArrangerObjectSpan::bounded_projection);
  if (have_unresizable)
    {
      throw ZrythmException (
        QObject::tr ("Attempted to resize unresizable objects"));
    }

  bool have_looped = std::ranges::any_of (
    sel_before_span, ArrangerObjectSpan::looped_projection);
  if (
    have_looped
    && (type == ResizeType::L || type == ResizeType::R || type == ResizeType::LStretch || type == ResizeType::RStretch))
    {
      throw ZrythmException (format_qstr (
        QObject::tr (
          "Cannot perform {} resize - selections contain looped objects"),
        type));
    }

  bool have_unloopable = !std::ranges::all_of (
    sel_before_span, ArrangerObjectSpan::is_region_projection);
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

template <RegionObject RegionT>
ArrangerSelectionsAction::AutomationFillAction::AutomationFillAction (
  const RegionT &region_before,
  const RegionT &region_after,
  bool           already_changed)
    : ArrangerSelectionsAction ()
{
  type_ = Type::AutomationFill;

// TODO
#if 0
  auto * clone = region_before.clone_raw_ptr ();
  clone->setParent (this);
  region_before_ = clone;

  clone = region_after.clone_raw_ptr ();
  clone->setParent (this);
  region_after_ = clone;
  first_run_ = already_changed;
#endif
}

void
ArrangerSelectionsAction::update_region_link_groups (const auto &objects)
{
// TODO (also maybe not needed depending on how linking is done - check
// ArrangerObjectOwner for ideas - like sharing an ArrangerObjectOwner between
// many regions)
#if 0
  /* handle children of linked regions */
  for (auto &_obj_var : objects)
    {
      std::visit (
        [&] (auto &&_obj) {
          using ObjT = base_type<decltype (_obj)>;
          /* get the actual object from the project */
          auto obj_var = PROJECT->find_arranger_object_by_id (_obj->get_uuid ());
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
#endif
}

void
ArrangerSelectionsAction::do_or_undo_move (bool do_it)
{
  // sel->sort_by_indices (!do_it);

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
      for (auto &own_obj_var : *sel_)
        {
          std::visit (
            [&] (auto &&own_obj_ptr) {
              using ObjT = base_type<decltype (own_obj_ptr)>;

              /* get the actual object from the project */
              auto prj_obj = std::get<ObjT *> (
                *PROJECT->find_arranger_object_by_id (own_obj_ptr->get_uuid ()));

              /* remember if automation point */
              if constexpr (std::is_same_v<ObjT, AutomationPoint>)
                {
                  obj_map[prj_obj] = own_obj_ptr;
                }

              if (!utils::math::floats_equal (ticks, 0.0))
                {
                  /* shift the actual object */
                  prj_obj->position ()->addTicks (ticks);

                  /* also shift the copy */
                  own_obj_ptr->position ()->addTicks (ticks);
                }

              if (delta_tracks != 0 || delta_lanes != 0)
                {
                  /* shift the actual object */
                  move_obj_by_tracks_and_lanes (
                    prj_obj, delta_tracks, delta_lanes, false, -1);

                  /* remember new info in own copy */
                  // ArrangerObjectSpan::copy_arranger_object_identifier (
                  // own_obj_ptr, prj_obj);
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
                      prj_obj->setChordDescriptorIndex (
                        prj_obj->chordDescriptorIndex () + delta_chords);

                      /* also shift the copy so they can match */
                      own_obj_ptr->setChordDescriptorIndex (
                        own_obj_ptr->chordDescriptorIndex () + delta_chords);
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
// TODO
#if 0
                      auto cur_at = prj_obj->get_automation_track ();
                      z_return_if_fail (cur_at);
                      auto port_var = PROJECT->find_port_by_id (*target_port_);
                      z_return_if_fail (
                        port_var
                        && std::holds_alternative<ControlPort *> (*port_var));
                      auto port = std::get<ControlPort *> (*port_var);
                      auto track_var = PROJECT->find_track_by_id (
                        port->id_->get_track_id ().value ());
                      z_return_if_fail (track_var.has_value ());
                      std::visit (
                        [&] (auto &&track) {
                          using TrackT = base_type<decltype (track)>;
                          if constexpr (
                            AutomatableTrack<TrackT>)
                            {
                              AutomationTrack * at = AutomationTrack::
                                find_from_port (*port, track, true);
                              z_return_if_fail (at);

                              /* move the actual object */
                              TRACKLIST->move_region_to_track (
                                prj_obj, track->get_uuid (), at->index_, -1);

                              target_port_ = cur_at->port_id_;
                            }
                          else
                            {
                              z_return_if_reached ();
                            }
                        },
                        track_var.value ());
#endif
                    }
                  else
                    {
                      z_return_if_reached ();
                    }
                }

              if (!utils::math::floats_equal (delta_normalized_amt, 0.0))
                {
                  if constexpr (std::is_same_v<ObjT, AutomationPoint>)
                    {
                      /* shift the actual object */
                      prj_obj->setValue (
                        prj_obj->value ()
                        + static_cast<float> (delta_normalized_amt));

                      /* also shift the copy so they can match */
                      own_obj_ptr->setValue (
                        own_obj_ptr->value ()
                        + static_cast<float> (delta_normalized_amt));
                    }
                  else
                    {
                      z_return_if_reached ();
                    }
                }
            },
            own_obj_var);
        }

#if 0
      /* if moving automation points, re-sort the region and remember the
       * new indices */
      auto first_own_obj_var = sel_->front ();
      std::visit (
        [&] (auto &&first_own_obj) {
          using ObjT = base_type<decltype (first_own_obj)>;
          if constexpr (std::is_same_v<ObjT, AutomationPoint>)
            {
              auto obj = std::get<AutomationPoint *> (
                *PROJECT->find_arranger_object_by_id (
                  first_own_obj->get_uuid ()));
              z_return_if_fail (obj);

              auto region =
                dynamic_cast<AutomationRegion *> (obj->get_region ());
              z_return_if_fail (region);
              region->force_sort ();

              for (auto &[prj_ap, cached_ap] : obj_map)
                {
                  // auto * prj_ap_cast =
                  // dynamic_cast<AutomationPoint *> (prj_ap);
                  auto * cached_ap_cast =
                    dynamic_cast<AutomationPoint *> (cached_ap);
                  cached_ap_cast->set_region_and_index (*region);
                }
            }
        },
        first_own_obj_var);
#endif
    }

  update_region_link_groups (*sel_);

  /* validate */
  CLIP_EDITOR->get_region_and_track ();

  // ArrangerSelections * sel = get_actual_arranger_selections ();
  /* EVENTS_PUSH (EventType::ET_ARRANGER_SELECTIONS_CHANGED, sel); */
  /* EVENTS_PUSH (EventType::ET_ARRANGER_SELECTIONS_MOVED, sel); */

  first_run_ = false;
}

void
ArrangerSelectionsAction::move_obj_by_tracks_and_lanes (
  ArrangerObjectPtrVariant obj_var,
  const int                tracks_diff,
  const int                lanes_diff,
  bool                     use_index_in_prev_lane,
  int                      index_in_prev_lane)
{
#if 0
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
              const auto track_before_var =
                *PROJECT->find_track_by_id (*obj->get_track_id ());
              std::visit (
                [&] (auto &&track_before) {
                  auto _track_to_move_to =
                    TRACKLIST->get_visible_track_after_delta (
                      track_before->get_uuid (), tracks_diff);
                  z_return_if_fail (_track_to_move_to);
                  auto * track_to_move_to =
                    Track::from_variant (_track_to_move_to.value ());
                  z_trace (
                    "Moving from track {} ({}) to track: {} ({})",
                    track_before->get_name (), track_before->get_uuid (),
                    track_to_move_to->get_name (),
                    track_to_move_to->get_uuid ());

                  /* shift the actual object by tracks */
#  if 0
                  if (ENUM_BITSET_TEST (
                        obj->flags_, ArrangerObject::Flags::NonProject))
                    {
                      obj->set_track_id (track_to_move_to->get_uuid ());
                      z_trace ("Updated track name hash for non-project object");
                    }
                  else
                    {
#  endif
                  TRACKLIST->move_region_to_track (
                    obj, track_to_move_to->get_uuid (), -1,
                    use_index_in_prev_lane ? index_in_prev_lane : -1);
                  z_trace ("Moved project object to track");
#  if 0
                    }
#  endif
                },
                track_before_var);
            }
        }
      if (lanes_diff)
        {
// TODO
#  if 0
          if constexpr (std::derived_from<ObjT, LaneOwnedObject>)
            {
              const auto obj_lane_pos = obj->get_lane ().get_index_in_track ();
              const auto new_lane_pos = obj_lane_pos + lanes_diff;
              z_return_if_fail (new_lane_pos >= 0);
              z_trace ("New lane position: {}", new_lane_pos);

              /* shift the actual object by lanes */
              if (
                ENUM_BITSET_TEST (obj->flags_, ArrangerObject::Flags::NonProject))
                {
                  // TODO:
                  // obj->id_.lane_pos_ = new_lane_pos;
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
                              TRACKLIST->move_region_to_track (
                                obj, r_track->get_uuid (), new_lane_pos,
                                use_index_in_prev_lane ? index_in_prev_lane : -1);
                              z_trace ("Moved project object to new lane");
                            }
                        },
                        *PROJECT->find_track_by_id (*obj->get_track_id ()));
                    }
                }
            }
#  endif
        }
      z_debug ("Object move completed");
    },
    obj_var);
#endif
}

void
ArrangerSelectionsAction::do_or_undo_duplicate_or_link (bool link, bool do_it)
{
#if 0
  // TODO:
  // sel->sort_by_indices (!do_it);
  // sel_after->sort_by_indices (!do_it);

  const double ticks = do_it ? ticks_ : -ticks_;
  const int    delta_tracks = do_it ? delta_tracks_ : -delta_tracks_;
  const int    delta_lanes = do_it ? delta_lanes_ : -delta_lanes_;
  const int    delta_chords = do_it ? delta_chords_ : -delta_chords_;
  const int    delta_pitch = do_it ? delta_pitch_ : -delta_pitch_;
  const double delta_normalized_amount =
    do_it ? delta_normalized_amount_ : -delta_normalized_amount_;

  /* clear current selections in the project */
  auto sel_after_span = ArrangerObjectSpan{ *sel_after_ };

  TRACKLIST->clear_selections_for_object_siblings (
    ArrangerObjectSpan::uuid_projection (sel_after_span[0]));

  /* this is used for automation points to keep track of which automation
   * point in the project matches which automation point in the cached
   * selections key: project automation point, value: cached automation
   * point */
  std::map<AutomationPoint *, AutomationPoint *> ap_map;

// TODO
#  if 0
  // this essentially sets the NonProject flag if not performing on first run
  for (
    const auto &[index, own_obj_var] :
    std::views::reverse (utils::views::enumerate (sel_after_span)))
    {
      std::visit (
        [&] (auto &&own_obj) {
          using ObjT = base_type<decltype (own_obj)>;
          auto own_orig_obj = std::get<ObjT *> (sel_->at (index));
          own_obj->flags_ |= ArrangerObject::Flags::NonProject;
          own_orig_obj->flags_ |= ArrangerObject::Flags::NonProject;

          /* on first run, we need to first move the original object
           * backwards (the project object too) */
          if (do_it && first_run_)
            {
              auto obj = std::get<ObjT *> (
                *PROJECT->find_arranger_object_by_id (own_obj->get_uuid ()));

              z_debug ("{} moving original object backwards", index);

              /* ticks */
              if (!utils::math::floats_equal (ticks_, 0.0))
                {
                  obj->move (-ticks_, AUDIO_ENGINE->frames_per_tick_);
                  own_obj->move (-ticks_, AUDIO_ENGINE->frames_per_tick_);
                }

              /* tracks & lanes */
              if (delta_tracks_ || delta_lanes_)
                {
                  if constexpr (std::derived_from<ObjT, LaneOwnedObject>)
                    {
                      z_trace ("moving prj obj");
                      move_obj_by_tracks_and_lanes (
                        obj, -delta_tracks_, -delta_lanes_, true,
                        *own_obj->index_in_prev_lane_);

                      z_trace ("moving own obj");
                      // const auto track_id_before_move = own_obj->track_id_;
                      // const auto lane_idx_before_move =
                      // own_obj->get_lane_index();
                      move_obj_by_tracks_and_lanes (
                        own_obj, -delta_tracks_, -delta_lanes_, true,
                        *own_obj->index_in_prev_lane_);

// TODO
#    if 0
                      /* since the object moved outside of its lane,
                       * decrement
                       * the index inside the lane for all of our cached objects
                       * in the same lane */
                      for (
                        const auto _ :
                        std::views::iota (index + 1_zu, sel_after_span.size ()))
                        {
                          if (
                            track_id_before_move
                              == own_obj->track_id_
                            && lane_idx_before_move == own_obj->get_lane_index())
                            {
                              own_obj->idx_--;
                            }
                        }
#    endif
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
              if (!utils::math::floats_equal (delta_normalized_amount_, 0.0))
                {
                  if constexpr (std::is_same_v<ObjT, AutomationPoint>)
                    {
                      obj->set_fvalue (
                        obj->normalized_val_ - (float) delta_normalized_amount_,
                        true);
                      own_obj->set_fvalue (
                        own_obj->normalized_val_
                          - (float) delta_normalized_amount_,
                        true);
                    }
                }
            } /* if do and first run */
        },
        own_obj_var);
    }
#  endif

  for (
    const auto &[own_obj_var, own_orig_obj_var] :
    std::views::zip (sel_after_span, *sel_))
    {
      std::visit (
        [&] (auto &&own_obj) {
          using ObjT = base_type<decltype (own_obj)>;
          auto own_orig_obj = std::get<ObjT *> (own_orig_obj_var);

          if (do_it)
            {

// TODO
#  if 0
              auto add_adjusted_clone_to_project = [] (const auto &obj_to_clone) {
                /* create a temporary clone */
                auto obj =
                  PROJECT->getArrangerObjectFactory ()
                    ->clone_new_object_identity (*obj_to_clone);

                /* if region, clear the remembered index so that the
                 * region gets appended instead of inserted */
                if constexpr (RegionObject<ObjT>)
                  {
                    obj->id_.idx_ = -1;
                  }


                /* add to track. */
                return std::get<ObjT *> (obj->add_clone_to_project (false));
              };

              auto added_obj_ref = add_adjusted_clone_to_project (own_obj);

              /* edit both project object and the copy */
              if (!utils::math::floats_equal (ticks, 0.0))
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
                      const auto port_var =
                        PROJECT->find_port_by_id (*target_port_);
                      z_return_if_fail (
                        port_var
                        && std::holds_alternative<ControlPort *> (*port_var));
                      const auto * port = std::get<ControlPort *> (*port_var);
                      auto         track_var = PROJECT->find_track_by_id (
                        port->id_->get_track_id ().value ());
                      z_return_if_fail (track_var.has_value ());
                      std::visit (
                        [&] (auto &&track) {
                          using TrackT = base_type<decltype (track)>;
                          if constexpr (
                            AutomatableTrack<TrackT>)
                            {
                              const auto * at = AutomationTrack::find_from_port (
                                *port, track, true);
                              z_return_if_fail (at);

                              /* move the actual object */
                              added_obj_ref->move_to_track (
                                track, at->index_, -1);
                            }
                          else
                            {
                              z_return_if_reached ();
                            }
                        },
                        track_var.value ());
                    }
                }
              if (!utils::math::floats_equal (delta_normalized_amount, 0.0))
                {
                  if constexpr (std::is_same_v<ObjT, AutomationPoint>)
                    {
                      added_obj_ref->set_fvalue (
                        added_obj_ref->normalized_val_
                          + static_cast<float> (delta_normalized_amount),
                        true);
                      own_obj->set_fvalue (
                        own_obj->normalized_val_
                          + static_cast<float> (delta_normalized_amount),
                        true);
                    }
                }

              if constexpr (RegionObject<ObjT>)
                {
                  /* if we are linking, create the necessary links */
                  if (link)
                    {
                      /* add link group to original object if necessary */
                      auto orig_r =
                        std::get<ObjT *> (*own_orig_obj->find_in_project ());
                      // z_return_if_fail (orig_r->id_.idx_ >= 0);

                      orig_r->create_link_group_if_none ();
                      const auto link_group = *orig_r->link_group_;

                      /* add link group to clone */
                      // z_return_if_fail (added_obj_ref->id_.idx_ >= 0);
                      added_obj_ref->set_link_group (link_group, true);

                      /* remember link groups */
                      own_orig_obj->set_link_group (link_group, true);
                      own_obj->set_link_group (link_group, true);
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
                          const auto * clip = added_obj_ref->get_clip ();
                          const auto   id = AUDIO_POOL->duplicate_clip (
                            clip->get_uuid (), true);
                          added_obj_ref->set_clip_id (id);
                        }
                    }
                } /* endif region */

              /* add the mapping to the hashtable */
              if constexpr (std::is_same_v<ObjT, AutomationPoint>)
                {
                  ap_map[added_obj_ref] = own_obj;
                }

              /* select it */
              added_obj_ref->setSelected (true);

              /* remember the identifier */
              ArrangerObjectSpan::copy_arranger_object_identifier (
                own_obj, added_obj_ref);
#  endif
            } /* endif do */
          else /* if undo */
            {
              /* find the actual object */
              // auto obj = std::get<ObjT *> (
              //   *PROJECT->find_arranger_object_by_id (own_obj->get_uuid ()));

              /* if the object was created with linking, delete the links */
              if constexpr (RegionObject<ObjT>)
                {
                  if (link)
                    {
// TODO?
#  if 0
                      /* remove link from created object (this will also
                       * automatically remove the link from the parent region
                       * if it is the only region in the link group) */
                      z_return_if_fail (obj->has_link_group ());
                      obj->unlink ();

                      /* unlink remembered link groups */
                      own_orig_obj->unlink ();
                      own_obj->unlink ();
#  endif
                    }
                }

              /* remove it */
              // TODO
              // obj->remove_from_project (true);

              /* set the copies back to original state */
              if (!utils::math::floats_equal (ticks, 0.0))
                {
                  own_obj->position ()->addTicks (ticks);
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
                      own_obj->setChordDescriptorIndex (
                        own_obj->chordDescriptorIndex () + delta_chords);
                    }
                }
              if (target_port_)
                {
                  /* nothing needed */
                }
              if (!utils::math::floats_equal (delta_normalized_amount, 0.0))
                {
                  if constexpr (std::is_same_v<ObjT, AutomationPoint>)
                    {
                      own_obj->setValue (
                        own_obj->value ()
                        + static_cast<float> (delta_normalized_amount));
                    }
                }

            } /* endif undo */
        },
        own_obj_var);
    }

#  if 0
  /* if copy-moving automation points, re-sort the region and remember the
   * new indices */
  if (std::holds_alternative<AutomationPoint *> (sel_after_->front ()))
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
              // const auto * prj_ap = pair.first;
              auto * cached_ap = pair.second;
              cached_ap->set_region_and_index (*region);
            }
        }
    }
#  endif

  if (do_it)
    {
      TRANSPORT->recalculate_total_bars (ArrangerObjectSpan{ *sel_after_ });

      /* EVENTS_PUSH (EventType::ET_ARRANGER_SELECTIONS_CREATED, sel); */
    }
  else
    {
      /* EVENTS_PUSH (EventType::ET_ARRANGER_SELECTIONS_REMOVED, sel); */
    }

  first_run_ = false;
#endif
}

void
ArrangerSelectionsAction::do_or_undo_create_or_delete (bool do_it, bool create)
{
  // sel->sort_by_indices (create ? !do_it : do_it);
  // auto * actual_sel = std::get<SelT *> (get_actual_arranger_selections ());
  // z_return_if_fail (actual_sel);

  if (!first_run_ || !create)
    {
      /* clear current selections in the project */
      TRACKLIST->clear_selections_for_object_siblings (
        ArrangerObjectSpan::uuid_projection (sel_->front ()));

// TODO
#if 0
      for (auto &own_obj_var : *sel_)
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
                      prj_obj =
                        std::get<ObjT *> (own_obj->insert_clone_to_project ());
                    }

                  /* select it */
                  PROJECT->getArrangerObjectFactory ()
                    ->get_selection_manager_for_object (*prj_obj)
                    .append_to_selection (prj_obj->get_uuid ());

                  /* remember new info */
                  ArrangerObjectSpan::copy_arranger_object_identifier (
                    own_obj, prj_obj);
                }

              /* if removing */
              else
                {
                  /* get the actual object from the project */
                  auto obj_opt =
                    PROJECT->find_arranger_object_by_id (own_obj->get_uuid ());
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
#endif
    }

  /* if first time creating the object, save the length for use by SnapGrid */
  if (ZRYTHM_HAVE_UI && first_run_ && create && do_it && sel_->size () == 1)
    {
      auto obj_in_project = get_project_arranger_objects ().front ();
      std::visit (
        [&] (auto &&obj) {
          using ObjT = base_type<decltype (obj)>;
          if constexpr (BoundedObject<ObjT>)
            {
              double ticks =
                ArrangerObjectSpan::bounds_projection (obj_in_project)
                  ->length ()
                  ->ticks ();
              if constexpr (TimelineObject<ObjT>)
                {
                  gui::SettingsManager::get_instance ()
                    ->set_timelineLastCreatedObjectLengthInTicks (ticks);
                }
              else
                {
                  gui::SettingsManager::get_instance ()
                    ->set_editorLastCreatedObjectLengthInTicks (ticks);
                }
            }
        },
        obj_in_project);
    }

  /* if creating */
  if ((do_it && create) || (!do_it && !create))
    {
      update_region_link_groups (*sel_);

      TRANSPORT->recalculate_total_bars (ArrangerObjectSpan{ *sel_ });

      /* EVENTS_PUSH (EventType::ET_ARRANGER_SELECTIONS_CREATED, sel); */
    }
  /* if deleting */
  else
    {
      /* EVENTS_PUSH (EventType::ET_ARRANGER_SELECTIONS_REMOVED, sel); */
    }

  first_run_ = false;

  if (std::ranges::any_of (
        *sel_, ArrangerObjectSpan::type_projection<AutomationRegion>))
    {
      ROUTER->recalc_graph (false);
    }
}

void
ArrangerSelectionsAction::do_or_undo_record (bool do_it)
{

  // sel->sort_by_indices (!do_it);
  // sel_after->sort_by_indices (!do_it);
  // auto * actual_sel = std::get<SelT *> (get_actual_arranger_selections ());
  // z_return_if_fail (actual_sel);

  if (!first_run_)
    {
      /* clear current selections in the project */
      TRACKLIST->clear_selections_for_object_siblings (
        ArrangerObjectSpan::uuid_projection (sel_->front ()));

      /* if do/redoing */
      if (do_it)
        {
          // TODO
#if 0
          /* create the newly recorded objects */
          for (auto &own_after_obj_var : *sel_after_)
            {
              std::visit (
                [&] (auto &&own_after_obj) {
                  using ObjT = base_type<decltype (own_after_obj)>;
                  // own_after_obj->flags_ |= ArrangerObject::Flags::NonProject;

                  // TODO: add the corresponding object to the project
                  auto prj_obj = std::get<ObjT *> (
                    get_arranger_object_registry ().find_by_id_or_throw (
                      own_after_obj->get_uuid ()));


                  /* select it */
                  PROJECT->arrangerObjectCreator ()
                    ->get_selection_manager_for_object (*prj_obj)
                    .append_to_selection (prj_obj->get_uuid ());


                  /* remember new info */
                  // own_after_obj->copy_identifier (*prj_obj);
                },
                own_after_obj_var);
            }
#endif

          /* delete the previous objects */
// TODO
#if 0
          for (auto &own_before_obj_var : *sel_)
            {
              std::visit (
                [&] (auto &&own_before_obj) {
                  using ObjT = base_type<decltype (own_before_obj)>;
                  own_before_obj->flags_ |= ArrangerObject::Flags::NonProject;

                  /* get the actual object from the project */
                  auto obj =
                    std::get<ObjT *> (*PROJECT->find_arranger_object_by_id (
                      own_before_obj->get_uuid ()));

                  /* remove it */
                  obj->remove_from_project (true);
                },
                own_before_obj_var);
              }
#endif
        }

      /* if undoing */
      else
        {
          /* delete the newly recorded objects */
          for (auto &own_after_obj_var : *sel_after_)
            {
              std::visit (
                [&] (auto &&own_after_obj) {
                  using ObjT = base_type<decltype (own_after_obj)>;
                  // own_after_obj->flags_ |= ArrangerObject::Flags::NonProject;

                  /* get the actual object from the project */
                  auto obj_opt = PROJECT->find_arranger_object_by_id (
                    own_after_obj->get_uuid ());
                  z_return_if_fail (obj_opt);
                  [[maybe_unused]] auto * obj = std::get<ObjT *> (*obj_opt);

                  /* remove it */
                  // TODO
                  // obj->remove_from_project (true);
                },
                own_after_obj_var);
            }

// TODO
#if 0
          /* add the objects before the recording */
          for (auto &own_before_obj_var : *sel_)
            {
              std::visit (
                [&] (auto &&own_before_obj) {
                  using ObjT = base_type<decltype (own_before_obj)>;
                  own_before_obj->flags_ |= ArrangerObject::Flags::NonProject;

                  /* add a clone to the project */
                  auto prj_obj = std::get<ObjT *> (
                    own_before_obj->add_clone_to_project (false));

                  /* select it */
                  PROJECT->getArrangerObjectFactory ()
                    ->get_selection_manager_for_object (*prj_obj)
                    .append_to_selection (prj_obj->get_uuid ());

                  /* remember new info */
                  ArrangerObjectSpan::copy_arranger_object_identifier (
                    own_before_obj, prj_obj);
                },
                own_before_obj_var);
              }
#endif
        }
    }

  /* EVENTS_PUSH (EventType::ET_ARRANGER_SELECTIONS_CREATED, sel); */
  /* EVENTS_PUSH (EventType::ET_ARRANGER_SELECTIONS_REMOVED, sel); */

  first_run_ = false;

  if (std::ranges::any_of (
        *sel_, ArrangerObjectSpan::type_projection<AutomationRegion>))
    {
      ROUTER->recalc_graph (false);
    }
}

void
ArrangerSelectionsAction::do_or_undo_edit (bool do_it)
{
// TODO
#if 0
  const auto &src_sel = do_it ? sel_ : sel_after_;
  const auto &dest_sel = do_it ? sel_after_ : sel_;

  if (!first_run_)
    {
      if (selected_positions_in_audio_editor_.has_value ())
        {
          z_return_if_fail (edit_type_ == EditType::EditorFunction);
          auto src_audio_sel = dest_sel;
          auto r = std::get<AudioRegion *> (
            get_arranger_object_registry ().find_by_id_or_throw (
              *clip_editor_region_id_));
          auto src_clip = AUDIO_POOL->get_clip (r->get_clip_id ());
          z_return_if_fail (src_clip);

          /* adjust the positions */
          auto [start, end] = *selected_positions_in_audio_editor_;
          start.add_frames (-r->get_position ().frames_, get_ticks_per_frame ());
          end.add_frames (-r->get_position ().frames_, get_ticks_per_frame ());
          auto num_frames =
            static_cast<unsigned_frame_t> (end.frames_ - start.frames_);
          z_return_if_fail (
            num_frames == (unsigned_frame_t) src_clip->get_num_frames ());

          auto src_clip_path = AUDIO_POOL->get_clip_path (*src_clip, false);
          z_debug (
            "replacing audio region {} frames with {} frames", r->get_name (),
            src_clip_path);

          /* replace the frames in the region */
          utils::audio::AudioBuffer buf{
            src_clip->get_num_channels (), static_cast<int> (num_frames)
          };
          for (int i = 0; i < buf.getNumChannels (); ++i)
            {
              buf.copyFrom (
                i, 0, src_clip->get_samples (), i, start.frames_, num_frames);
            }
          r->replace_frames (buf, start.frames_);
        }
      else /* not audio function */
        {
          for (
            const auto &[own_src_obj_var, own_dest_obj_var] :
            std::views::zip (*src_sel, *dest_sel))
            {
              std::visit (
                [&] (auto &&own_src_obj, auto &&own_dest_obj) {
                  using ObjT = base_type<decltype (own_src_obj)>;
                  if constexpr (
                    std::is_same_v<base_type<decltype (own_dest_obj)>, ObjT>)
                    {
                      own_src_obj->flags_ |= ArrangerObject::Flags::NonProject;
                      own_dest_obj->flags_ |= ArrangerObject::Flags::NonProject;

                      /* find the actual object */
                      auto obj =
                        std::get<ObjT *> (*PROJECT->find_arranger_object_by_id (
                          own_src_obj->get_uuid ()));

                      /* change the parameter */
                      switch (edit_type_)
                        {
                        case EditType::Name:
                          {
                            if constexpr (std::derived_from<ObjT, NamedObject>)
                              {
                                obj->set_name (own_dest_obj->get_name ());
                              }
                            else
                              {
                                throw ZrythmException ("Not a nameable object");
                              }
                          }
                          break;
                        case EditType::Position:
                          obj->set_position_unvalidated (
                            own_dest_obj->get_position ());
                          if constexpr (std::derived_from<ObjT, BoundedObject>)
                            {
                              obj->end_pos_->set_to_position (
                                *own_dest_obj->end_pos_);
                            }
                          if constexpr (std::derived_from<ObjT, LoopableObject>)
                            {
                              obj->loop_start_pos_ =
                                own_dest_obj->loop_start_pos_;
                              obj->loop_end_pos_ = own_dest_obj->loop_end_pos_;
                              obj->clip_start_pos_ =
                                own_dest_obj->clip_start_pos_;
                            }
                          break;
                        case EditType::Fades:
                          if constexpr (std::derived_from<ObjT, FadeableObject>)
                            {
                              obj->fade_in_pos_ = own_dest_obj->fade_in_pos_;
                              obj->fade_out_pos_ = own_dest_obj->fade_out_pos_;
                              obj->fade_in_opts_ = own_dest_obj->fade_in_opts_;
                              obj->fade_out_opts_ = own_dest_obj->fade_out_opts_;
                            }
                          break;
                        case EditType::Primitive:
                          if constexpr (std::derived_from<ObjT, MuteableObject>)
                            {
                              obj->muted_ = own_dest_obj->muted_;
                            }
                          if constexpr (std::derived_from<ObjT, ColoredObject>)
                            {
                              obj->color_ = own_dest_obj->color_;
                              obj->use_color_ = own_dest_obj->use_color_;
                            }
                          if constexpr (std::is_same_v<ObjT, AudioRegion>)
                            {
                              obj->musical_mode_ = own_dest_obj->musical_mode_;
                              obj->gain_ = own_dest_obj->gain_;
                            }
                          if constexpr (std::is_same_v<ObjT, MidiNote>)
                            {
                              obj->pitch_ = own_dest_obj->pitch_;
                              obj->vel_->set_val (own_dest_obj->vel_->vel_);
                            }
                          if constexpr (std::is_same_v<ObjT, AutomationPoint>)
                            {
                              obj->curve_opts_ = own_dest_obj->curve_opts_;
                              obj->fvalue_ = own_dest_obj->fvalue_;
                              obj->normalized_val_ =
                                own_dest_obj->normalized_val_;
                            }
                          break;
                        case EditType::EditorFunction:
                          obj->set_position_unvalidated (
                            own_dest_obj->get_position ());
                          if constexpr (std::derived_from<ObjT, BoundedObject>)
                            {
                              obj->end_pos_->set_to_position (
                                *own_dest_obj->end_pos_);
                            }
                          if constexpr (std::derived_from<ObjT, LoopableObject>)
                            {
                              obj->clip_start_pos_ =
                                own_dest_obj->clip_start_pos_;
                              obj->loop_start_pos_ =
                                own_dest_obj->loop_start_pos_;
                              obj->loop_end_pos_ = own_dest_obj->loop_end_pos_;
                            }
                          if constexpr (std::derived_from<ObjT, MuteableObject>)
                            {
                              obj->muted_ = own_dest_obj->muted_;
                            }
                          if constexpr (std::is_same_v<ObjT, MidiNote>)
                            {
                              obj->pitch_ = own_dest_obj->pitch_;
                              obj->vel_->set_val (own_dest_obj->vel_->vel_);
                            }
                          if constexpr (std::is_same_v<ObjT, AutomationPoint>)
                            {
                              obj->curve_opts_ = own_dest_obj->curve_opts_;
                              obj->fvalue_ = own_dest_obj->fvalue_;
                              obj->normalized_val_ =
                                own_dest_obj->normalized_val_;
                            }
                          break;
                        case EditType::Scale:
                          {
                            if constexpr (std::is_same_v<ObjT, ScaleObject>)
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

  update_region_link_groups (*dest_sel);

  // auto sel = get_actual_arranger_selections ();
  /* EVENTS_PUSH
   * (EventType::ET_ARRANGER_SELECTIONS_CHANGED_REDRAW_EVERYTHING, sel);
   */

  first_run_ = false;
#endif
}

void
ArrangerSelectionsAction::do_or_undo_automation_fill (bool do_it)
{
// TODO
#if 0
  if (!first_run_)
    {
      /* clear current selections in the project */
      TRACKLIST->clear_selections_for_object_siblings (
        ArrangerObjectSpan::uuid_projection (sel_->front ()));

      /* get the actual object from the project */
      auto region = std::get<
        AutomationRegion *> (*PROJECT->find_arranger_object_by_id (
        std::get<AutomationRegion *> (
          do_it ? region_before_.value () : region_after_.value ())
          ->get_uuid ()));
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
      PROJECT->getArrangerObjectFactory ()
        ->get_selection_manager_for_object (*prj_obj)
        .append_to_selection (prj_obj->get_uuid ());

      /* remember new info */
      ArrangerObjectSpan::copy_arranger_object_identifier (
        std::get<AutomationRegion *> (do_it ? *region_after_ : *region_before_),
        prj_obj);
    }

  first_run_ = false;
#endif
}

void
ArrangerSelectionsAction::do_or_undo_split (bool do_it)
{
#if 0
  for (const auto &[index, own_obj_var] : utils::views::enumerate (*sel_))
    {
      std::visit (
        [&] (auto &&own_obj) {
          using ObjT = base_type<decltype (own_obj)>;
          // own_obj->flags_ |= ArrangerObject::Flags::NonProject;
          if constexpr (BoundedObject<ObjT>)
            {
              if (do_it)
                {
                  /* remove the original object from the project */
                  auto obj = std::get<ObjT *> (
                    *PROJECT->find_arranger_object_by_id (own_obj->get_uuid ()));
                  z_return_if_fail (obj);

                  // TODO
                  // obj->remove_from_project (true);

                  if (first_run_)
                    {
                      /* split */
                      auto [r1, r2] = ArrangerObjectSpan::split_bounded_object (
                        *obj, *PROJECT->getArrangerObjectFactory (), pos_);

                      // TODO: re-add the additional logic that was removed from
                      // BoundedObject::split()
                      // to add the objects into the project

                      r1_[index] = r1.id ();
                      r2_[index] = r2.id ();
                    }
                  else
                    {
                      for (
                        const auto &[r1_var, r2_var] : std::views::zip (
                          ArrangerObjectSpan{
                            get_arranger_object_registry (), r1_ },
                          ArrangerObjectSpan{
                            get_arranger_object_registry (), r2_ }))
                        {
                          auto * r1 = std::get<ObjT *> (r1_var);
                          auto * r2 = std::get<ObjT *> (r2_var);
                          (void) r1;
                          (void) r2;
                          // TODO: add r1 & r2 to the project
                        }
                    }
                }
              /* else if undoing split */
              else
                {
// TODO
/* find the actual objects and remove from project */
#  if 0
                  auto * r1 = std::get<ObjT *> (
                    get_arranger_object_registry ().find_by_id_or_throw (
                      r1_.at (index)));
                  auto * r2 = std::get<ObjT *> (
                    get_arranger_object_registry ().find_by_id_or_throw (
                      r2_.at (index)));
                  r1->remove_from_project (true);
                  r2->remove_from_project (true);
#  endif

                  /* re-insert original object at its original position */
                  auto prj_obj = std::get<ObjT *> (
                    get_arranger_object_registry ().find_by_id_or_throw (
                      own_obj->get_uuid ()));
                  // TODO: insert to project
                  (void) prj_obj;
                }
            }
        },
        own_obj_var);
    }

  // ArrangerSelections * sel = get_actual_arranger_selections ();
  /* EVENTS_PUSH (EventType::ET_ARRANGER_SELECTIONS_CHANGED, sel); */

  first_run_ = false;
#endif
}

void
ArrangerSelectionsAction::do_or_undo_merge (bool do_it)
{
#if 0
  /* if first run, merge */
  if (first_run_)
    {
      sel_after_.emplace (std::vector<ArrangerObjectPtrVariant> ());
// TODO
#  if 0
      sel_after_->push_back(get_arranger_object_registry().find_by_id_or_throw(ArrangerObjectSpan{*sel_}.merge(get_frames_per_tick())));
#  endif
    }

  // sel->sort_by_indices (!do_it);
  // sel_after->sort_by_indices (!do_it);

  auto &before_objs = do_it ? *sel_ : *sel_after_;
  auto &after_objs = do_it ? *sel_after_ : *sel_;

  /* remove the before objects from the project */
  for (auto &own_before_obj_var : std::ranges::reverse_view (before_objs))
    {
      std::visit (
        [&] (auto &&own_before_obj) {
          using ObjT = base_type<decltype (own_before_obj)>;
          own_before_obj->flags_ |= ArrangerObject::Flags::NonProject;

          /* find the actual object */
          auto prj_obj = std::get<ObjT *> (
            *PROJECT->find_arranger_object_by_id (own_before_obj->get_uuid ()));

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

          auto prj_obj =
            std::get<ObjT *> (own_after_obj->add_clone_to_project (false));

          /* remember positions */
          ArrangerObjectSpan::copy_arranger_object_identifier (
            own_after_obj, prj_obj);
        },
        own_after_obj_var);
    }

  // ArrangerSelections * sel = get_actual_arranger_selections ();
  /* EVENTS_PUSH (EventType::ET_ARRANGER_SELECTIONS_CHANGED, sel); */

  first_run_ = false;
#endif
}

void
ArrangerSelectionsAction::do_or_undo_resize (bool do_it)
{
#if 0
  bool sel_after_existed = sel_after_.has_value ();
  if (!sel_after_.has_value ())
    {
      /* create the "after" selections here if not already given (e.g., when
       * the objects are already edited) */
      set_after_selections (ArrangerObjectSpan{ *sel_ });
    }

  auto &objs_before = *sel_;
  auto &objs_after = *sel_after_;

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
                  own_obj_before->flags_ |= ArrangerObject::Flags::NonProject;
                  own_obj_after->flags_ |= ArrangerObject::Flags::NonProject;

                  /* find the actual object */
                  auto obj = std::get<ObjT *> (*(
                    do_it
                      ? PROJECT->find_arranger_object_by_id (
                          own_obj_before->get_uuid ())
                      : PROJECT->find_arranger_object_by_id (
                          own_obj_after->get_uuid ())));

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
                      if constexpr (std::derived_from<ObjT, BoundedObject>)
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
                  PROJECT->getArrangerObjectFactory ()
                    ->get_selection_manager_for_object (*new_obj)
                    .append_to_selection (new_obj->get_uuid ());
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
#endif
}

void
ArrangerSelectionsAction::do_or_undo_quantize (bool do_it)
{
#if 0
  auto &objs = *sel_;
  auto &quantized_objs = *sel_after_;

  for (
    const auto &[own_obj_var, own_quantized_obj_var] :
    std::views::zip (objs, quantized_objs))
    {
      std::visit (
        [&] (auto &&own_obj) {
          using ObjT = base_type<decltype (own_obj)>;
          auto own_quantized_obj = std::get<ObjT *> (own_quantized_obj_var);
          if constexpr (
            std::is_same_v<ObjT, base_type<decltype (own_quantized_obj)>>)
            {
              own_obj->flags_ |= ArrangerObject::Flags::NonProject;
              own_quantized_obj->flags_ |= ArrangerObject::Flags::NonProject;

              /* find the actual object */
              auto obj = std::get<ObjT *> (
                get_arranger_object_registry ().find_by_id_or_throw (
                  own_obj->get_uuid ()));

              if (do_it)
                {
                  /* quantize it */
                  if (opts_->adjust_start_)
                    {
                      const auto &[new_pos, ticks] =
                        opts_->quantize_position (obj->get_position ());
                      obj->set_position_unvalidated (new_pos);
                      if constexpr (std::derived_from<ObjT, BoundedObject>)
                        {
                          obj->end_pos_->add_ticks (ticks, frames_per_tick_);
                        }
                    }
                  if (opts_->adjust_end_)
                    {
                      if constexpr (std::derived_from<ObjT, BoundedObject>)
                        {
                          const auto &[new_pos, _] =
                            opts_->quantize_position (obj->get_end_position ());
                          obj->set_end_position_unvalidated (new_pos);
                        }
                    }
                  obj->position_setter_validated (
                    obj->get_position (), AUDIO_ENGINE->ticks_per_frame_);
                  if constexpr (std::derived_from<ObjT, BoundedObject>)
                    {
                      obj->end_position_setter_validated (
                        *obj->end_pos_, AUDIO_ENGINE->ticks_per_frame_);
                    }

                  /* remember the quantized position so we can find the
                   * object when undoing */
                  own_quantized_obj->get_position ().set_to_position (
                    obj->get_position ());
                  if constexpr (std::derived_from<ObjT, BoundedObject>)
                    {
                      own_quantized_obj->end_pos_->set_to_position (
                        *obj->end_pos_);
                    }
                }
              else
                {
                  /* unquantize it */
                  obj->position_setter_validated (
                    own_obj->get_position (), AUDIO_ENGINE->ticks_per_frame_);
                  if constexpr (std::derived_from<ObjT, BoundedObject>)
                    {
                      obj->end_position_setter_validated (
                        own_obj->get_end_position (),
                        AUDIO_ENGINE->ticks_per_frame_);
                    }
                }
            }
        },
        own_obj_var);
    }

  // ArrangerSelections * sel = get_actual_arranger_selections ();
  /* EVENTS_PUSH (EventType::ET_ARRANGER_SELECTIONS_QUANTIZED, sel); */

  first_run_ = false;
#endif
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
  TRACKLIST->collection ()->get_track_span ().set_caches (
    CacheType::PlaybackSnapshots);

  /* reset new_lane_created */
// TODO/delete
#if 0
  for (auto track_var : TRACKLIST->get_track_span ())
    {
      std::visit (
        [&] (auto &&track) {
          using TrackT = base_type<decltype (track)>;
          if constexpr (std::derived_from<TrackT, LanedTrack>)
            {
              track->last_lane_created_ = 0;
              track->block_auto_creation_and_deletion_ = false;
              track->create_missing_lanes (track->lanes_.size () - 1);
              track->remove_empty_last_lanes ();
            }
        },
        track_var);
      }
#endif

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

QString
ArrangerSelectionsAction::to_string () const
{
  switch (type_)
    {
    case Type::Create:
      z_return_val_if_fail (sel_, {});
      return QObject::tr ("Create arranger objects");
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
}
