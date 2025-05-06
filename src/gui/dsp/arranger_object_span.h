// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "gui/dsp/arranger_object_all.h"
#include "utils/debug.h"
#include "utils/uuid_identifiable_object.h"

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

  using Position = dsp::Position;

  static auto name_projection (const VariantType &obj_var)
  {
    return std::visit (
      [] (const auto &obj) -> utils::Utf8String {
        using ObjT = base_type<decltype (obj)>;
        if constexpr (std::derived_from<ObjT, NamedObject>)
          {
            return obj->get_name ();
          }
        else
          {
            throw std::runtime_error (
              "Name projection called on non-named object");
          }
      },
      obj_var);
  }
  static Position position_projection (const VariantType &obj_var)
  {
    return std::visit (
      [&] (auto &&obj) -> Position { return obj->get_position (); }, obj_var);
  }
  static Position end_position_with_start_position_fallback_projection (
    const VariantType &obj_var)
  {
    return std::visit (
      [&] (auto &&obj) -> Position {
        using ObjT = base_type<decltype (obj)>;
        if constexpr (std::derived_from<ObjT, BoundedObject>)
          {
            return obj->get_end_position ();
          }
        else
          {
            return obj->get_position ();
          }
      },
      obj_var);
  }
  static auto midi_note_pitch_projection (const VariantType &obj_var)
  {
    return std::get<MidiNote *> (obj_var)->pitch_;
  }
  static auto looped_projection (const VariantType &obj_var)
  {
    return std::visit (
      [] (const auto &obj) {
        using ObjT = base_type<decltype (obj)>;
        if constexpr (std::derived_from<ObjT, LoopableObject>)
          {
            return obj->is_looped ();
          }
        else
          return false;
      },
      obj_var);
  }
  static auto is_timeline_object_projection (const VariantType &obj_var)
  {
    return Base::template derived_from_type_projection<TimelineObject> (obj_var);
  }
  static auto is_editor_object_projection (const VariantType &obj_var)
  {
    return !is_timeline_object_projection (obj_var);
  }
  static auto track_projection (const VariantType &obj_var)
  {
    return std::visit ([&] (auto &&obj) { return obj->get_track (); }, obj_var);
  }
  static auto selected_projection (const VariantType &obj_var)
  {
    return std::visit (
      [&] (auto &&obj) { return obj->getSelected (); }, obj_var);
  }
  static auto deletable_projection (const VariantType &obj_var)
  {
    return std::visit (
      [&] (auto &&obj) { return obj->is_deletable (); }, obj_var);
  }
  static auto cloneable_projection (const VariantType &obj_var)
  {
    return deletable_projection (obj_var);
  }
  static auto renameable_projection (const VariantType &obj_var)
  {
    return Base::template derived_from_type_projection<NamedObject> (obj_var)
           && deletable_projection (obj_var);
  }

  /**
   * Inits the selections after loading a project.
   *
   * @param project Whether these are project selections (as opposed to
   * clones).
   * @param frames_per_tick Frames per tick to use in position conversions.
   */
  void init_loaded (bool project, dsp::FramesPerTick frames_per_tick);

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
   *
   * @param global For non-timeline selections, whether to return the global
   * position (local + region start).
   */
  auto get_first_object_and_pos (bool global) const
    -> std::pair<VariantType, Position>;

  /**
   * Gets last object of the given type (if any, otherwise matches all types)
   * and its end (if applicable, otherwise start) position.
   *
   * @param ends_last Whether to get the object that ends last, otherwise the
   * object that starts last.
   * @param global For non-timeline selections, whether to return the global
   * position (local + region start).
   */
  auto get_last_object_and_pos (bool global, bool ends_last) const
    -> std::pair<VariantType, Position>;

  /**
   * Gets the highest track in the selections.
   */
  std::pair<TrackPtrVariant, TrackPtrVariant> get_first_and_last_track () const;

  std::pair<MidiNote *, MidiNote *> get_first_and_last_note () const
  {
    auto midi_notes =
      *this
      | std::views::transform (Base::template type_transformation<MidiNote>);
    auto [min_it, max_it] =
      std::ranges::minmax_element (midi_notes, {}, &MidiNote::pitch_);
    return { *min_it, *max_it };
  }

  /**
   * Pastes the given selections to the given Position.
   */
  void paste_to_pos (const Position &pos, bool undoable);

  /**
   * Code to run after deserializing a span of objects.
   */
  void post_deserialize (dsp::FramesPerTick frames_per_tick)
  {
    const auto post_deserialize_obj = [frames_per_tick] (auto * obj) {
      /* TODO: this acts as if a BPM change happened (and is only effective if
       * so), so if no BPM change happened this is unnecessary, so this should
       * be refactored in the future. this was added to fix copy-pasting audio
       * regions after changing the BPM (see #4993) */
      obj->update_positions (true, true, frames_per_tick);
    };
    for (const auto &var : *this)
      {
        std::visit (
          [&] (auto &&obj) {
            using ObjT = base_type<decltype (obj)>;

            post_deserialize_obj (obj);
            if constexpr (DerivedFromCRTPBase<ObjT, ArrangerObjectOwner>)
              {
                std::ranges::for_each (
                  obj->get_children_view (), post_deserialize_obj);
              }
          },
          var);
      }
  }

  /**
   * @brief Used for debugging.
   */
  bool validate () const;

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
  auto merge (dsp::FramesPerTick frames_per_tick) const
    -> ArrangerObjectUuidReference;

  /**
   * Returns if the selections can be pasted at the current place/playhead.
   */
  bool can_be_pasted () const;

  bool all_on_same_lane () const;

  /**
   * Returns if the selections can be pasted at the given position/region.
   *
   * @param pos Position to paste to.
   * @param idx Track index to start pasting to, if applicable.
   */
  bool can_be_pasted_at (Position pos, int idx = -1) const;

  bool contains_looped () const
  {
    return std::ranges::any_of (*this, looped_projection);
  };

  bool can_be_merged () const;

  double get_length_in_ticks () const
  {
    auto [_1, p1] = get_first_object_and_pos (false);
    auto [_2, p2] = get_last_object_and_pos (false, true);
    return p2.ticks_ - p1.ticks_;
  }

  /** Whether the selections contain the given clip.*/
  bool contains_clip (const AudioClip &clip) const
  {
    return std::ranges::any_of (
      *this | std::views::filter (Base::template type_projection<AudioRegion>)
        | std::views::transform (Base::template type_transformation<AudioRegion>),
      [&clip] (auto &&region) {
        return region->get_clip_id () == clip.get_uuid ();
      });
  };

  bool can_split_at_pos (Position pos) const;

  /**
   * Returns the region at the given position, or NULL.
   *
   * @param include_region_end Whether to include the region's end in the
   * calculation.
   */
  std::optional<VariantType>
  get_bounded_object_at_pos (dsp::Position pos, bool include_region_end = false)
    const
  {
    auto view =
      *this
      | std::views::filter (
        Base::template derived_from_type_projection<BoundedObject>);
    auto it = std::ranges::find_if (view, [&] (const auto &r_var) {
      auto r =
        Base::template derived_from_type_transformation<BoundedObject> (r_var);
      return r->get_position () <= pos
             && r->end_pos_->frames_ + (include_region_end ? 1 : 0) > pos.frames_;
    });
    return it != view.end () ? std::make_optional (*it) : std::nullopt;
  }

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
  template <FinalBoundedObjectSubclass BoundedObjectT>
  static auto split_bounded_object (
    const BoundedObjectT &self,
    const auto           &factory,
    const Position       &global_pos,
    dsp::FramesPerTick    frames_per_tick)
    -> std::pair<ArrangerObjectUuidReference, ArrangerObjectUuidReference>
  {
    const auto ticks_per_frame = dsp::to_ticks_per_frame (frames_per_tick);

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
      Position local = global_pos;
      if constexpr (RegionSubclass<BoundedObjectT>)
        {
          auto local_frames =
            self.timeline_frames_to_local (global_pos.frames_, true);
          local.from_frames (local_frames, ticks_per_frame);
        }
      return local;
    }();

    /*
     * for first object set:
     * - end pos
     * - fade out pos
     */
    get_derived_object (new_object1_ref)
      ->end_position_setter_validated (global_pos, ticks_per_frame);
    get_derived_object (new_object1_ref)
      ->set_position (
        local_pos, ArrangerObject::PositionType::FadeOut, false, ticks_per_frame);

    if constexpr (std::derived_from<BoundedObjectT, LoopableObject>)
      {
        /* if original object was not looped, make the new object unlooped
         * also */
        if (!self.is_looped ())
          {
            get_derived_object (new_object1_ref)
              ->loop_end_position_setter_validated (local_pos, ticks_per_frame);

            if constexpr (std::is_same_v<BoundedObjectT, AudioRegion>)
              {
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
              }
            else if constexpr (RegionWithChildren<BoundedObjectT>)
              {
                /* remove objects starting after the end */
                auto children =
                  get_derived_object (new_object1_ref)->get_children_vector ();
                for (const auto &child_id : children)
                  {
                    const auto &child = child_id.get_object ();
                    if (position_projection (child) > local_pos)
                      {
                        get_derived_object (new_object1_ref)
                          ->remove_object (child_id.id ());
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
    if constexpr (std::derived_from<BoundedObjectT, LoopableObject>)
      {
        get_derived_object (new_object2_ref)
          ->clip_start_position_setter_validated (local_pos, ticks_per_frame);
      }
    get_derived_object (new_object2_ref)
      ->position_setter_validated (global_pos, ticks_per_frame);
    Position r2_local_end = *get_derived_object (new_object2_ref)->end_pos_;
    r2_local_end.add_ticks (
      -get_derived_object (new_object2_ref)->get_position ().ticks_,
      frames_per_tick);
    if constexpr (std::derived_from<BoundedObjectT, FadeableObject>)
      {
        get_derived_object (new_object2_ref)
          ->set_position (
            r2_local_end, ArrangerObject::PositionType::FadeOut, false,
            ticks_per_frame);
      }

    /* if original object was not looped, make the new object unlooped also */
    if constexpr (std::derived_from<BoundedObjectT, LoopableObject>)
      {
        if (!self.is_looped ())
          {
            Position init_pos;
            get_derived_object (new_object2_ref)
              ->clip_start_position_setter_validated (init_pos, ticks_per_frame);
            get_derived_object (new_object2_ref)
              ->loop_start_position_setter_validated (init_pos, ticks_per_frame);
            get_derived_object (new_object2_ref)
              ->loop_end_position_setter_validated (
                r2_local_end, ticks_per_frame);

            if constexpr (RegionSubclass<BoundedObjectT>)
              {
                if constexpr (RegionWithChildren<BoundedObjectT>)
                  {
                    /* move all objects backwards */
                    get_derived_object (new_object2_ref)
                      ->add_ticks_to_children (
                        -local_pos.ticks_, frames_per_tick);
                  }

                /* if audio region, create a new region */
                if constexpr (std::is_same_v<BoundedObjectT, AudioRegion>)
                  {
                    auto prev_r2_ref = new_object2_ref;
                    auto prev_r2_clip =
                      get_derived_object (prev_r2_ref)->get_clip ();
                    assert (prev_r2_clip);
                    assert ((size_t) r2_local_end.frames_ > 0);
                    utils::audio::AudioBuffer tmp{
                      prev_r2_clip->get_num_channels (),
                      (int) r2_local_end.frames_
                    };
                    for (int i = 0; i < prev_r2_clip->get_num_channels (); ++i)
                      {
                        tmp.copyFrom (
                          i, 0, prev_r2_clip->get_samples (), i,
                          local_pos.frames_, r2_local_end.frames_);
                      }
                    assert (
                      !get_derived_object (prev_r2_ref)->get_name ().empty ());
                    assert (r2_local_end.frames_ >= 0);
                    new_object2_ref =
                      factory.create_audio_region_from_audio_buffer_FIXME (
                        get_derived_object (prev_r2_ref)->get_lane (), tmp,
                        prev_r2_clip->get_bit_depth (),
                        get_derived_object (prev_r2_ref)->get_name (),
                        local_pos.ticks_);
                    assert (
                      get_derived_object (new_object2_ref)->get_clip_id ()
                      != get_derived_object (prev_r2_ref)->get_clip_id ());
                  }
                else if constexpr (RegionSubclass<BoundedObjectT>)
                  {
                    // using ChildType = RegionChildType_t<BoundedObjectT>;
                    /* remove objects starting before the start */
                    std::vector<RegionOwnedObject *> children;
                    get_derived_object (new_object2_ref)
                      ->append_children (children);
                    for (auto * child : children)
                      {
                        if (child->get_position ().frames_ < 0)
                          get_derived_object (new_object2_ref)
                            ->remove_object (child->get_uuid ());
                      }
                  }
              }
          }
      }

    /* make sure regions have names */
    if constexpr (RegionSubclass<BoundedObjectT>)
      {
        auto track_var = self.get_track ();
        std::visit (
          [&] (auto &&track) {
            AutomationTrack * at = nullptr;
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
  static void copy_arranger_object_identifier (
    const VariantType &dest,
    const VariantType &src);
};

static_assert (std::ranges::random_access_range<ArrangerObjectSpan>);
