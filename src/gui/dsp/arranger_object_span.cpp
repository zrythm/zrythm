#include "gui/dsp/arranger_object_factory.h"
#include "gui/dsp/engine.h"
#include "gui/dsp/track_span.h"
#include "utils/debug.h"

#include "./arranger_object_span.h"

template <utils::UuidIdentifiableObjectPtrVariantRange Range>
auto
ArrangerObjectSpanImpl<Range>::merge (dsp::FramesPerTick frames_per_tick) const
  -> ArrangerObjectUuidReference
{
  bool is_timeline = std::ranges::all_of (
    *this, Base::template derived_from_type_projection<Region>);
  bool is_midi =
    std::ranges::all_of (*this, Base::template type_projection<MidiNote>);
  assert (is_timeline || is_midi);
  if (is_timeline)
    {
      if (!all_on_same_lane ())
        {
          throw ZrythmException ("selections not on same lane");
        }

      auto ticks_length = get_length_in_ticks ();
      auto num_frames = static_cast<unsigned_frame_t> (
        ceil (type_safe::get (frames_per_tick) * ticks_length));
      auto [first_obj_var, pos] = get_first_object_and_pos (true);
      Position end_pos{ pos.ticks_ + ticks_length, frames_per_tick };

      return std::visit (
        [&] (auto &&first_r) -> ArrangerObjectUuidReference {
          using RegionT = base_type<decltype (first_r)>;
          if constexpr (std::derived_from<RegionT, Region>)
            {
              const auto get_new_r = [] (auto &new_r_ref) {
                return std::get<RegionT *> (new_r_ref->get_object ());
              };
              std::optional<ArrangerObjectUuidReference> new_r;
              if constexpr (std::is_same_v<RegionT, MidiRegion>)
                {
                  new_r =
                    ArrangerObjectFactory::get_instance ()
                      ->get_builder<MidiRegion> ()
                      .with_start_ticks (pos.ticks_)
                      .with_end_ticks (end_pos.ticks_)
                      .build ();
                  for (auto &obj : *this)
                    {
                      auto * r = std::get<MidiRegion *> (obj);
                      double ticks_diff =
                        r->pos_->ticks_ - first_r->pos_->ticks_;

                      for (auto * mn : r->get_children_view ())
                        {
                          auto new_mn =
                            ArrangerObjectFactory::get_instance ()
                              ->clone_new_object_identity (*mn);
                          std::visit (
                            [&] (auto &&m) {
                              m->move (ticks_diff, frames_per_tick);
                            },
                            new_mn.get_object ());
                          get_new_r (new_r)->add_object (new_mn);
                        }
                    }
                }
              else if constexpr (std::is_same_v<RegionT, AudioRegion>)
                {
                  auto                      first_r_clip = first_r->get_clip ();
                  utils::audio::AudioBuffer frames{
                    first_r_clip->get_num_channels (),
                    static_cast<int> (num_frames)
                  };
                  auto max_depth = first_r_clip->get_bit_depth ();
                  assert (!first_r_clip->get_name ().empty ());
                  for (auto &obj : *this)
                    {
                      auto * r = std::get<AudioRegion *> (obj);
                      long   frames_diff =
                        r->pos_->frames_ - first_r->pos_->frames_;
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

// TODO
#if 0
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

              get_new_r (new_r)->gen_name (
                first_r->get_name (), nullptr, nullptr);
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

template <utils::UuidIdentifiableObjectPtrVariantRange Range>
bool
ArrangerObjectSpanImpl<Range>::all_on_same_lane () const
{
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
}

template <utils::UuidIdentifiableObjectPtrVariantRange Range>
std::pair<TrackPtrVariant, TrackPtrVariant>
ArrangerObjectSpanImpl<Range>::get_first_and_last_track () const
{
  auto tracks = *this | std::views::transform (track_projection);
  auto [min_it, max_it] =
    std::ranges::minmax_element (tracks, {}, TrackSpan::position_projection);
  return std::make_pair (*min_it, *max_it);
}

template <utils::UuidIdentifiableObjectPtrVariantRange Range>
void
ArrangerObjectSpanImpl<Range>::init_loaded (
  bool               project,
  dsp::FramesPerTick frames_per_tick)
{
  for (auto &obj_variant : *this)
    {
      std::visit (
        [&] (auto &&o) {
          using ObjectT = base_type<decltype (o)>;
          if (project)
            { /* throws an error otherwise */
              if constexpr (std::is_same_v<AudioRegion, ObjectT>)
                {
                  // o->read_from_pool_ = true;
                  auto clip = o->get_clip ();
                  z_return_if_fail (clip);
                }
              o->update_positions (true, false, frames_per_tick);
            }
          else /* else if not project */
            {
              o->init_loaded ();
              o->update_positions (true, false, frames_per_tick);
              if constexpr (std::derived_from<ObjectT, Region>)
                {
                  if constexpr (std::is_same_v<AudioRegion, ObjectT>)
                    {
                      o->fix_positions (frames_per_tick);
                      o->validate (project, frames_per_tick);
                    }
                  o->validate (project, frames_per_tick);
                }
            }
        },
        obj_variant);
    }
}

template <utils::UuidIdentifiableObjectPtrVariantRange Range>
auto
ArrangerObjectSpanImpl<Range>::get_first_object_and_pos (bool global) const
  -> std::pair<VariantType, Position>
{
  auto ret_obj = *std::ranges::min_element (*this, {}, position_projection);
  auto ret_pos = position_projection (ret_obj);
  if (global)
    {
      std::visit (
        [&] (auto &&obj) {
          using ObjT = base_type<decltype (obj)>;
          if constexpr (std::derived_from<ObjT, RegionOwnedObject>)
            {
              auto region = obj->get_region ();
              ret_pos.add_ticks (
                region->pos_->ticks_, AUDIO_ENGINE->frames_per_tick_);
            }
        },
        ret_obj);
    }

  return { ret_obj, ret_pos };
}

template <utils::UuidIdentifiableObjectPtrVariantRange Range>
auto
ArrangerObjectSpanImpl<Range>::get_last_object_and_pos (
  bool global,
  bool ends_last) const -> std::pair<VariantType, Position>
{
  const auto proj =
    ends_last
      ? end_position_with_start_position_fallback_projection
      : position_projection;
  auto ret_obj = *std::ranges::min_element (*this, {}, proj);
  auto ret_pos = proj (ret_obj);
  if (global)
    {
      std::visit (
        [&] (auto &&obj) {
          using ObjT = base_type<decltype (obj)>;
          if constexpr (std::derived_from<ObjT, RegionOwnedObject>)
            {
              auto region = obj->get_region ();
              ret_pos.add_ticks (
                region->pos_->ticks_, AUDIO_ENGINE->frames_per_tick_);
            }
        },
        ret_obj);
    }

  return { ret_obj, ret_pos };
}

template <utils::UuidIdentifiableObjectPtrVariantRange Range>
bool
ArrangerObjectSpanImpl<Range>::can_be_merged () const
{
  return this->size () > 1
         && std::ranges::all_of (
           *this, Base::template derived_from_type_projection<BoundedObject>)
         && all_on_same_lane () && std::ranges::none_of (*this, looped_projection);
}

template <utils::UuidIdentifiableObjectPtrVariantRange Range>
void
ArrangerObjectSpanImpl<Range>::copy_arranger_object_identifier (
  const VariantType &dest_var,
  const VariantType &src_var)
{
  std::visit (
    [&] (auto &&dest) {
      using ObjT = base_type<decltype (dest)>;
      auto src = std::get<ObjT *> (src_var);
      dest->track_id_ = src->track_id_;
      // TODO?
      // dest->uuid_ = src->get_uuid();

      if constexpr (std::derived_from<ObjT, RegionOwnedObject>)
        {
          dest->region_id_ = src->region_id_;
          // dest->index_ = src->index_;
        }

      if constexpr (std::derived_from<ObjT, NamedObject>)
        {
          dest->set_name (src->get_name ());
        }

      if constexpr (std::is_same_v<ObjT, ChordObject>)
        {
          dest->chord_index_ = src->chord_index_;
        }
    },
    dest_var);
}

template class ArrangerObjectSpanImpl<
  std::span<const ArrangerObjectSpan::VariantType>>;
template class ArrangerObjectSpanImpl<
  utils::UuidIdentifiableObjectSpan<ArrangerObjectRegistry>>;
template class ArrangerObjectSpanImpl<utils::UuidIdentifiableObjectSpan<
  ArrangerObjectRegistry,
  ArrangerObjectUuidReference>>;
