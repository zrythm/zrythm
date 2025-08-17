// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

// #include "plugins/carla_native_plugin.h"
#include "plugins/internal_plugin_base.h"
#include "plugins/juce_plugin.h"

namespace zrythm::plugins
{
inline auto
plugin_ptr_variant_to_base (const PluginPtrVariant &var)
{
  return std::visit ([] (auto &&pl) -> Plugin * { return pl; }, var);
}
} // namespace zrythm::plugins
