// SPDX-FileCopyrightText: © 2020-2021, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef __PLUGINS_CARLA_DISCOVERY_H__
#define __PLUGINS_CARLA_DISCOVERY_H__

#include "zrythm-config.h"

#include <filesystem>

#if HAVE_CARLA

#  include "common/plugins/plugin_descriptor.h"
#  include "common/utils/types.h"

#  include "carla_wrapper.h"

namespace zrythm::plugins
{

class PluginDescriptor;
class PluginManager;

/**
 * @addtogroup plugins
 *
 * @{
 */

class ZCarlaDiscovery
{
public:
  ZCarlaDiscovery (PluginManager &owner);

  void start (BinaryType btype, PluginProtocol protocol);

  /**
   * @return Whether done.
   */
  bool idle ();

  /**
   * Create a descriptor for the given AU plugin.
   */
  static std::unique_ptr<PluginDescriptor>
  create_au_descriptor_from_info (const CarlaCachedPluginInfo * info);

  static std::unique_ptr<PluginDescriptor> descriptor_from_discovery_info (
    const CarlaPluginDiscoveryInfo * info,
    std::string_view                 sha1);

private:
  /**
   * @brief Returns the absolute path to the carla-discovery binary.
   *
   * @param arch
   * @return The path, or an empty path if an issue occurred.
   */
  static fs::path get_discovery_path (PluginArchitecture arch);

public:
  /**
   * Array of CarlaPluginDiscoveryHandle's and a boolean whether done.
   */
  std::vector<std::pair<CarlaPluginDiscoveryHandle, bool>> handles_;

  /**
   * @brief Pointer to owner.
   */
  PluginManager * owner_ = nullptr;
};

} // namespace zrythm::plugins

/**
 * @}
 */

#endif // HAVE_CARLA
#endif // __PLUGINS_CARLA_DISCOVERY_H__
