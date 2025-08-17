// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "structure/tracks/track_all.h"

namespace zrythm::engine::device_io
{
class AudioEngine;
}

namespace zrythm::structure::tracks
{
/**
 * @brief Track span that offers helper methods on a range of tracks.
 */
class TrackSpan : public utils::UuidIdentifiableObjectView<TrackRegistry>
{
public:
  using Base = utils::UuidIdentifiableObjectView<TrackRegistry>;
  using VariantType = typename Base::VariantType;
  using TrackUuid = typename Base::UuidType;
  using Base::Base; // Inherit constructors

  using ArrangerObjectRegistry = arrangement::ArrangerObjectRegistry;

  bool contains_track (const TrackUuid &id) const
  {
    return std::ranges::contains (*this, id, Base::uuid_projection);
  }

  static auto name_projection (const VariantType &track_var)
  {
    return std::visit (
      [] (const auto &track) { return track->get_name (); }, track_var);
  }
  static auto selected_projection (const VariantType &track_var)
  {
    return std::visit (
      [] (const auto &track) { return track->selected (); }, track_var);
  }
  static auto foldable_projection (const VariantType &track_var)
  {
    return std::visit (
      [] (const auto &track) {
        using TrackT = base_type<decltype (track)>;
        return FoldableTrack<TrackT>;
      },
      track_var);
  }

#define DEFINE_PROJECTION_FOR_TRACK_TYPE(func_name, base_class) \
  static auto func_name##_projection (const VariantType &track_var) \
  { \
    return std::visit ( \
      [] (const auto &track) { \
        auto &track_ref = *track; \
        using TrackT = base_type<decltype (track_ref)>; \
        if constexpr (std::derived_from<TrackT, base_class>) \
          { \
            return track_ref.func_name (); \
          } \
        else \
          { \
            return false; \
          } \
      }, \
      track_var); \
  }
  DEFINE_PROJECTION_FOR_TRACK_TYPE (enabled, Track)

  template <typename BaseType>
  static auto derived_type_transformation (const VariantType &track_var)
  {
    return std::visit (
      [] (const auto &track) {
        using TrackT = base_type<decltype (track)>;
        if constexpr (!std::derived_from<TrackT, BaseType>)
          {
            throw std::runtime_error (
              "Must filter before using this transformation.");
          }
        return dynamic_cast<BaseType *> (track);
      },
      track_var);
  }

  std::optional<VariantType>
  get_track_by_name (const utils::Utf8String &name) const
  {
    auto it = std::ranges::find (*this, name, name_projection);
    return it != this->end () ? std::make_optional (*it) : std::nullopt;
  }

  bool contains_track_name (const utils::Utf8String &name) const
  {
    return std::ranges::contains (*this, name, name_projection);
  }

  auto get_selected_tracks () const
  {
    return std::views::filter (*this, selected_projection);
  }

  MasterTrack &get_master_track () const
  {
    auto it = std::ranges::find_if (*this, [] (const auto &track_var) {
      return std::holds_alternative<MasterTrack *> (track_var);
    });
    if (it == this->end ())
      {
        throw std::runtime_error ("Master track not found");
      }
    return *std::get<MasterTrack *> (*it);
  }

  /**
   * Toggle visibility of the tracks in this collection.
   */
  void toggle_visibility ()
  {
    std::ranges::for_each (*this, [&] (auto &&track_var) {
      std::visit (
        [&] (auto &&tr) { tr->setVisible (!tr->visible ()); }, track_var);
    });
  }

#if 0
  /**
   * Deselects all tracks except for the last visible track.
   */
  void select_last_visible ()
  {
    auto res = std::ranges::find_last_if (*this, visible_projection);
    if (!res.empty ())
      {
        std::visit (
          [&] (auto &&tr) { select_single (tr->get_uuid ()); }, res.front ());
      }
  }

  void select_all_visible_tracks ()
  {
    std::ranges::for_each (
      *this | std::views::filter (visible_projection), [] (auto &&track_var) {
        std::visit ([] (auto &&tr) { tr->setSelected (true); }, track_var);
      });
  }
#endif

