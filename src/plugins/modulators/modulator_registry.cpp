// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <array>
#include <ranges>

#include "plugins/modulators/engines/macro_engine.h"
#include "plugins/modulators/modulator_registry.h"

namespace zrythm::plugins::modulators
{

namespace
{

const std::array<ModulatorRegistration, 1> kModulators = {
  ModulatorRegistration{
                        .descriptor_id_ = MacroEngine::kDescriptorId,
                        .make_descriptor_ = &MacroEngine::make_descriptor,
                        .make_engine_ = [] () -> std::unique_ptr<ModulatorEngine> {
      return std::make_unique<MacroEngine> ();
    } },
};

}

std::span<const ModulatorRegistration>
modulator_registrations ()
{
  return kModulators;
}

const ModulatorRegistration *
find_modulator_registration (std::string_view descriptor_id)
{
  const auto found = std::ranges::find (
    kModulators, descriptor_id, &ModulatorRegistration::descriptor_id_);
  return found != kModulators.end () ? &*found : nullptr;
}

} // namespace zrythm::plugins::modulators
