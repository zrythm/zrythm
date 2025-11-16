// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "plugins/internal_plugin_base.h"

namespace zrythm::plugins
{

InternalPluginBase::InternalPluginBase (
  dsp::ProcessorBase::ProcessorBaseDependencies dependencies,
  StateDirectoryParentPathProvider              state_path_provider,
  QObject *                                     parent)
    : Plugin (dependencies, std::move (state_path_provider), parent)
{
  // Connect to configuration changes
  connect (
    this, &Plugin::configurationChanged, this,
    &InternalPluginBase::on_configuration_changed);

  // Connect to UI visibility changes
  connect (
    this, &Plugin::uiVisibleChanged, this,
    &InternalPluginBase::on_ui_visibility_changed);

  auto bypass_ref = generate_default_bypass_param ();
  add_parameter (bypass_ref);
  bypass_id_ = bypass_ref.id ();
  auto gain_ref = generate_default_gain_param ();
  add_parameter (gain_ref);
  gain_id_ = gain_ref.id ();
}

void
InternalPluginBase::on_configuration_changed ()
{
  z_debug ("configuration changed");

  // Reinitialize plugin with new configuration

  Q_EMIT instantiationFinished (true, {});
}

void
InternalPluginBase::on_ui_visibility_changed ()
{
  // TODO
}

void
InternalPluginBase::save_state (std::optional<fs::path> abs_state_dir)
{
  // Minimal implementation - do nothing for dummy plugin
}

void
InternalPluginBase::load_state (std::optional<fs::path> abs_state_dir)
{
  // Minimal implementation - do nothing for dummy plugin
}

void
InternalPluginBase::prepare_for_processing_impl (
  sample_rate_t sample_rate,
  nframes_t     max_block_length)
{
  // Minimal implementation - no special preparation needed
}

void
InternalPluginBase::process_impl (EngineProcessTimeInfo time_info) noexcept
{
  // Simple pass-through implementation
}

InternalPluginBase::~InternalPluginBase () = default;

} // namespace zrythm::plugins
