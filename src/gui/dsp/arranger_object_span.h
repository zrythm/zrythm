#pragma once

#include "gui/dsp/arranger_object_all.h"
#include "utils/debug.h"
#include "utils/uuid_identifiable_object.h"

/**
 * @brief Track span that offers helper methods on a range of tracks.
 */
template <utils::UuidIdentifiableObjectPtrVariantRange Range>
class ArrangerObjectSpanImpl
    : public utils::
        UuidIdentifiableObjectCompatibleSpan<Range, ArrangerObjectRegistry>
{
public:
  using Base =
    utils::UuidIdentifiableObjectCompatibleSpan<Range, ArrangerObjectRegistry>;
  using VariantType = typename Base::value_type;
  using ArrangerObjectUuid = typename Base::UuidType;
  using Base::Base; // Inherit constructors

  using Position = dsp::Position;

  static auto name_projection (const VariantType &obj_var)
  {
    return std::visit (
      [] (const auto &obj) -> std::string {
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
      [&] (auto &&obj) -> Position { return *obj->pos_; }, obj_var);
  }
  static Position end_position_with_start_position_fallback_projection (
    const VariantType &obj_var)
  {
    return std::visit (
      [&] (auto &&obj) -> Position {
        using ObjT = base_type<decltype (obj)>;
        if constexpr (std::derived_from<ObjT, BoundedObject>)
          {
            return *obj->end_pos_;
          }
        else
          {
            return *obj->pos_;
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
      [&] (auto &&obj) { return obj->is_selected (); }, obj_var);
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
  void init_loaded (bool project, double frames_per_tick);

  std::vector<VariantType> create_snapshots (QObject &owner) const
  {
    return std::ranges::to<std::vector> (
      *this | std::views::transform ([&] (const auto &obj_var) {
        return std::visit (
          [&] (auto &&obj) -> VariantType {
            return obj->clone_qobject (&owner, ObjectCloneType::Snapshot);
          },
          obj_var);
      }));
  }

  std::vector<VariantType>
  create_new_identities (ArrangerObjectRegistry &object_registry) const
  {
    return std::ranges::to<std::vector> (
      *this | std::views::transform ([&] (const auto &obj_var) {
        return std::visit (
          [&] (auto &&obj) -> VariantType {
            return obj->clone_and_register (object_registry);
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
   * Adds a clone of each object in the selection to the given region (if
   * applicable).
   */
  void add_to_region (Region &region);

  /**
   * Moves the selections by the given amount of ticks.
   *
   * @param ticks Ticks to add.
   */
  void add_ticks (double ticks);

  /**
   * Selects all objects.
   */
  void select_all ()
  {
    for (const auto &obj_var : *this)
      {
        std::visit ([&] (auto &&obj) { obj->setSelected (true); }, obj_var);
      }
  }

  void select_single (const ArrangerObjectPtrVariant &obj_var)
  {
    deselect_all ();
    std::visit ([&] (auto &&obj) { obj->setSelected (true); }, obj_var);
  }

  void deselect_all ()
  {
    for (const auto &obj_var : *this)
      {
        std::visit ([&] (auto &&obj) { obj->setSelected (false); }, obj_var);
      }
  }

  /**
   * Code to run after deserializing.
   */
  void post_deserialize ();

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
  auto merge (double frames_per_tick) const -> ArrangerObjectUuid;

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
        return region->pool_id_ == clip.get_pool_id ();
      });
  };

  bool can_split_at_pos (Position pos) const;

  /**
   * Sets the listen status of notes on and off based on changes in the previous
   * selections and the current selections.
   */
  void unlisten_note_diff (const ArrangerObjectSpanImpl &prev) const
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
    const BoundedObjectT   &self,
    ArrangerObjectRegistry &registry,
    const Position         &global_pos,
    double frames_per_tick) -> std::pair<BoundedObjectT *, BoundedObjectT *>
  {
    const double ticks_per_frame = 1.0 / frames_per_tick;

    /* create the new objects as new identities */
    auto * new_object1 = self.clone_and_register (registry);
    auto * new_object2 = self.clone_and_register (registry);

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
    new_object1->end_pos_setter (&global_pos);
    new_object1->set_position (
      &local_pos, ArrangerObject::PositionType::FadeOut, false);

    if constexpr (std::derived_from<BoundedObjectT, LoopableObject>)
      {
        /* if original object was not looped, make the new object unlooped
         * also */
        if (!self.is_looped ())
          {
            new_object1->loop_end_pos_setter (&local_pos);

            if constexpr (std::is_same_v<BoundedObjectT, AudioRegion>)
              {
                /* create new audio region */
                auto prev_r1 = new_object1;
                auto prev_r1_clip = prev_r1->get_clip ();
                z_return_val_if_fail (
                  prev_r1_clip, std::make_pair (nullptr, nullptr));
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
                z_return_val_if_fail (
                  !prev_r1->name_.empty (), std::make_pair (nullptr, nullptr));
                z_return_val_if_fail_cmp (
                  local_pos.frames_, >=, 0, std::make_pair (nullptr, nullptr));
                registry.unregister_object (new_object1->get_uuid ());
                new_object1 = registry.create_object<AudioRegion> (
                  frames, true, prev_r1->name_, prev_r1_clip->get_bit_depth (),
                  *prev_r1->pos_, prev_r1->id_.track_uuid_,
                  prev_r1->id_.lane_pos_, prev_r1->id_.idx_);
                z_return_val_if_fail (
                  new_object1->pool_id_ != prev_r1->pool_id_,
                  std::make_pair (nullptr, nullptr));
                prev_r1->deleteLater ();
              }
            else if constexpr (RegionSubclass<BoundedObjectT>)
              {
                /* remove objects starting after the end */
                std::vector<RegionOwnedObjectImpl<BoundedObjectT> *> children;
                new_object1->append_children (children);
                for (auto child : children)
                  {
                    if (*child->pos_ > local_pos)
                      {
                        auto casted_child = dynamic_cast<
                          RegionChildType_t<BoundedObjectT> *> (child);
                        new_object1->remove_object (*casted_child, false);
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
        new_object2->clip_start_pos_setter (&local_pos);
      }
    new_object2->pos_setter (&global_pos);
    Position r2_local_end = *new_object2->end_pos_;
    r2_local_end.add_ticks (-new_object2->pos_->ticks_, frames_per_tick);
    if constexpr (std::derived_from<BoundedObjectT, FadeableObject>)
      {
        new_object2->set_position (
          &r2_local_end, ArrangerObject::PositionType::FadeOut, false);
      }

    /* if original object was not looped, make the new object unlooped also */
    if constexpr (std::derived_from<BoundedObjectT, LoopableObject>)
      {
        if (!self.is_looped ())
          {
            Position init_pos;
            new_object2->clip_start_pos_setter (&init_pos);
            new_object2->loop_start_pos_setter (&init_pos);
            new_object2->loop_end_pos_setter (&r2_local_end);

            if constexpr (RegionSubclass<BoundedObjectT>)
              {
                /* move all objects backwards */
                new_object2->add_ticks_to_children (-local_pos.ticks_);

                /* if audio region, create a new region */
                if constexpr (std::is_same_v<BoundedObjectT, AudioRegion>)
                  {
                    auto prev_r2 = new_object2;
                    auto prev_r2_clip = prev_r2->get_clip ();
                    z_return_val_if_fail (
                      prev_r2_clip, std::make_pair (nullptr, nullptr));
                    size_t num_frames =
                      (size_t) r2_local_end.frames_
                      * prev_r2_clip->get_num_channels ();
                    z_return_val_if_fail_cmp (
                      num_frames, >, 0, std::make_pair (nullptr, nullptr));
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
                    z_return_val_if_fail (
                      !prev_r2->name_.empty (),
                      std::make_pair (nullptr, nullptr));
                    z_return_val_if_fail_cmp (
                      r2_local_end.frames_, >=, 0,
                      std::make_pair (nullptr, nullptr));
                    registry.unregister_object (prev_r2->get_uuid ());
                    new_object2 = registry.create_object<AudioRegion> (
                      tmp, true, prev_r2->get_name (),
                      prev_r2_clip->get_bit_depth (), local_pos,
                      prev_r2->id_.track_uuid_, prev_r2->id_.lane_pos_,
                      prev_r2->id_.idx_);
                    z_return_val_if_fail (
                      new_object2->pool_id_ != prev_r2->pool_id_,
                      std::make_pair (nullptr, nullptr));
                    prev_r2->deleteLater ();
                  }
                else if constexpr (RegionSubclass<BoundedObjectT>)
                  {
                    using ChildType = RegionChildType_t<BoundedObjectT>;
                    /* remove objects starting before the start */
                    std::vector<RegionOwnedObjectImpl<BoundedObjectT> *> children;
                    new_object2->append_children (children);
                    for (auto child : children)
                      {
                        if (child->pos_->frames_ < 0)
                          new_object2->remove_object (
                            *static_cast<ChildType *> (child), false);
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
            new_object1->gen_name (self.name_.c_str (), at, track);
            new_object2->gen_name (self.name_.c_str (), at, track);
          },
          track_var);
      }

    return std::make_pair (new_object1, new_object2);
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

using ArrangerObjectSpan =
  ArrangerObjectSpanImpl<std::span<const ArrangerObjectPtrVariant>>;
using ArrangerObjectRegistrySpan = ArrangerObjectSpanImpl<
  utils::UuidIdentifiableObjectSpan<ArrangerObjectRegistry>>;
extern template class ArrangerObjectSpanImpl<
  std::span<const ArrangerObjectSpan::VariantType>>;
extern template class ArrangerObjectSpanImpl<
  utils::UuidIdentifiableObjectSpan<ArrangerObjectRegistry>>;

static_assert (std::ranges::random_access_range<ArrangerObjectSpan>);
static_assert (std::ranges::random_access_range<ArrangerObjectRegistrySpan>);

using ArrangerObjectSpanVariant =
  std::variant<ArrangerObjectSpan, ArrangerObjectRegistrySpan>;
