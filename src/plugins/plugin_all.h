// SPDX-FileCopyrightText: © 2025-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "plugins/clap_plugin.h"
#include "plugins/faust/faust_plugin.h"
#include "plugins/juce_plugin.h"
#include "plugins/vst3_plugin.h"

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
  if (auto * clap = qobject_cast<ClapPlugin *> (pl))
    {
      return clap;
    }
  if (auto * vst3 = qobject_cast<Vst3Plugin *> (pl))
    {
      return vst3;
    }
  if (auto * juce = qobject_cast<JucePlugin *> (pl))
    {
      return juce;
    }
  if (auto * faust = qobject_cast<FaustPlugin *> (pl))
    {
      return faust;
    }
  throw std::invalid_argument ("Invalid plugin ptr");
}
} // namespace zrythm::plugins
