// SPDX-FileCopyrightText: © 2020-2021, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef __PLUGINS_CARLA_DISCOVERY_H__
#define __PLUGINS_CARLA_DISCOVERY_H__

#include "zrythm-config.h"

#include <filesystem>

#if HAVE_CARLA

#  include "gui/dsp/plugin_descriptor.h"
#  include "utils/types.h"

#  include "carla_wrapper.h"

namespace zrythm::gui::old_dsp::plugins
{

class PluginDescriptor;
class PluginManager;
class CarlaDiscoveryStartThread;

/**
 * @addtogroup plugins
 *
 * @{
 */

class ZCarlaDiscovery
{
public:
  ZCarlaDiscovery (PluginManager &owner);

  friend class CarlaDiscoveryStartThread;

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

  /**
   * Array of CarlaPluginDiscoveryHandle's and a boolean whether done.
   */
  std::vector<std::pair<CarlaPluginDiscoveryHandle, bool>> handles_;

  std::mutex handles_mutex_;

public:
  /**
   * @brief Pointer to owner.
   */
  PluginManager * owner_ = nullptr;
};

class CarlaDiscoveryStartThread : public QThread
{
  Q_OBJECT

public:
  CarlaDiscoveryStartThread (
    BinaryType       btype,
    PluginProtocol   protocol,
    ZCarlaDiscovery &self);

  friend class ZCarlaDiscovery;

  void run () override;

public:
  BinaryType       btype_;
  PluginProtocol   protocol_;
  ZCarlaDiscovery &discovery_;
};

} // namespace zrythm::gui::old_dsp::plugins

/**
 * @}
 */

#endif // HAVE_CARLA
#endif // __PLUGINS_CARLA_DISCOVERY_H__
