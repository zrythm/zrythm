// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <optional>
#include <unordered_map>

namespace zrythm::utils
{

/**
 * @class ConstBidirectionalMap
 * @tparam K Key type
 * @tparam V Value type
 * @brief A read-only bidirectional map that maintains key-value and value-key
 * mappings.
 *
 * This class provides constant-time lookups in both directions (key to value
 * and value to key) using two internal unordered maps. The map is constructed
 * with an initializer list and cannot be modified after construction.
 */
template <typename K, typename V> class ConstBidirectionalMap
{
  std::unordered_map<K, V> key_to_val_;
  std::unordered_map<V, K> val_to_key_;

public:
  ConstBidirectionalMap (std::initializer_list<std::pair<K, V>> list)
  {
    for (const auto &[key, val] : list)
      {
        insert (key, val);
      }
  }
  void insert (const K &key, const V &val)
  {
    key_to_val_.insert ({ key, val });
    val_to_key_.insert ({ val, key });
  }

  std::optional<V> find_by_key (const K &key) const
  {
    if (auto it = key_to_val_.find (key); it != key_to_val_.end ())
      return it->second;
    return std::nullopt;
  }

  std::optional<K> find_by_value (const V &val) const
  {
    if (val_to_key_.contains (val))
      {
        return val_to_key_.at (val);
      }
    return std::nullopt;
  }
};

}
