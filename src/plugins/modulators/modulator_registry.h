// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <span>

#include "plugins/modulators/modulator_engine.h"

namespace zrythm::plugins::modulators
{

/** A native modulator that can be instantiated by the Modulator host. */
struct ModulatorRegistration final
{
  std::string_view descriptor_id_;

  std::unique_ptr<PluginDescriptor> (*make_descriptor_) ();
  std::unique_ptr<ModulatorEngine> (*make_engine_) ();
};

/** Returns every registered native modulator. */
std::span<const ModulatorRegistration>
modulator_registrations ();

/**
 * @brief Returns the registration with @p descriptor_id, or null when the ID
 * does not identify a native modulator.
 */
const ModulatorRegistration *
find_modulator_registration (std::string_view descriptor_id);

} // namespace zrythm::plugins::modulators
