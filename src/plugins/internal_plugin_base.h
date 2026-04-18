// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "plugins/plugin.h"

namespace zrythm::plugins
{

/**
 * @brief A base class for internal plugins.
 *
 * This plugin on its own simply passes audio through without any processing.
 */
class InternalPluginBase : public Plugin
{
  Q_OBJECT
  QML_ELEMENT
  QML_UNCREATABLE ("")

public:
  /**
   * @brief Constructor for InternalPluginBase.
   *
   * @param dependencies Processor dependencies
   * @param parent Parent QObject
   */
  InternalPluginBase (
    dsp::ProcessorBase::ProcessorBaseDependencies dependencies,
    QObject *                                     parent = nullptr);

  ~InternalPluginBase () override;

protected:
  void prepare_for_processing_impl (
    units::sample_rate_t sample_rate,
    units::sample_u32_t  max_block_length) override;

  void
  process_impl (dsp::graph::EngineProcessTimeInfo time_info) noexcept override;

  std::string save_state_impl () const override { return {}; }
  void        load_state_impl (const std::string &) override { }

private Q_SLOTS:
  /**
   * @brief Handle configuration changes.
   */
  void on_configuration_changed (
    PluginConfiguration * configuration,
    bool                  generateNewPluginPortsAndParams);

  /**
   * @brief Handle visibility changes.
   */
  void on_ui_visibility_changed ();
};

} // namespace zrythm::plugins
