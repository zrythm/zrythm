// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

// #include "plugins/carla_native_plugin.h"
#include "plugins/clap_plugin.h"
#include "plugins/internal_plugin_base.h"
#include "plugins/juce_plugin.h"

namespace zrythm::plugins
{
inline auto
plugin_ptr_variant_to_base (const PluginPtrVariant &var)
{
  return std::visit ([] (auto &&pl) -> Plugin * { return pl; }, var);
}

inline auto
plugin_base_to_ptr_variant (Plugin * pl) -> PluginPtrVariant
{
  if (auto * clap = dynamic_cast<ClapPlugin *> (pl))
    {
      return clap;
    }
  if (auto * juce = dynamic_cast<JucePlugin *> (pl))
    {
      return juce;
    }
  if (auto * internal = dynamic_cast<InternalPluginBase *> (pl))
    {
      return internal;
    }
  throw std::invalid_argument ("Invalid plugin ptr");
}
} // namespace zrythm::plugins
