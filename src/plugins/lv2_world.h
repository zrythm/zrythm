// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <filesystem>
#include <memory>
#include <string_view>

#include "utils/utf8_string.h"

#include <lilv/lilv.h>

namespace zrythm::plugins
{

struct LilvNodeDeleter
{
  void operator() (LilvNode * node) const { lilv_node_free (node); }
};
using LilvNodeUPtr = std::unique_ptr<LilvNode, LilvNodeDeleter>;

struct LilvNodesDeleter
{
  void operator() (LilvNodes * nodes) const { lilv_nodes_free (nodes); }
};
using LilvNodesUPtr = std::unique_ptr<LilvNodes, LilvNodesDeleter>;

struct LilvUIsDeleter
{
  void operator() (LilvUIs * uis) const { lilv_uis_free (uis); }
};
using LilvUIsUPtr = std::unique_ptr<LilvUIs, LilvUIsDeleter>;

inline utils::Utf8String
node_to_utf8 (const LilvNode * node)
{
  return node != nullptr
           ? utils::Utf8String::from_utf8_encoded_string (
               lilv_node_as_string (node))
           : utils::Utf8String{};
}

/**
 * @brief RAII owner of a lilv world, shared by scanning and hosting.
 *
 * @param spec_bundles_dir Directory containing the LV2 specification
 * bundles, loaded so that the plugin class hierarchy and extension
 * vocabularies resolve.
 * @throw ZrythmException on construction if the world cannot be created.
 *
 * @note All methods are main-thread only (lilv world mutation is not
 * synchronized).
 */
class Lv2World
{
public:
  explicit Lv2World (const std::filesystem::path &spec_bundles_dir);
  ~Lv2World ();
  Lv2World (const Lv2World &) = delete;
  Lv2World &operator= (const Lv2World &) = delete;
  Lv2World (Lv2World &&) = delete;
  Lv2World &operator= (Lv2World &&) = delete;

  /**
   * @brief Loads @p bundle_dir into the world and returns the plugin
   * with @p uri.
   *
   * Loading is idempotent; previously loaded bundles stay in the world.
   *
   * @return The plugin, valid for the process lifetime, or nullptr when
   * the bundle contains no plugin with that URI.
   */
  const LilvPlugin *
  find_plugin (const std::filesystem::path &bundle_dir, std::string_view uri);

  /**
   * @brief Raw world access for LV2 backend implementation files
   * (discovery, hosting).
   */
  LilvWorld * raw () const noexcept { return world_.get (); }

private:
  struct WorldDeleter
  {
    void operator() (LilvWorld * world) const { lilv_world_free (world); }
  };
  std::unique_ptr<LilvWorld, WorldDeleter> world_;
};

} // namespace zrythm::plugins
