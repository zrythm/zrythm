// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <span>

#include "dsp/audio_port.h"
#include "dsp/processor_base.h"
#include "dsp/speaker_arrangement.h"
#include "utils/iobject_registry.h"
#include "utils/utf8_string.h"

namespace zrythm::dsp
{

/**
 * @brief One audio bus of a processor's bus configuration.
 *
 * This is the currency of bus configuration negotiation: processors report
 * their current configuration as a list of these per flow, and
 * reconcile_audio_bus_configuration() applies an accepted configuration to
 * the processor's ports.
 */
struct AudioBusConfig
{
  /** Bus name, synced to the port's label. */
  utils::Utf8String name;

  SpeakerArrangement arrangement;

  /**
   * Bus purpose. Only used when a port is created: purpose is creation-time
   * identity and is never mutated on an existing port.
   */
  AudioPort::Purpose purpose{};

  /** Inactive buses are (or become) detached ports. */
  bool active{ true };
};

/**
 * @brief Applies an accepted bus configuration to a processor's audio ports
 * as a delta, returning whether anything changed.
 *
 * Matching is positional: a port's position among the audio ports of @p flow
 * in the processor's bus-ordered port list is its bus index. MIDI and CV
 * ports are not counted and are never touched. For each position:
 *
 * - matching an entry in @p desired: the port's arrangement and label are
 *   synced in place, and the port is attached or detached per
 *   AudioBusConfig::active
 * - beyond @p desired: the port is detached
 * - missing: a new AudioPort is created with a fresh UUID (detached when the
 *   desired bus is inactive)
 *
 * Ports are never destroyed, so port UUIDs, connections and QML pointers
 * stay valid across reconciliation.
 *
 * Must be called on the main thread with the engine paused. When this
 * returns true, the graph must be recalculated and all nodes re-prepared
 * before processing resumes.
 */
[[nodiscard]] bool
reconcile_audio_bus_configuration (
  utils::IObjectRegistry         &registry,
  ProcessorBase                  &processor,
  PortFlow                        flow,
  std::span<const AudioBusConfig> desired) [[clang::blocking]];

} // namespace zrythm::dsp
