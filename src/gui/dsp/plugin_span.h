// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "gui/dsp/plugin_all.h"
#include "utils/uuid_identifiable_object.h"

using namespace zrythm::gui::old_dsp::plugins;

/**
 * @brief Track span that offers helper methods on a range of tracks.
 */
template <utils::UuidIdentifiableObjectPtrVariantRange Range>
class PluginSpanImpl
    : public utils::UuidIdentifiableObjectCompatibleSpan<Range, PluginRegistry>
{
public:
  using Base =
    utils::UuidIdentifiableObjectCompatibleSpan<Range, PluginRegistry>;
  using VariantType = typename Base::value_type;
  using PluginUuid = typename Base::UuidType;
  using Base::Base; // Inherit constructors

  static auto name_projection (const VariantType &pl_var)
  {
    return std::visit ([] (const auto &pl) { return pl->get_name (); }, pl_var);
  }
  static auto visible_projection (const VariantType &pl_var)
  {
    return std::visit ([] (const auto &pl) { return pl->visible_; }, pl_var);
  }
  static auto slot_projection (const VariantType &pl_var)
  {
    return std::visit ([] (const auto &pl) { return *pl->get_slot (); }, pl_var);
  }
  static auto state_dir_projection (const VariantType &pl_var)
  {
    return std::visit ([] (auto &&pl) { return pl->state_dir_; }, pl_var);
  }
  static auto slot_type_projection (const VariantType &pl_var)
  {
    return std::visit (
      [] (const auto &pl) { return pl->get_slot_type (); }, pl_var);
  }
  static auto track_id_projection (const VariantType &pl_var)
  {
    return std::visit (
      [] (const auto &pl) { return pl->get_track_id (); }, pl_var);
  }
  static auto instantiation_failed_projection (const VariantType &pl_var)
  {
    return std::visit (
      [] (const auto &pl) { return pl->instantiation_failed_; }, pl_var);
  }

  void select_single (const PluginUuid &id)
  {
    std::ranges::for_each (*this, [&] (auto &&pl_var) {
      std::visit (
        [&] (auto &&pl) { pl->set_selected (pl->get_uuid () == id); }, pl_var);
    });
  }

  bool validate () const
  {
    return std::ranges::all_of (*this, [&] (const auto &pl_var) {
      return std::visit (
        [] (const auto &pl) { return pl->validate (); }, pl_var);
    });
  }

  bool contains_uninstantiated_due_to_failure () const
  {
    return !std::ranges::all_of (*this, instantiation_failed_projection);
  }

  /**
   * @brief Creates a snapshot of the plugin collection.
   *
   * Intended to be used in undoable actions.
   *
   * @return A vector of plugin variants.
   */
  std::vector<VariantType> create_snapshots (QObject &owner) const
  {
    return std::ranges::to<std::vector> (
      *this | std::views::transform ([&] (const auto &plugin_var) {
        return std::visit (
          [&] (auto &&pl) -> VariantType { return pl->clone_qobject (&owner); },
          plugin_var);
      }));
  }

  /**
   * Returns whether the plugins can be pasted to the given slot.
   */
  bool can_be_pasted (const dsp::PluginSlot &slot) const;

  /**
   * Paste the selections starting at the slot in the given channel.
   *
   * This calls gen_full_from_this() internally to generate FullMixerSelections
   * with cloned plugins (calling init_loaded() on each), which are then pasted.
   */
  void paste_to_slot (Plugin::Channel &ch, dsp::PluginSlot slot) const;

  void get_plugins (std::vector<Plugin *> &plugins) const
  {
    auto base_plugins = Base::as_base_type ();
    plugins.insert (plugins.end (), base_plugins.begin (), base_plugins.end ());
  }

  void init_loaded (AutomatableTrack * track)
  {
    for (const auto &pl_var : *this)
      {
        std::visit ([&] (auto &&pl) { pl->init_loaded (track); }, pl_var);
      }
  }

  auto get_slot_type_of_first_plugin () const
  {
    return slot_type_projection (Base::front ());
  }
  auto get_track_id_of_first_plugin () const
  {
    return track_id_projection (Base::front ());
  }
};

using PluginSpan = PluginSpanImpl<
  std::span<const zrythm::gui::old_dsp::plugins::PluginPtrVariant>>;
using PluginRegistrySpan =
  PluginSpanImpl<utils::UuidIdentifiableObjectSpan<PluginRegistry>>;
using PluginUuidReferenceSpan = PluginSpanImpl<
  utils::UuidIdentifiableObjectSpan<PluginRegistry, PluginUuidReference>>;
extern template class PluginSpanImpl<std::span<const PluginSpan::VariantType>>;
extern template class PluginSpanImpl<
  utils::UuidIdentifiableObjectSpan<PluginRegistry>>;
extern template class PluginSpanImpl<
  utils::UuidIdentifiableObjectSpan<PluginRegistry, PluginUuidReference>>;

static_assert (std::ranges::random_access_range<PluginSpan>);
static_assert (std::ranges::random_access_range<PluginRegistrySpan>);
static_assert (std::ranges::random_access_range<PluginUuidReferenceSpan>);

using PluginSpanVariant =
  std::variant<PluginSpan, PluginRegistrySpan, PluginUuidReferenceSpan>;
