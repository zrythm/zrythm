// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "structure/arrangement/arranger_object_factory.h"
#include "structure/arrangement/arranger_object_span.h"

namespace zrythm::structure::arrangement
{
auto
ArrangerObjectSpan::merge () const -> ArrangerObjectUuidReference
{
  bool is_timeline = std::ranges::all_of (*this, is_region_projection);
  [[maybe_unused]] bool is_midi =
    std::ranges::all_of (*this, Base::template type_projection<MidiNote>);
  assert (is_timeline || is_midi);
  if (is_timeline)
    {
      if (!all_on_same_lane ())
        {
          throw ZrythmException ("selections not on same lane");
        }

      auto ticks_length = units::ticks (get_length_in_ticks ());
      auto [first_obj_var, pos] = get_first_object_and_pos ();
      auto start_pos = units::ticks (pos);
      auto end_pos = start_pos + ticks_length;

      return std::visit (
        [&] (auto &&first_r) -> ArrangerObjectUuidReference {
          using RegionT = base_type<decltype (first_r)>;
          if constexpr (RegionObject<RegionT>)
            {
              const auto get_new_r = [] (auto &new_r_ref) {
                return std::get<RegionT *> (new_r_ref->get_object ());
              };
              std::optional<ArrangerObjectUuidReference> new_r;
              if constexpr (std::is_same_v<RegionT, MidiRegion>)
                {
                  new_r =
                    PROJECT->arrangerObjectFactory ()
                      ->get_builder<MidiRegion> ()
                      .with_start_ticks (pos)
                      .with_end_ticks (end_pos.in (units::ticks))
                      .build_in_registry ();
                  for (const auto &obj : *this)
                    {
                      auto * r = std::get<MidiRegion *> (obj);
                      double ticks_diff =
                        r->position ()->ticks ()
                        - first_r->position ()->ticks ();

                      for (auto * mn : r->get_children_view ())
                        {
                          auto new_mn =
                            PROJECT->arrangerObjectFactory ()
                              ->clone_new_object_identity (*mn);
                          std::visit (
                            [&] (auto &&m) {
                              m->position ()->addTicks (ticks_diff);
                            },
                            new_mn.get_object ());
                          get_new_r (new_r)->add_object (new_mn);
                        }
                    }
                }
              else if constexpr (std::is_same_v<RegionT, AudioRegion>)
                {
// TODO
#if 0
                  auto                      first_r_clip = first_r->get_clip ();
                  utils::audio::AudioBuffer frames{
                    first_r_clip->get_num_channels (),
                    static_cast<int> (num_frames)
                  };
                  auto max_depth = first_r_clip->get_bit_depth ();
                  assert (!first_r_clip->get_name ().empty ());
                  for (const auto &obj : *this)
                    {
                      auto * r = std::get<AudioRegion *> (obj);
                      long   frames_diff =
                        r->get_position ().frames_
                        - first_r->get_position ().frames_;
                      long r_frames_length = r->get_length_in_frames ();

                      auto * clip = r->get_clip ();
                      for (int i = 0; i < frames.getNumChannels (); ++i)
                        {
                          utils::float_ranges::add2 (
                            &frames.getWritePointer (i)[frames_diff],
                            clip->get_samples ().getReadPointer (i),
                            static_cast<size_t> (r_frames_length));
                        }

                      max_depth = std::max (max_depth, clip->get_bit_depth ());
                    }

                  new_r = new AudioRegion (
                    frames, true, first_r_clip->get_name (), max_depth, pos,
                    first_r->get_track_id(), first_r->id_.lane_pos_,
                    first_r->id_.idx_);
#endif
                }
              else if constexpr (std::is_same_v<RegionT, ChordRegion>)
                {
// TODO
#if 0
                  new_r = new ChordRegion (
                    pos, end_pos, first_r->id_.idx_,
                    AUDIO_ENGINE->ticks_per_frame_);
                  for (auto &obj : *this)
                    {
                      auto * r = std::get<ChordRegion *> (obj);
                      double ticks_diff =
                        r->pos_->ticks_ - first_r->pos_->ticks_;

                      for (auto &co : r->chord_objects_)
                        {
                          auto new_co = co->clone_raw_ptr ();
                          new_co->move (ticks_diff);
                          new_r->add_object (new_co, false);
                        }
                    }
#endif
                }
              else if constexpr (std::is_same_v<RegionT, AutomationRegion>)
                {
// TODO
#if 0
                  new_r = new AutomationRegion (
                    pos, end_pos, first_r->id_.track_uuid_,
                    first_r->id_.at_idx_, first_r->id_.idx_);
                  for (auto &obj : *this)
                    {
                      auto * r = std::get<AutomationRegion *> (obj);
                      double ticks_diff =
                        r->pos_->ticks_ - first_r->pos_->ticks_;

                      for (auto &ap : r->aps_)
                        {
                          auto * new_ap = ap->clone_raw_ptr ();
                          new_ap->move (ticks_diff);
                          new_r->add_object (new_ap, false);
                        }
                    }
#endif
                }

              get_new_r (new_r)->name ()->setName (first_r->name ()->name ());
              return *new_r;
            }
          else
            {
              throw std::runtime_error ("merging non-regions unimplemented");
            }
        },
        first_obj_var);
    }
  else
    {
      throw std::runtime_error ("merging midi notes unimplemented");
    }
}

bool
ArrangerObjectSpan::all_on_same_lane () const
{
  return true;

// TODO
#if 0
  if (this->empty ())
    return true;

  // either LaneOwnedObject or AutomationRegion
  auto first_obj_var = this->front ();
  return std::visit (
    [&] (auto &&first) {
      using FirstObjT = base_type<decltype (first)>;
      if (
        !std::ranges::all_of (*this, Base::template type_projection<FirstObjT>))
        return false;

      if constexpr (std::derived_from<FirstObjT, LaneOwnedObject>)
        {
          const auto &lane = first->get_lane ();
          return std::ranges::all_of (
            Base::template as_type<FirstObjT> (), [&] (auto &&obj) {
              return std::addressof (obj->get_lane ()) == std::addressof (lane);
            });
        }
      else if constexpr (std::is_same_v<FirstObjT, AutomationRegion>)
        {
          const auto at = first->get_automation_track ();
          return std::ranges::all_of (
            Base::template as_type<FirstObjT> (),
            [&] (auto &&obj) { return obj->get_automation_track () == at; });
        }
      else
        {
          return false;
        }
    },
    first_obj_var);
#endif
}

auto
ArrangerObjectSpan::get_first_object_and_pos () const
  -> std::pair<VariantType, double>
{
  auto ret_obj =
    *std::ranges::min_element (*this, {}, position_ticks_projection);
  auto ret_pos = position_ticks_projection (ret_obj);
  return { ret_obj, ret_pos };
}

auto
ArrangerObjectSpan::get_last_object_and_pos (bool ends_last) const
  -> std::pair<VariantType, double>
{
  const auto proj =
    ends_last
      ? end_position_ticks_with_start_position_fallback_projection
      : position_ticks_projection;
  auto ret_obj = *std::ranges::min_element (*this, {}, proj);
  auto ret_pos = proj (ret_obj);
  return { ret_obj, ret_pos };
}

bool
ArrangerObjectSpan::can_be_merged () const
{
  return this->size () > 1 && std::ranges::all_of (*this, bounded_projection)
         && all_on_same_lane () && std::ranges::none_of (*this, looped_projection);
}
}
