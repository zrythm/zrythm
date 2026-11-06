// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <vector>

#include "plugins/internal_plugin_base.h"

namespace zrythm::plugins::modulators
{

/**
 * @brief Host plugin for the builtin native modulator nodes.
 *
 * A single plugin type hosts every native modulator (Macro, LFO, etc.); the
 * configured descriptor selects the processing engine. Only this host type is
 * part of PluginVariant, so adding a node does not change the serialized
 * plugin-type list.
 */
class Modulator final : public InternalPluginBase
{
  Q_OBJECT
  QML_ELEMENT
  QML_UNCREATABLE ("")

public:
  /** Returns the descriptor IDs of the available native modulators. */
  static std::vector<std::string_view> available_descriptor_ids ();

  /**
   * @brief Returns a new descriptor for a native modulator ID, or null when no
   * modulator with that ID exists.
   */
  [[nodiscard]] static std::unique_ptr<PluginDescriptor>
  make_descriptor (std::string_view descriptor_id);

  /** Returns whether @p descriptor identifies a native modulator hosted by
   * this plugin type (Internal protocol with a registered modulator ID). */
  [[nodiscard]] static bool
  descriptor_is_native_modulator (const PluginDescriptor &descriptor);

  Modulator (utils::IObjectRegistry &registry, QObject * parent = nullptr);
  ~Modulator () override;

protected:
  void prepare_plugin_for_processing (
    units::sample_rate_t sample_rate,
    units::sample_u32_t  max_block_length) override;

  void process_impl (
    dsp::graph::ProcessBlockInfo time_info,
    const dsp::ITransport &,
    const dsp::TempoMap &) noexcept [[clang::nonblocking]] override;

  void release_resources_impl () override;

protected Q_SLOTS:
  void on_configuration_changed (
    PluginConfiguration * configuration,
    bool                  generateNewPluginPortsAndParams) override;

private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};

} // namespace zrythm::plugins::modulators
