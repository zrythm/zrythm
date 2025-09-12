// SPDX-FileCopyrightText: © 2021-2022, 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "plugins/plugin_descriptor.h"

namespace zrythm::plugins
{

/**
 * Configuration for instantiating a plugin descriptor.
 *
 * (Previously `PluginSetting`).
 */
class PluginConfiguration : public QObject
{
  Q_OBJECT
  Q_PROPERTY (
    zrythm::plugins::PluginDescriptor * descriptor READ descriptor CONSTANT)
  QML_ELEMENT
  QML_UNCREATABLE ("")

public:
  PluginConfiguration (QObject * parent = nullptr) : QObject (parent) { };

  // ============================================================================
  // QML Interface
  // ============================================================================

  PluginDescriptor * descriptor () const { return descr_.get (); }

  // ============================================================================

  /**
   * Creates a plugin setting with the recommended settings for the given plugin
   * descriptor based on the current setup.
   */
  static std::unique_ptr<PluginConfiguration>
  create_new_for_descriptor (const zrythm::plugins::PluginDescriptor &descr);

  enum class HostingType : std::uint8_t
  {
    /**
     * @brief Plugin hosted via JUCE.
     */
    JUCE,

    /**
     * @brief Plugin hosted via custom plugin format implementation.
     *
     * @note Unimplemented.
     */
    Custom
  };

public:
  /**
   * Makes sure the setting is valid in the current run and changes any fields
   * to make it conform.
   */
  void validate ();

  void print () const;

  friend void init_from (
    PluginConfiguration       &obj,
    const PluginConfiguration &other,
    utils::ObjectCloneType     clone_type);

  zrythm::plugins::PluginDescriptor * get_descriptor () const
  {
    return descr_.get ();
  }

  auto get_name () const { return get_descriptor ()->name_; }

  void copy_fields_from (const PluginConfiguration &other);

private:
  static constexpr auto kDescriptorKey = "descriptor"sv;
  static constexpr auto kForceGenericUIKey = "forceGenericUI"sv;
  static constexpr auto kBridgeModeKey = "bridgeMode"sv;
  friend void to_json (nlohmann::json &j, const PluginConfiguration &p)
  {
    j = nlohmann::json{
      { kDescriptorKey,     p.descr_            },
      { kForceGenericUIKey, p.force_generic_ui_ },
      { kBridgeModeKey,     p.bridge_mode_      },
    };
  }
  friend void from_json (const nlohmann::json &j, PluginConfiguration &p);

public:
  /** The descriptor of the plugin this setting is for. */
  std::unique_ptr<zrythm::plugins::PluginDescriptor> descr_;

  HostingType hosting_type_{ HostingType::JUCE };

  /** Whether to force a generic UI. */
  bool force_generic_ui_{};

  /** Requested carla bridge mode. */
  zrythm::plugins::BridgeMode bridge_mode_{};

  BOOST_DESCRIBE_CLASS (
    PluginConfiguration,
    (),
    (descr_, hosting_type_, force_generic_ui_, bridge_mode_),
    (),
    ())
};

}
