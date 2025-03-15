#include "gui/dsp/engine.h"
#include "gui/dsp/track_span.h"
#include "utils/debug.h"

#include "./arranger_object_span.h"

template <utils::UuidIdentifiableObjectPtrVariantRange Range>
auto
ArrangerObjectSpanImpl<Range>::merge (double frames_per_tick) const
  -> ArrangerObjectUuid
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
      auto num_frames =
        static_cast<unsigned_frame_t> (ceil (frames_per_tick * ticks_length));
      auto [first_obj_var, pos] = get_first_object_and_pos (true);
      Position end_pos{ pos.ticks_ + ticks_length, frames_per_tick };

      return std::visit (
        [&] (auto &&first_r) -> ArrangerObject::Uuid {
          using RegionT = base_type<decltype (first_r)>;
          if constexpr (std::derived_from<RegionT, Region>)
            {
              RegionT * new_r;
              if constexpr (std::is_same_v<RegionT, MidiRegion>)
                {
                  new_r = new MidiRegion (
                    pos, end_pos, first_r->id_.track_uuid_,
                    first_r->id_.lane_pos_, first_r->id_.idx_);
                  for (auto &obj : *this)
                    {
                      auto * r = std::get<MidiRegion *> (obj);
                      double ticks_diff =
                        r->pos_->ticks_ - first_r->pos_->ticks_;

                      for (auto &mn : r->midi_notes_)
                        {
                          auto new_mn = mn->clone_raw_ptr ();
                          new_mn->move (ticks_diff);
                          new_r->append_object (new_mn, false);
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

                  new_r = new AudioRegion (
                    frames, true, first_r_clip->get_name (), max_depth, pos,
                    first_r->id_.track_uuid_, first_r->id_.lane_pos_,
                    first_r->id_.idx_);
                }
              else if constexpr (std::is_same_v<RegionT, ChordRegion>)
                {
                  new_r = new ChordRegion (pos, end_pos, first_r->id_.idx_);
                  for (auto &obj : *this)
                    {
                      auto * r = std::get<ChordRegion *> (obj);
                      double ticks_diff =
                        r->pos_->ticks_ - first_r->pos_->ticks_;

                      for (auto &co : r->chord_objects_)
                        {
                          auto new_co = co->clone_raw_ptr ();
                          new_co->move (ticks_diff);
                          new_r->append_object (new_co, false);
                        }
                    }
                }
              else if constexpr (std::is_same_v<RegionT, AutomationRegion>)
                {
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
                          new_r->append_object (new_ap, false);
                        }
                    }
                }

              new_r->gen_name (first_r->name_.c_str (), nullptr, nullptr);
              return new_r->get_uuid ();
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

  auto first_obj_var = this->front ();
  return std::visit (
    [&] (auto &&first) {
      using FirstObjT = base_type<decltype (first)>;
      if constexpr (std::derived_from<FirstObjT, Region>)
        {
          const auto &id = first->id_;
          return std::ranges::all_of (*this, [&] (const auto &obj_var) {
            return std::visit (
              [&] (auto &&curr) {
                using CurObjT = base_type<decltype (curr)>;
                if constexpr (std::derived_from<CurObjT, Region>)
                  {
                    if (id.type_ != curr->id_.type_)
                      return false;

                    if constexpr (std::derived_from<CurObjT, LaneOwnedObject>)
                      return id.track_uuid_ == curr->id_.track_uuid_
                             && id.lane_pos_ == curr->id_.lane_pos_;
                    else if constexpr (std::is_same_v<CurObjT, AutomationRegion>)
                      return id.track_uuid_ == curr->id_.track_uuid_
                             && id.at_idx_ == curr->id_.at_idx_;
                    else
                      return true;
                  }
                else
                  {
                    return false;
                  }
              },
              obj_var);
          });
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
ArrangerObjectSpanImpl<Range>::init_loaded (bool project, double frames_per_tick)
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
                  o->read_from_pool_ = true;
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
                  o->validate (project, 0);
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

      if constexpr (std::derived_from<ObjT, RegionOwnedObject>)
        {
          dest->region_id_ = src->region_id_;
          dest->index_ = src->index_;
        }

      if constexpr (std::derived_from<ObjT, NamedObject>)
        {
          dest->name_ = src->name_;
        }

      if constexpr (std::derived_from<ObjT, Region>)
        {
          dest->id_ = src->id_;
        }

      if constexpr (std::is_same_v<ObjT, ChordObject>)
        {
          dest->chord_index_ = src->chord_index_;
        }
      if constexpr (std::is_same_v<ObjT, Marker>)
        {
          dest->marker_track_index_ = src->marker_track_index_;
        }
      if constexpr (std::is_same_v<ObjT, ScaleObject>)
        {
          dest->index_in_chord_track_ = src->index_in_chord_track_;
        }
    },
    dest_var);
}

template class ArrangerObjectSpanImpl<
  std::span<const ArrangerObjectSpan::VariantType>>;
template class ArrangerObjectSpanImpl<
  utils::UuidIdentifiableObjectSpan<ArrangerObjectRegistry>>;
