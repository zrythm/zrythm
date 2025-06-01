// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "structure/tracks/track_all.h"
#include "utils/uuid_identifiable_object.h"
#include "utils/views.h"

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

  static auto position_projection (const VariantType &track_var)
  {
    return std::visit (
      [] (const auto &track) { return track->get_index (); }, track_var);
  }
  static auto visible_projection (const VariantType &track_var)
  {
    return std::visit (
      [] (const auto &track) { return track->should_be_visible (); }, track_var);
  }
  static auto name_projection (const VariantType &track_var)
  {
    return std::visit (
      [] (const auto &track) { return track->get_name (); }, track_var);
  }
  static auto selected_projection (const VariantType &track_var)
  {
    return std::visit (
      [] (const auto &track) { return track->getSelected (); }, track_var);
  }
  static auto foldable_projection (const VariantType &track_var)
  {
    return Base::template derived_from_type_projection<FoldableTrack> (
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
            return track_ref.get_##func_name (); \
          } \
        else \
          { \
            return false; \
          } \
      }, \
      track_var); \
  }
  DEFINE_PROJECTION_FOR_TRACK_TYPE (listened, ChannelTrack)
  DEFINE_PROJECTION_FOR_TRACK_TYPE (muted, ChannelTrack)
  DEFINE_PROJECTION_FOR_TRACK_TYPE (soloed, ChannelTrack)
  DEFINE_PROJECTION_FOR_TRACK_TYPE (enabled, ChannelTrack)
  DEFINE_PROJECTION_FOR_TRACK_TYPE (disabled, ChannelTrack)

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

  std::optional<VariantType> get_track_by_pos (int pos) const
  {
    auto it = std::ranges::find (*this, pos, position_projection);
    return it != this->end () ? std::make_optional (*it) : std::nullopt;
  }

  std::optional<VariantType>
  get_track_by_name (const utils::Utf8String &name) const
  {
    auto it = std::ranges::find (*this, name, name_projection);
    return it != this->end () ? std::make_optional (*it) : std::nullopt;
  }

  /**
   * @brief Returns the highest track (with smallest position).
   */
  VariantType get_first_track () const
  {
    return std::ranges::min (*this, {}, position_projection);
  }

  /**
   * @brief Returns the lowest track (with largest position).
   */
  VariantType get_last_track () const
  {
    return std::ranges::max (*this, {}, position_projection);
  }

  bool contains_track_name (const utils::Utf8String &name) const
  {
    return std::ranges::contains (*this, name, name_projection);
  }

  auto get_visible_tracks () const
  {
    return std::views::filter (*this, visible_projection);
  }

  auto get_selected_tracks () const
  {
    return std::views::filter (*this, selected_projection);
  }

  /**
   * @brief Prints the track collection contents (for debugging).
   */
  void print_tracks () const
  {
    z_info ("----- tracklist tracks ------");
    for (const auto [index, track_var] : utils::views::enumerate (*this))
      {
        std::visit (
          [&] (auto &&track_ref) {
            const auto                  &track = *track_ref;
            std::string                  parent_str;
            std::vector<FoldableTrack *> parents;
            track.add_folder_parents (parents, false);
            parent_str.append (parents.size () * 2, '-');
            if (!parents.empty ())
              parent_str += ' ';

            int fold_size = 1;
            if constexpr (
              std::is_same_v<base_type<decltype (track)>, FoldableTrack>)
              {
                fold_size = track->size_;
              }

            z_info (
              "[{:03}] {}{} (pos {}, parents {}, size {})", index, parent_str,
              track.get_name (), track.get_index (), parents.size (), fold_size);
          },
          track_var);
      }
    z_info ("------ end ------");
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
   * Marks the tracks to be bounced.
   *
   * @param with_parents Also mark all the track's parents recursively.
   * @param mark_master Also mark the master track.
   *   Set to true when exporting the mixdown, false otherwise.
   */
  void
  mark_for_bounce (Tracklist &tracklist, bool with_parents, bool mark_master);

  /**
   * Marks or unmarks all tracks for bounce.
   */
  void mark_all_tracks_for_bounce (Tracklist &tracklist, bool bounce);

  /**
   * Toggle visibility of the tracks in this collection.
   */
  void toggle_visibility ()
  {
    std::ranges::for_each (*this, [&] (auto &&track_var) {
      std::visit (
        [&] (auto &&tr) { tr->setVisible (!tr->getVisible ()); }, track_var);
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
    return !std::ranges::all_of (
      *this, Base::template derived_from_type_projection<AutomatableTrack>);
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
   * @brief Paste to the given position in the tracklist (performs a copy).
   *
   * @param pos
   * @throw ZrythmException if failed to paste.
   */
  void paste_to_pos (int pos);

  /**
   * Fills in the given array with all plugins in the tracklist.
   */
  template <typename T> void get_plugins (T &container)
  {
    std::ranges::for_each (*this, [&container] (const auto &track_var) {
      std::visit (
        [&container] (auto &&track) { track->get_plugins (container); },
        track_var);
    });
  }

  void deselect_all_plugins ()
  {
    std::vector<Plugin *> plugins;
    get_plugins (plugins);
    for (const auto &pl : plugins)
      {
        pl->set_selected (false);
      }
  }

  auto has_soloed () const
  {
    return std::ranges::any_of (*this, soloed_projection);
  }

  auto has_listened () const
  {
    return std::ranges::any_of (*this, listened_projection);
  }

  auto get_num_muted_tracks () const
  {
    return std::ranges::count_if (*this, muted_projection);
  }

  auto get_num_soloed_tracks () const
  {
    return std::ranges::count_if (*this, soloed_projection);
  }

  auto get_num_listened_tracks () const
  {
    return std::ranges::count_if (*this, listened_projection);
  }

  /**
   * @brief Get the total (max) bars in the track list.
   *
   * @param total_bars Current known total bars
   * @return New total bars if > than @p total_bars, or @p total_bars.
   */
  int
  get_total_bars (const engine::session::Transport &transport, int total_bars)
    const
  {
    std::ranges::for_each (*this, [&] (const auto &track_var) {
      std::visit (
        [&] (auto &&track) {
          total_bars = track->get_total_bars (transport, total_bars);
        },
        track_var);
    });
    return total_bars;
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
   * Exposes each track's ports that should be exposed to the backend.
   *
   * This should be called after setting up the engine.
   */
  void expose_ports_to_backend (engine::device_io::AudioEngine &engine);

  /**
   * @brief @see Channel.reconnect_ext_input_ports().
   */
  void reconnect_ext_input_ports (engine::device_io::AudioEngine &engine);

  /**
   * @brief Fixes audio regions and returns whether positions were adjusted.
   */
  bool fix_audio_regions (dsp::FramesPerTick frames_per_tick);

  /**
   * Activate or deactivate all plugins.
   *
   * This is useful for exporting: deactivating and reactivating a plugin will
   * reset its state.
   */
  void activate_all_plugins (bool activate)
  {
    std::ranges::for_each (*this, [&] (const auto &track_var) {
      std::visit (
        [&] (auto &&track) { track->activate_all_plugins (activate); },
        track_var);
    });
  }

  void init_loaded (
    gui::old_dsp::plugins::PluginRegistry &plugin_registry,
    PortRegistry                          &port_registry)
  {
    std::ranges::for_each (*this, [&] (const auto &track_var) {
      std::visit (
        [&] (auto &&track) {
          track->init_loaded (plugin_registry, port_registry);
        },
        track_var);
    });
  }

  void
  move_after_copying_or_moving_inside (int diff_between_track_below_and_parent);

  /**
   * @brief Creates a snapshot of the track collection.
   *
   * Intended to be used in undoable actions.
   *
   * @return A vector of track variants.
   */
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
            return track->clone_qobject (
              &owner, ObjectCloneType::Snapshot, track_registry,
              plugin_registry, port_registry, obj_registry, false);
          },
          track_var);
      }));
  }

  std::vector<TrackUuidReference> create_new_identities (
    TrackRegistry          &track_registry,
    PluginRegistry         &plugin_registry,
    PortRegistry           &port_registry,
    ArrangerObjectRegistry &obj_registry) const
  {
    return std::ranges::to<std::vector> (
      *this | std::views::transform ([&] (const auto &track_var) {
        return std::visit (
          [&] (auto &&track) -> TrackUuidReference {
            return track_registry.clone_object (
              *track, track_registry, plugin_registry, port_registry,
              obj_registry, true);
          },
          track_var);
      }));
  }
};

static_assert (std::ranges::random_access_range<TrackSpan>);

namespace TrackCollections
{

template <std::ranges::range TrackRange>
void
sort_by_position (TrackRange &tracks)
{
  std::ranges::sort (tracks, {}, TrackSpan::position_projection);
}
}; // namespace TrackCollections
}
