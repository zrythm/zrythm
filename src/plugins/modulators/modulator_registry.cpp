// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <array>
#include <ranges>

#include "plugins/modulators/engines/adsr_engine.h"
#include "plugins/modulators/engines/envelope_follower_engine.h"
#include "plugins/modulators/engines/lfo_engine.h"
#include "plugins/modulators/engines/macro_engine.h"
#include "plugins/modulators/engines/math_engine.h"
#include "plugins/modulators/engines/midi_to_cv_engine.h"
#include "plugins/modulators/modulator_registry.h"

namespace zrythm::plugins::modulators
{

namespace
{

const std::array<ModulatorRegistration, 6> kModulators = {
  ModulatorRegistration{
                        .descriptor_id_ = MacroEngine::kDescriptorId,
                        .make_descriptor_ = &MacroEngine::make_descriptor,
                        .make_engine_ = [] () -> std::unique_ptr<ModulatorEngine> {
      return std::make_unique<MacroEngine> ();
    } },
  ModulatorRegistration{
                        .descriptor_id_ = LfoEngine::kDescriptorId,
                        .make_descriptor_ = &LfoEngine::make_descriptor,
                        .make_engine_ = [] () -> std::unique_ptr<ModulatorEngine> {
      return std::make_unique<LfoEngine> ();
    } },
  ModulatorRegistration{
                        .descriptor_id_ = EnvelopeFollowerEngine::kDescriptorId,
                        .make_descriptor_ = &EnvelopeFollowerEngine::make_descriptor,
                        .make_engine_ = [] () -> std::unique_ptr<ModulatorEngine> {
      return std::make_unique<EnvelopeFollowerEngine> ();
    } },
  ModulatorRegistration{
                        .descriptor_id_ = MidiToCvEngine::kDescriptorId,
                        .make_descriptor_ = &MidiToCvEngine::make_descriptor,
                        .make_engine_ = [] () -> std::unique_ptr<ModulatorEngine> {
      return std::make_unique<MidiToCvEngine> ();
    } },
  ModulatorRegistration{
                        .descriptor_id_ = AdsrEngine::kDescriptorId,
                        .make_descriptor_ = &AdsrEngine::make_descriptor,
                        .make_engine_ = [] () -> std::unique_ptr<ModulatorEngine> {
      return std::make_unique<AdsrEngine> ();
    } },
  ModulatorRegistration{
                        .descriptor_id_ = MathEngine::kDescriptorId,
                        .make_descriptor_ = &MathEngine::make_descriptor,
                        .make_engine_ = [] () -> std::unique_ptr<ModulatorEngine> {
      return std::make_unique<MathEngine> ();
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
