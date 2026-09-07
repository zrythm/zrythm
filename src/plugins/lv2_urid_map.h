// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <cstdint>
#include <memory>

namespace zrythm::plugins
{

/**
 * @brief URIDs the host itself forges and parses, cached once for
 * realtime-safe access on the audio thread.
 */
struct Lv2HostUrids
{
  uint32_t atom_Sequence = 0;
  uint32_t atom_Chunk = 0;
  uint32_t atom_Float = 0;
  uint32_t atom_Int = 0;
  uint32_t atom_Long = 0;
  uint32_t atom_Double = 0;
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
 * @brief LV2 URI <-> URID table, owned by the Lv2World it belongs to.
 *
 * LV2 identifies extension data by numeric URIDs that are host-specific.
 * Everything sharing one lilv world must share one table, so URIDs are
 * valid across plugin instances and for lilv's state API: the table is
 * owned by the Lv2World it belongs to.
 *
 * The LV2 urid specification requires the map to be dynamic ("hosts
 * SHOULD NOT return 0... the URI map SHOULD be dynamic") and does not
 * require map() to be realtime-safe ("plugins SHOULD cache any IDs they
 * might need in performance critical situations"). Mapping a URI for the
 * first time allocates, like in other LV2 hosts (jalv, Ardour).
 * Host code that runs on the audio thread uses the pre-cached URIDs of
 * Lv2World::host_urids() instead of calling map().
 *
 * @return 0 only when a URID could not be created (empty/null URI);
 * unmap() returns nullptr for unknown URIDs.
 */
class Lv2UridMap
{
public:
  /**
   * @brief Returns the URID for @p uri, assigning one on first sight.
   *
   * Not realtime-safe (may allocate and locks a mutex): call from main
   * or non-audio threads only. Realtime code uses the cached values of
   * Lv2World::host_urids().
   */
  uint32_t map (const char * uri) [[clang::blocking]];

  /**
   * @brief Returns the URI for @p urid, or nullptr when @p urid was
   * not handed out by this table.
   *
   * The returned string stays valid for the lifetime of the table.
   * Not realtime-safe (locks a mutex).
   */
  const char * unmap (uint32_t urid) const [[clang::blocking]];

  /**
   * @brief Maps the URIDs the host itself forges and parses (atom, MIDI,
   * time, parameters, buffer-size).
   *
   * Main-thread only; realtime code reads the cached copy without locking.
   */
  Lv2HostUrids host_urids () [[clang::blocking]];

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

} // namespace zrythm::plugins
