// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <concepts>
#include <optional>
#include <span>
#include <string_view>
#include <variant>

#include "structure/arrangement/editor_function_param.h"
#include "utils/logger.h"
#include "utils/utf8_string.h"

namespace zrythm::structure::arrangement
{

/**
 * @brief The contract every editor function kind fulfills.
 *
 * A kind is a struct whose instance carries the function's parameters,
 * which also declares its parameters for the generic function dialog
 * (@ref params) and provides its pure computation over a selection of
 * @p ObjectT.
 */
template <typename KindT, typename ObjectT>
concept EditorFunctionKind =
  requires (std::span<ObjectT * const> objects, const KindT &kind) {
    { KindT::kId } -> std::convertible_to<std::string_view>;
    { KindT::kMinimumSelectionCount } -> std::convertible_to<int>;
    { KindT::label (false) } -> std::same_as<utils::Utf8String>;
    { KindT::params () };
    { KindT::compute (objects, kind) };
  };

namespace detail
{

/// Returns the kind held by the variant with the given id, if any.
template <typename KindVariant>
std::optional<KindVariant>
kind_from_id (std::string_view id)
{
  std::optional<KindVariant> found;
  [&]<std::size_t... Is> (std::index_sequence<Is...>) {
    ((
      [&] {
        if (id == std::variant_alternative_t<Is, KindVariant>::kId)
          found.emplace (std::in_place_index<Is>);
      }(),
      ...));
  }(std::make_index_sequence<std::variant_size_v<KindVariant>>{});
  return found;
}

/// Returns the user-visible name of the kind held by @p kind.
template <typename KindVariant>
utils::Utf8String
label (const KindVariant &kind, bool translate)
{
  return std::visit (
    [translate] (const auto &k) {
      using K = std::decay_t<decltype (k)>;
      return K::label (translate);
    },
    kind);
}

/// Computes the result of the kind held by @p kind over @p objects.
template <typename KindVariant, typename ObjectT>
auto
compute (std::span<ObjectT * const> objects, const KindVariant &kind)
{
  return std::visit (
    [&] (const auto &k) {
      using K = std::decay_t<decltype (k)>;
      z_debug ("computing {}...", K::kId);
      return K::compute (objects, k);
    },
    kind);
}

} // namespace detail

} // namespace zrythm::structure::arrangement
