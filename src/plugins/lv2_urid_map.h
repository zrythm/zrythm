// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <cstdint>
#include <memory>

namespace zrythm::plugins
{

/**
 * @brief Process-wide LV2 URI <-> URID table.
 *
 * LV2 identifies extension data by numeric URIDs that are host-specific.
 * One shared table assigns a stable URID to every URI seen in the
 * process, so URIDs are valid across plugin instances and for lilv's
 * state API.
 *
 * The LV2 urid specification requires the map to be dynamic ("hosts
 * SHOULD NOT return 0... the URI map SHOULD be dynamic") and does not
 * require map() to be realtime-safe ("plugins SHOULD cache any IDs they
 * might need in performance critical situations"). Mapping a URI for
 * the first time allocates, like in other LV2 hosts (jalv, Ardour).
 * Host code that runs on the audio thread uses the pre-cached URIDs of
 * lv2_host_urids() instead of calling map().
 *
 * @return 0 only when a URID could not be created (empty/null URI);
 * unmap() returns nullptr for unknown URIDs.
 */
class Lv2UridMap
{
public:
  /**
   * @brief Returns the process-wide shared table.
   */
  static Lv2UridMap &instance ();

  /**
   * @brief Returns the URID for @p uri, assigning one on first sight.
   *
   * Not realtime-safe (may allocate and locks a mutex): call from main
   * or non-audio threads only. Realtime code uses the cached values of
   * lv2_host_urids().
   */
  uint32_t map (const char * uri) [[clang::blocking]];

  /**
   * @brief Returns the URI for @p urid, or nullptr when @p urid was
   * not handed out by this table.
   *
   * The returned string stays valid for the lifetime of the process.
   * Not realtime-safe (locks a mutex).
   */
  const char * unmap (uint32_t urid) const [[clang::blocking]];

  Lv2UridMap ();
  ~Lv2UridMap ();
  Lv2UridMap (const Lv2UridMap &) = delete;
  Lv2UridMap &operator= (const Lv2UridMap &) = delete;
  Lv2UridMap (Lv2UridMap &&) = delete;
  Lv2UridMap &operator= (Lv2UridMap &&) = delete;

private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};

/**
 * @brief URIDs the host itself forges and parses, cached once for
 * realtime-safe access on the audio thread.
 */
struct Lv2HostUrids
{
  uint32_t atom_Sequence = 0;
  uint32_t atom_Object = 0;
  uint32_t midi_MidiEvent = 0;
  uint32_t time_Position = 0;
  uint32_t time_speed = 0;
  uint32_t time_frame = 0;
  uint32_t time_framesPerSecond = 0;
  uint32_t time_bar = 0;
  uint32_t time_barBeat = 0;
  uint32_t time_beatUnit = 0;
  uint32_t time_beatsPerBar = 0;
  uint32_t time_beatsPerMinute = 0;
  uint32_t param_sampleRate = 0;
  uint32_t bufsz_minBlockLength = 0;
  uint32_t bufsz_maxBlockLength = 0;
  uint32_t bufsz_nominalBlockLength = 0;
  uint32_t bufsz_sequenceSize = 0;
};

/**
 * @brief Returns the cached host URIDs (initialized on first use).
 */
const Lv2HostUrids &
lv2_host_urids ();

} // namespace zrythm::plugins
