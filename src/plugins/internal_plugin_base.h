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
   * @param state_path_provider Function to provide state directory path
   * @param parent Parent QObject
   */
  InternalPluginBase (
    dsp::ProcessorBase::ProcessorBaseDependencies dependencies,
    StateDirectoryParentPathProvider              state_path_provider,
    QObject *                                     parent = nullptr);

  ~InternalPluginBase () override;

protected:
  // Plugin interface implementation
  void save_state (std::optional<fs::path> abs_state_dir) override;
  void load_state (std::optional<fs::path> abs_state_dir) override;

  void prepare_for_processing_impl (
    units::sample_rate_t sample_rate,
    nframes_t            max_block_length) override;

  void process_impl (EngineProcessTimeInfo time_info) noexcept override;

private Q_SLOTS:
  /**
   * @brief Handle configuration changes.
   */
  void on_configuration_changed ();

  /**
   * @brief Handle visibility changes.
   */
  void on_ui_visibility_changed ();
};

} // namespace zrythm::plugins
