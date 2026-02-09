// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <cassert>

#include "dsp/juce_hardware_audio_interface.h"

namespace zrythm::dsp
{

JuceHardwareAudioInterface::JuceHardwareAudioInterface (
  std::shared_ptr<juce::AudioDeviceManager> device_manager)
    : device_manager_ (std::move (device_manager))
{
  assert (device_manager_ != nullptr);
}

JuceHardwareAudioInterface::~JuceHardwareAudioInterface () = default;

[[nodiscard]] nframes_t
JuceHardwareAudioInterface::get_block_length () const
{
  auto * dev = device_manager_->getCurrentAudioDevice ();
  assert (dev != nullptr);
  return dev->getCurrentBufferSizeSamples ();
}

[[nodiscard]] units::sample_rate_t
JuceHardwareAudioInterface::get_sample_rate () const
{
  auto * dev = device_manager_->getCurrentAudioDevice ();
  assert (dev != nullptr);
  return units::sample_rate (static_cast<int> (dev->getCurrentSampleRate ()));
}

void
JuceHardwareAudioInterface::add_audio_callback (
  juce::AudioIODeviceCallback * callback)
{
  device_manager_->addAudioCallback (callback);
}

void
JuceHardwareAudioInterface::remove_audio_callback (
  juce::AudioIODeviceCallback * callback)
{
  device_manager_->removeAudioCallback (callback);
}

juce::AudioWorkgroup
JuceHardwareAudioInterface::get_device_audio_workgroup () const
{
  return device_manager_->getDeviceAudioWorkgroup ();
}

std::unique_ptr<IHardwareAudioInterface>
JuceHardwareAudioInterface::create (
  std::shared_ptr<juce::AudioDeviceManager> device_manager)
{
  return std::make_unique<JuceHardwareAudioInterface> (
    std::move (device_manager));
}

} // namespace zrythm::dsp
