// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "structure/arrangement/arranger_object_all.h"
#include "utils/debug.h"
#include "utils/uuid_identifiable_object.h"

namespace zrythm::structure::arrangement
{

/**
 * Flag used in some functions.
 */
enum class ResizeType : basic_enum_base_type_t
{
  Normal,
  Loop,
  Fade,
  Stretch,
  /**
   * Used when we want to resize to contents when BPM changes.
   *
   * @note Only applies to audio.
   */
  StretchTempoChange,
};

/**
 * Resizes the object on the left side or right side by given amount of ticks,
 * for objects that do not have loops (currently none? keep it as reference).
 *
 * @param left 1 to resize left side, 0 to resize right side.
 * @param ticks Number of ticks to resize.
 *
 * @throw ZrythmException on failure.
 */
template <BoundedObject ObjectT>
void
resize_bounded_object (ObjectT &obj, bool left, ResizeType type, double ticks)
{
  z_trace ("resizing object( left: {}, type: {}, ticks: {})", left, type, ticks);

  // fade resizes are handled directly (not via this function)

  if (left)
    {
      if (type != ResizeType::Fade)
        {
          obj.getPosition ()->setTicks (obj.getPosition ()->ticks () + ticks);

          if constexpr (FadeableObject<ObjectT>)
            {
              obj.get_fade_range ().startOffset->setTicks (
                obj.get_fade_range ().startOffset->ticks () - ticks);
            }

          if (type == ResizeType::Loop)
            {
              if constexpr (RegionObject<ObjectT>)
                {
                  double loop_len =
                    obj.get_loop_range ().get_loop_length_in_ticks ();

                  // if clip start is not before loop start, adjust clip start pos
                  const auto loop_start_pos =
                    obj.get_loop_range ().loopStartPosition ()->ticks ();
                  auto clip_start_pos =
                    obj.get_loop_range ().clipStartPosition ()->ticks ();
                  if (clip_start_pos >= loop_start_pos)
                    {
                      clip_start_pos += ticks;

                      while (clip_start_pos < loop_start_pos)
                        {
                          clip_start_pos += loop_len;
                        }
                      obj.get_loop_range ().clipStartPosition ()->setTicks (
                        clip_start_pos);
                    }

                  /* make sure clip start goes back to loop start if it exceeds
                   * loop end */
                  const auto loop_end_pos =
                    obj.get_loop_range ().loopEndPosition ()->ticks ();
                  while (clip_start_pos > loop_end_pos)
                    {
                      clip_start_pos += -loop_len;
                    }
                  obj.get_loop_range ().clipStartPosition ()->setTicks (
                    clip_start_pos);
                }
            }
          else if constexpr (RegionObject<ObjectT>)
            {
              obj.get_loop_range ().loopEndPosition ()->setTicks (
                obj.get_loop_range ().loopEndPosition ()->ticks () - ticks);

              /* move containing items */
              if constexpr (
                is_derived_from_template_v<ArrangerObjectOwner, ObjectT>)
                {
                  obj.add_ticks_to_children (-ticks);
                }
            }
        }
    }
  /* else if resizing right side */
  else
    {
      if (type != ResizeType::Fade)
        {
          obj.length ()->setTicks (obj.length ()->ticks () + ticks);

          const auto change_ratio =
            (obj.length ()->ticks ()) / (obj.length ()->ticks () - ticks);

          if (type != ResizeType::Loop)
            {
              if constexpr (RegionObject<ObjectT>)
                {
                  if (
                    type == ResizeType::StretchTempoChange
                    || type == ResizeType::Stretch)
                    {
                      obj.get_loop_range ().loopEndPosition ()->setTicks (
                        obj.get_loop_range ().loopEndPosition ()->ticks ()
                        * change_ratio);
                    }
                  else
                    {
                      obj.get_loop_range ().loopEndPosition ()->setTicks (
                        obj.get_loop_range ().loopEndPosition ()->ticks ()
                        + ticks);
                    }

                  /* if stretching, also stretch loop start */
                  if (
                    type == ResizeType::StretchTempoChange
                    || type == ResizeType::Stretch)
                    {
                      obj.get_loop_range ().loopStartPosition ()->setTicks (
                        obj.get_loop_range ().loopStartPosition ()->ticks ()
                        * change_ratio);
                    }
                }
            }
          if constexpr (FadeableObject<ObjectT>)
            {
              // we only need changes when stretching
              if (
                type == ResizeType::StretchTempoChange
                || type == ResizeType::Stretch)
                {
                  obj.get_fade_range ().endOffset ()->setTicks (
                    obj.get_fade_range ().endOffset ()->ticks () * change_ratio);
                  obj.get_fade_range ().startOffset ()->setTicks (
                    obj.get_fade_range ().startOffset ()->ticks ()
                    * change_ratio);
                }
            }

// TODO
#if 0
                if (type == ResizeType::Stretch)
                  {
                    if constexpr (std::derived_from<ObjT, Region>)
                      {
                        double new_length = get_length_in_ticks ();

                        if (type != ResizeType::StretchTempoChange)
                          {
                            /* FIXME this flag is not good,
                             * remove from this function and
                             * do it in the arranger */
                            if (during_ui_action)
                              {
                                self->stretch_ratio_ =
                                  new_length / self->before_length_;
                              }
                            /* else if as part of an action */
                            else
                              {
                                /* stretch contents */
                                double stretch_ratio = new_length / before_length;
                                try
                                  {
                                    self->stretch (stretch_ratio);
                                  }
                                catch (const ZrythmException &ex)
                                  {
                                    throw ZrythmException (
                                      "Failed to stretch region");
                                  }
                              }
                          }
                      }
                  }
#endif
        }
    }
}

/**
 * Stretch the region's contents.
 *
 * This should be called right after changing the region's size.
 *
 * @param ratio The ratio to stretch by.
 *
 * @throw ZrythmException on error.
 */
template <RegionObject RegionT>
void
stretch_region_contents (RegionT &r, double ratio)
{
  z_debug ("stretching region {} (ratio {:f})", r, ratio);

// TODO
#if 0
  if constexpr (std::is_same_v<RegionT, AudioRegion>)
    {
      auto * clip = r.get_clip ();
      auto new_clip_id = AUDIO_POOL->duplicate_clip (clip->get_uuid (), false);
      auto * new_clip = AUDIO_POOL->get_clip (new_clip_id);
      r.set_clip_id (new_clip->get_uuid ());

      auto stretcher = dsp::Stretcher::create_rubberband (
        AUDIO_ENGINE->get_sample_rate (), new_clip->get_num_channels (), ratio,
        1.0, false);

      auto buf = new_clip->get_samples ();
      buf.interleave_samples ();
      auto stretched_buf = stretcher->stretch_interleaved (buf);
      stretched_buf.deinterleave_samples (new_clip->get_num_channels ());
      new_clip->clear_frames ();
      new_clip->expand_with_frames (stretched_buf);
      auto num_frames_per_channel = new_clip->get_num_frames ();
      z_return_if_fail (num_frames_per_channel > 0);

      AUDIO_POOL->write_clip (*new_clip, false, false);

      /* readjust end position to match the number of frames exactly */
      dsp::Position new_end_pos (
        static_cast<signed_frame_t> (num_frames_per_channel),
        AUDIO_ENGINE->ticks_per_frame_);

      r.loopEndPosition ()->setSamples (num_frames_per_channel);
      r.length ()->setSamples (num_frames_per_channel);
    }
  else
    {
      auto objs = r.get_children_view ();
      for (auto * obj : objs)
        {
          using ObjT = base_type<decltype (obj)>;
          /* set start pos */
          double        before_ticks = obj->get_position ().ticks_;
          double        new_ticks = before_ticks * ratio;
          dsp::Position tmp (new_ticks, AUDIO_ENGINE->frames_per_tick_);
          obj->position_setter_validated (tmp, AUDIO_ENGINE->ticks_per_frame_);

          if constexpr (std::derived_from<ObjT, BoundedObject>)
            {
              /* set end pos */
              before_ticks = obj->end_pos_->ticks_;
              new_ticks = before_ticks * ratio;
              tmp = dsp::Position (new_ticks, AUDIO_ENGINE->frames_per_tick_);
              obj->end_position_setter_validated (
                tmp, AUDIO_ENGINE->ticks_per_frame_);
            }
        }
    }
#endif
}

/**
 * @brief Track span that offers helper methods on a range of tracks.
 */
class ArrangerObjectSpan
    : public utils::UuidIdentifiableObjectView<ArrangerObjectRegistry>
{
public:
  using Base = utils::UuidIdentifiableObjectView<ArrangerObjectRegistry>;
  using VariantType = typename Base::VariantType;
  using ArrangerObjectUuid = typename Base::UuidType;
  using Base::Base; // Inherit constructors

  static auto name_projection (const VariantType &obj_var)
  {
    return std::visit (
      [] (const auto &obj) -> utils::Utf8String {
        using ObjT = base_type<decltype (obj)>;
        if constexpr (NamedObject<ObjT>)
          {
            if constexpr (RegionObject<ObjT>)
              {
                return obj->name ()->get_name ();
              }
            else if constexpr (std::is_same_v<ObjT, Marker>)
              {
                return obj->name ()->get_name ();
              }
          }
        else
          {
            throw std::runtime_error (
              "Name projection called on non-named object");
          }
      },
      obj_var);
  }
  static auto position_ticks_projection (const VariantType &obj_var)
  {
    return std::visit (
      [&] (auto &&obj) { return obj->position ()->ticks (); }, obj_var);
  }
  static auto end_position_ticks_with_start_position_fallback_projection (
    const VariantType &obj_var)
  {
    return std::visit (
      [&] (auto &&obj) {
        using ObjT = base_type<decltype (obj)>;
        auto ticks = obj->position ()->ticks ();
        if constexpr (BoundedObject<ObjT>)
          {
            if constexpr (RegionObject<ObjT>)
              {
                return ticks + obj->bounds ()->length ()->ticks ();
              }
            else
              {
                return ticks + obj->bounds ()->length ()->ticks ();
              }
          }
        else
          {
            return ticks;
          }
      },
      obj_var);
  }
  static auto midi_note_pitch_projection (const VariantType &obj_var)
  {
    return std::get<MidiNote *> (obj_var)->pitch ();
  }
  static auto looped_projection (const VariantType &obj_var)
  {
    return std::visit (
      [] (const auto &obj) {
        using ObjT = base_type<decltype (obj)>;
        if constexpr (RegionObject<ObjT>)
          {
            return obj->loopRange ()->is_looped ();
          }
        else
          return false;
      },
      obj_var);
  }
  static auto is_timeline_object_projection (const VariantType &obj_var)
  {
    return std::visit (
      [] (const auto &ptr) { return TimelineObject<base_type<decltype (ptr)>>; },
      obj_var);
  }
  static auto is_editor_object_projection (const VariantType &obj_var)
  {
    return !is_timeline_object_projection (obj_var);
  }
  static auto deletable_projection (const VariantType &obj_var)
  {
    return std::visit (
      [&] (auto &&obj) { return is_arranger_object_deletable (*obj); }, obj_var);
  }
  static auto cloneable_projection (const VariantType &obj_var)
  {
    return deletable_projection (obj_var);
  }
  static auto renameable_projection (const VariantType &obj_var)
  {
    return std::visit (
             [] (const auto &ptr) {
               return NamedObject<base_type<decltype (ptr)>>;
             },
             obj_var)
           && deletable_projection (obj_var);
  }
  static auto bounded_projection (const VariantType &obj_var)
  {
    return std::visit (
      [] (const auto &ptr) { return BoundedObject<base_type<decltype (ptr)>>; },
      obj_var);
  }
  static auto is_region_projection (const VariantType &obj_var)
  {
    return std::visit (
      [] (const auto &ptr) { return RegionObject<base_type<decltype (ptr)>>; },
      obj_var);
  }
  static auto bounds_projection (const VariantType &obj_var)
  {
    return std::visit (
      [] (const auto &ptr) -> ArrangerObjectBounds * {
        using ObjT = base_type<decltype (ptr)>;
        if constexpr (BoundedObject<ObjT>)
          {
            return get_object_bounds (*ptr);
          }
        else
          {
            throw std::runtime_error ("Not a bounded object");
          }
      },
      obj_var);
  }

// deprecated - no snapshots - use to/from json
#if 0
  std::vector<VariantType>
  create_snapshots (const auto &object_factory, QObject &owner) const
  {
    return std::ranges::to<std::vector> (
      *this | std::views::transform ([&] (const auto &obj_var) {
        return std::visit (
          [&] (auto &&obj) -> VariantType {
            return object_factory.clone_object_snapshot (*obj, owner);
          },
          obj_var);
      }));
  }
#endif

  auto create_new_identities (const auto &object_factory) const
    -> std::vector<ArrangerObjectUuidReference>
  {
    return std::ranges::to<std::vector> (
      *this | std::views::transform ([&] (const auto &obj_var) {
        return std::visit (
          [&] (auto &&obj) -> VariantType {
            return object_factory.clone_new_object_identity (*obj);
          },
          obj_var);
      }));
  }

  /**
   * Sorts the selections by their indices (eg, for regions, their track
   * indices, then the lane indices, then the index in the lane).
   *
   * @note Only works for objects whose tracks exist.
   *
   * @param desc Descending or not.
   */
  std::vector<VariantType> sort_by_indices (bool desc);

  // std::vector<VariantType> sort_by_positions (bool desc);

  /**
   * Gets first object of the given type (if any, otherwise matches all types)
   * and its start position.
   */
  auto get_first_object_and_pos () const -> std::pair<VariantType, double>;

  /**
   * Gets last object of the given type (if any, otherwise matches all types)
   * and its end (if applicable, otherwise start) position.
   *
   * @param ends_last Whether to get the object that ends last, otherwise the
   * object that starts last.
   */
  auto get_last_object_and_pos (bool ends_last) const
    -> std::pair<VariantType, double>;

  std::pair<MidiNote *, MidiNote *> get_first_and_last_note () const
  {
    auto midi_notes =
      *this
      | std::views::transform (Base::template type_transformation<MidiNote>);
    auto [min_it, max_it] =
      std::ranges::minmax_element (midi_notes, {}, &MidiNote::pitch);
    return { *min_it, *max_it };
  }

  /**
   * Returns if the selections contain an undeletable object (such as the
   * start marker).
   */
  bool contains_undeletable_object () const
  {
    return !std::ranges::all_of (*this, deletable_projection);
  }

  /**
   * Returns if the selections contain an unclonable object (such as the start
   * marker).
   */
  bool contains_unclonable_object () const
  {
    return !std::ranges::all_of (*this, cloneable_projection);
  }

  /** Whether the selections contain an unrenameable object (such as the start
   * marker). */
  bool contains_unrenamable_object () const
  {
    return !std::ranges::all_of (*this, renameable_projection);
  }

  /**
   * Checks whether an object matches the given parameters.
   *
   * If a parameter should be checked, the has_* argument must be true and the
   * corresponding argument must have the value to be checked against.
   */
  // bool contains_object_with_property (Property property, bool value) const;

  /**
   * Merges the objects into one.
   *
   * @note All selections must be on the same lane.
   */
  auto merge () const -> ArrangerObjectUuidReference;

  /**
   * Returns if the selections can be pasted at the current place/playhead.
   */
  bool can_be_pasted () const;

  bool all_on_same_lane () const;

  bool contains_looped () const
  {
    return std::ranges::any_of (*this, looped_projection);
  };

  bool can_be_merged () const;

  double get_length_in_ticks () const
  {
    auto [_1, p1] = get_first_object_and_pos ();
    auto [_2, p2] = get_last_object_and_pos (true);
    return p2 - p1;
  }

  /**
   * Returns the region at the given position, or NULL.
   *
   * @param include_region_end Whether to include the region's end in the
   * calculation.
   */
  std::optional<VariantType> get_bounded_object_at_position (
    units::sample_t pos_samples,
    bool            include_region_end = false) const
  {
    auto view = *this | std::views::filter (bounded_projection);
    auto it = std::ranges::find_if (view, [&] (const auto &r_var) {
      auto bounds = bounds_projection (r_var);
      return bounds->is_hit (pos_samples, include_region_end);
    });
    return it != view.end () ? std::make_optional (*it) : std::nullopt;
  }

// TODO: implement elsewhere
#if 0
  /**
   * Sets the listen status of notes on and off based on changes in the previous
   * selections and the current selections.
   */
  void unlisten_note_diff (const ArrangerObjectSpan &prev) const
  {
    // Check for notes in prev that are not in objects_ and stop listening to
    // them
    for (const auto &prev_mn : prev.template get_elements_by_type<MidiNote> ())
      {
        if (std::ranges::none_of (*this, [&prev_mn] (const auto &obj) {
              auto mn = std::get<MidiNote *> (obj);
              return *mn == *prev_mn;
            }))
          {
            prev_mn->listen (false);
          }
      }
  }
#endif

  // void sort_by_pitch (bool desc);

  /**
   * Splits the given object at the given position and returns a pair of
   * newly-created objects (with unique identities).
   *
   * @param object_var The object to split.
   * @param global_pos The position to split at (global).
   * @return A pair of the new objects created.
   * @note This doesn't modify anything inside the project. The caller is
   * responsible removing/adding objects to/from the project.
   */
  template <BoundedObject BoundedObjectT>
  static auto split_bounded_object (
    const BoundedObjectT &self,
    const auto           &factory,
    signed_frame_t        global_pos)
    -> std::pair<ArrangerObjectUuidReference, ArrangerObjectUuidReference>
  {
    const auto &tempo_map = self.get_tempo_map ();
    /* create the new objects as new identities */
    auto       new_object1_ref = factory.clone_new_object_identity (self);
    auto       new_object2_ref = factory.clone_new_object_identity (self);
    const auto get_derived_object = [] (auto &obj_ref) {
      return std::get<BoundedObjectT *> (obj_ref.get_object ());
    };

    z_debug ("splitting objects...");

    /* get global/local positions (the local pos is after traversing the
     * loops) */
    auto local_pos = [&] () {
      auto local_frames = global_pos;
      if constexpr (TimelineObject<BoundedObjectT>)
        {
          local_frames = timeline_frames_to_local (self, global_pos, true);
        }
      return local_frames;
    }();

    /*
     * for first object set:
     * - end pos (fade out position follows it)
     */
    ArrangerObjectSpan::bounds_projection (get_derived_object (new_object1_ref))
      ->length ()
      ->setSamples (
        global_pos
        - get_derived_object (new_object1_ref)->position ()->samples ());

    if constexpr (RegionObject<BoundedObjectT>)
      {
        /* if original object was not looped, make the new object unlooped
         * also */
        if (!self.loopRange ()->is_looped ())
          {
            if constexpr (std::is_same_v<BoundedObjectT, AudioRegion>)
              {
// TODO
#if 0
                /* create new audio region */
                auto prev_r1_ref = new_object1_ref;
                auto prev_r1_clip =
                  get_derived_object (prev_r1_ref)->get_clip ();
                assert (prev_r1_clip);
                utils::audio::AudioBuffer frames{
                  prev_r1_clip->get_num_channels (),
                  static_cast<int> (local_pos.frames_)
                };
                for (int i = 0; i < prev_r1_clip->get_num_channels (); ++i)
                  {
                    frames.copyFrom (
                      i, 0, prev_r1_clip->get_samples (), i, 0,
                      static_cast<int> (local_pos.frames_));
                  }
                assert (!get_derived_object (prev_r1_ref)->get_name ().empty ());
                assert (local_pos.frames_ >= 0);
                new_object1_ref =
                  factory.create_audio_region_from_audio_buffer_FIXME (
                    get_derived_object (prev_r1_ref)->get_lane (), frames,
                    prev_r1_clip->get_bit_depth (),
                    get_derived_object (prev_r1_ref)->get_name (),
                    get_derived_object (prev_r1_ref)->get_position ().ticks_);
                assert (
                  get_derived_object (new_object1_ref)->get_clip_id ()
                  != get_derived_object (prev_r1_ref)->get_clip_id ());
#endif
              }
            else
              {
                /* remove objects starting after the end */
                for (
                  const auto * child :
                  get_derived_object (new_object1_ref)->get_children_view ())
                  {
                    if (child->position ()->samples () > local_pos)
                      {
                        get_derived_object (new_object1_ref)
                          ->remove_object (child->get_uuid ());
                      }
                  }
              }
          }
      }

    /*
     * for second object set:
     * - start pos
     * - clip start pos
     */
    if constexpr (RegionObject<BoundedObjectT>)
      {
        get_derived_object (new_object2_ref)
          ->loopRange ()
          ->clipStartPosition ()
          ->setSamples (local_pos);
      }
    get_derived_object (new_object2_ref)->position ()->setSamples (global_pos);
    signed_frame_t r2_local_end =
      ArrangerObjectSpan::bounds_projection (get_derived_object (new_object2_ref))
        ->get_end_position_samples (true);
    r2_local_end -=
      get_derived_object (new_object2_ref)->position ()->samples ();

    /* if original object was not looped, make the new object unlooped also */
    if constexpr (RegionObject<BoundedObjectT>)
      {
        if (!self.loopRange ()->is_looped ())
          {
            get_derived_object (new_object2_ref)
              ->loopRange ()
              ->setTrackBounds (true);

            /* if audio region, create a new region */
            if constexpr (std::is_same_v<BoundedObjectT, AudioRegion>)
              {
// TODO
#if 0
                auto prev_r2_ref = new_object2_ref;
                auto prev_r2_clip =
                  get_derived_object (prev_r2_ref)->get_clip ();
                assert (prev_r2_clip);
                assert (r2_local_end > 0);
                utils::audio::AudioBuffer tmp{
                  prev_r2_clip->get_num_channels (), (int) r2_local_end
                };
                for (int i = 0; i < prev_r2_clip->get_num_channels (); ++i)
                  {
                    tmp.copyFrom (
                      i, 0, prev_r2_clip->get_samples (), i, local_pos,
                      r2_local_end);
                  }
                assert (!get_derived_object (prev_r2_ref)->get_name ().empty ());
                assert (r2_local_end >= 0);
                new_object2_ref =
                  factory.create_audio_region_from_audio_buffer_FIXME (
                    get_derived_object (prev_r2_ref)->get_lane (), tmp,
                    prev_r2_clip->get_bit_depth (),
                    get_derived_object (prev_r2_ref)->get_name (), local_pos);
                assert (
                  get_derived_object (new_object2_ref)->get_clip_id ()
                  != get_derived_object (prev_r2_ref)->get_clip_id ());
#endif
              }
            else
              {
                /* move all objects backwards */
                const double ticks_to_subtract =
                  tempo_map.samples_to_tick (units::samples (local_pos))
                    .in (units::ticks);
                get_derived_object (new_object2_ref)
                  ->add_ticks_to_children (-ticks_to_subtract);

/* remove objects starting before the start */
// TODO
#if 0
                for (
                  auto * child :
                  get_derived_object (new_object2_ref)->get_children_view ())
                  {
                    if (child->position ().frames_ < 0)
                      get_derived_object (new_object2_ref)
                        ->remove_object (child->get_uuid ());
                  }
#endif
              }
          }
      }

      /* make sure regions have names */
// TODO
#if 0
    if constexpr (RegionObject<BoundedObjectT>)
      {
        auto track_var = self.get_track ();
        std::visit (
          [&] (auto &&track) {
            structure::tracks::AutomationTrack * at = nullptr;
            if constexpr (std::is_same_v<BoundedObjectT, AutomationRegion>)
              {
                at = self.get_automation_track ();
              }
            get_derived_object (new_object1_ref)
              ->generate_name (self.get_name (), at, track);
            get_derived_object (new_object2_ref)
              ->generate_name (self.get_name (), at, track);
          },
          track_var);
      }
#endif

    return std::make_pair (new_object1_ref, new_object2_ref);
  }

  /**
   * Copies identifier values from src to this object.
   *
   * Used by `UndoableAction`s where only partial copies that identify the
   * original objects are needed.
   *
   * Need to rethink this as it's not easily maintainable.
   */
// deprecated - use from/to json
#if 0
  static void copy_arranger_object_identifier (
    const VariantType &dest,
    const VariantType &src);
#endif
};

static_assert (std::ranges::random_access_range<ArrangerObjectSpan>);

}