  /**
   * Returns whether the tracklist selections contains a track that cannot have
   * automation lanes.
   */
  bool contains_non_automatable_track () const
  {
    return std::ranges::any_of (*this, [&] (auto &&track_var) {
      return std::visit (
        [] (auto &&tr) { return !tr->has_automation (); }, track_var);
    });
  }

  bool contains_undeletable_track () const
  {
    return std::ranges::any_of (*this, [&] (auto &&track_var) {
      return std::visit (
        [] (auto &&tr) { return !tr->is_deletable (); }, track_var);
    });
  }

  bool contains_uncopyable_track () const
  {
    return std::ranges::any_of (*this, [&] (auto &&track_var) {
      return std::visit (
        [] (auto &&tr) { return !tr->is_copyable (); }, track_var);
    });
  }

  bool contains_uninstantiated_plugin () const
  {
    return std::ranges::any_of (*this, [] (const auto &track_var) {
      return std::visit (
        [&] (auto &&tr) { return tr->contains_uninstantiated_plugin (); },
        track_var);
    });
  }

  /**
   * Fills in the given array with all plugins in the tracklist.
   */
  template <typename T> void get_plugins (T &container)
  {
    std::ranges::for_each (*this, [&container] (const auto &track_var) {
      std::visit (
        [&container] (auto &&track) { track->collect_plugins (container); },
        track_var);
    });
  }

  void deselect_all_plugins ()
  {
    // TODO?
  }

  static bool currently_soloed_projection (const VariantType &var)
  {
    return std::visit (
      [] (auto &&track) {
        if (!track->channel ())
          return false;
        return track->channel ()->fader ()->currently_soloed ();
      },
      var);
  }
  static bool currently_muted_projection (const VariantType &var)
  {
    return std::visit (
      [] (auto &&track) {
        if (!track->channel ())
          return false;
        return track->channel ()->fader ()->currently_muted ();
      },
      var);
  }
  static bool currently_listened_projection (const VariantType &var)
  {
    return std::visit (
      [] (auto &&track) {
        if (!track->channel ())
          return false;
        return track->channel ()->fader ()->currently_listened ();
      },
      var);
  }

  auto has_soloed () const
  {
    return std::ranges::any_of (*this, currently_soloed_projection);
  }

  auto has_listened () const
  {
    return std::ranges::any_of (*this, currently_listened_projection);
  }

  auto get_num_muted_tracks () const
  {
    return std::ranges::count_if (*this, currently_muted_projection);
  }

  auto get_num_soloed_tracks () const
  {
    return std::ranges::count_if (*this, currently_soloed_projection);
  }

  auto get_num_listened_tracks () const
  {
    return std::ranges::count_if (*this, currently_listened_projection);
  }

  /**
   * Set various caches (snapshots, track name hashes, plugin
   * input/output ports, etc).
   */
  void set_caches (CacheType types)
  {
    std::ranges::for_each (*this, [&] (const auto &track_var) {
      std::visit ([&] (auto &&track) { track->set_caches (types); }, track_var);
    });
  }

  /**
   * @brief Creates a snapshot of the track collection.
   *
   * Intended to be used in undoable actions.
   *
   * @return A vector of track variants.
   */
// depreacted: use from/to json
#if 0
  std::vector<VariantType> create_snapshots (
    QObject                &owner,
    TrackRegistry          &track_registry,
    PluginRegistry         &plugin_registry,
    PortRegistry           &port_registry,
    ArrangerObjectRegistry &obj_registry) const
  {
    return std::ranges::to<std::vector> (
      *this | std::views::transform ([&] (const auto &track_var) {
        return std::visit (
          [&] (auto &&track) -> VariantType {
            return utils::clone_qobject (
              *track, &owner, utils::ObjectCloneType::Snapshot, track_registry,
              plugin_registry, port_registry, obj_registry, false);
          },
          track_var);
      }));
  }

  std::vector<TrackUuidReference>
  create_new_identities (FinalTrackDependencies track_deps) const
  {
    return std::ranges::to<std::vector> (
      *this | std::views::transform ([&] (const auto &track_var) {
        return std::visit (
          [&] (auto &&track) -> TrackUuidReference {
            return track_deps.track_registry_.clone_object (*track, track_deps);
          },
          track_var);
      }));
  }
#endif
};

static_assert (std::ranges::random_access_range<TrackSpan>);
}
