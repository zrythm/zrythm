// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "engine/device_io/engine.h"

namespace zrythm::engine::device_io
{
class DummyDriver : public AudioEngine::AudioDriver, public AudioEngine::MidiDriver
{
  friend class DummyEngineThread;

public:
  DummyDriver (AudioEngine &engine) : engine_ (engine) { }
  bool              setup_audio () override;
  bool              setup_midi () override;
  bool              activate_audio (bool activate) override;
  bool              activate_midi (bool activate) override { return true; }
  void              tear_down_audio () override;
  void              tear_down_midi () override { }
  utils::Utf8String get_driver_name () const override { return u8"Dummy"; }

private:
  int          process (zrythm::engine::device_io::AudioEngine * self);
  AudioEngine &engine_;

  /**
   * @brief Dummy audio DSP processing thread.
   *
   * Use signalThreadShouldExit() to stop the thread.
   *
   * @note The thread will join automatically when the engine is destroyed.
   */
  std::unique_ptr<juce::Thread> dummy_audio_thread_;
};
}
