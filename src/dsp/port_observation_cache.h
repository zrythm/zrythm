// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <array>
#include <vector>

#include "dsp/midi_event.h"

namespace zrythm::dsp
{

/**
 * @brief Per-requester cache for drained observation data.
 *
 * Filled by the manager's drain timer (consuming reads from observer ring
 * buffers). Read by UI components at their own pace — no synchronization
 * needed since each requester has its own cache.
 */
struct PortObservationCache
{
  static constexpr size_t kMaxAudioSamples = 100000;
  static constexpr size_t kMaxMidiEvents = 4096;

  std::vector<std::vector<float>> audio;
  std::vector<RealtimeMidiEvent>  midi;

  void clear_audio ()
  {
    for (auto &c : audio)
      c.clear ();
  }
  void clear_midi () { midi.clear (); }
  void clear ()
  {
    clear_audio ();
    clear_midi ();
  }
};

}
