// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "plugins/plugin_all.h"
#include "utils/uuid_identifiable_object.h"

/**
 * @brief Span of plugins that offers helper methods.
 */
class PluginSpan
    : public utils::UuidIdentifiableObjectView<plugins::PluginRegistry>
{
public:
  using Base = utils::UuidIdentifiableObjectView<plugins::PluginRegistry>;
  using VariantType = typename Base::VariantType;
  using PluginUuid = typename Base::UuidType;
  using Base::Base; // Inherit constructors

  static auto name_projection (const VariantType &pl_var)
  {
    return std::visit ([] (const auto &pl) { return pl->get_name (); }, pl_var);
  }
  static auto visible_projection (const VariantType &pl_var)
  {
    return std::visit ([] (const auto &pl) { return pl->uiVisible (); }, pl_var);
  }
  static auto state_dir_projection (const VariantType &pl_var)
  {
    return std::visit (
      [] (auto &&pl) { return pl->get_state_directory (); }, pl_var);
  }
  static auto instantiation_failed_projection (const VariantType &pl_var)
  {
    return std::visit (
      [] (const auto &pl) {
        return pl->instantiationStatus ()
               == plugins::Plugin::InstantiationStatus::Failed;
      },
      pl_var);
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
    // TODO
    return {};
#if 0
    return std::ranges::to<std::vector> (
      *this | std::views::transform ([&] (const auto &plugin_var) {
        return std::visit (
          [&] (auto &&pl) -> VariantType {
            return utils::clone_qobject (*pl, &owner);
          },
          plugin_var);
      }));
#endif
  }

  /**
   * Returns whether the plugins can be pasted to the given slot.
   */
  bool can_be_pasted (const plugins::PluginSlot &slot) const;
};

static_assert (std::ranges::random_access_range<PluginSpan>);
