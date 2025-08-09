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

} // namespace zrythm::plugins
