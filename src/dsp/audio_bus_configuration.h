// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <cstdint>
#include <optional>
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

  /** Bus purpose as reported by the hosting SDK (main or sidechain). */
  AudioPort::Purpose purpose{};

  /** Inactive buses are (or become) detached ports. */
  bool active{ true };

  /**
   * The stable id the hosting SDK assigns to this bus (e.g. the CLAP audio
   * port id), if any.
   *
   * Buses with a stable id match ports carrying the same
   * AudioPort::external_port_id, even when a removal shifted enumeration
   * indices; a matched port adopts the id when it has none (e.g. saved before
   * ids were recorded).
   */
  std::optional<uint32_t> external_id;
};

/**
 * @brief What reconcile_audio_bus_configuration() changed.
 */
struct AudioBusReconcileResult
{
  /**
   * The port set, an arrangement, a purpose or a detached state changed.
   *
   * The graph must be recalculated and all nodes re-prepared before
   * processing resumes: reconciliation may have dropped port buffers or
   * created unprepared ports, and purposes feed graph wiring.
   */
  bool graph_changed;

  /**
   * Only metadata changed: a port's label or external id. Neither affects
   * buffers or the graph, so no recalculation is needed.
   */
  bool metadata_changed;
};

/**
 * @brief Applies an accepted bus configuration to a processor's audio ports
 * as a delta, returning what changed.
 *
 * MIDI and CV ports are not counted and are never touched. For each entry of
 * @p desired, in order:
 *
 * - when the entry carries a stable external id and an unmatched port carries
 *   the same id (see AudioBusConfig::external_id): that port is matched, even
 *   if a removal shifted enumeration indices
 * - otherwise: the first unmatched port carrying no external id, in
 *   port-list order (ports saved before ids were recorded adopt the reported
 *   id)
 * - no match: a new AudioPort is created with a fresh UUID (detached when the
 *   desired bus is inactive)
 *
 * A matched port's arrangement, label, purpose and detached state are synced
 * in place per the entry. Ports nothing matched are detached. Ports are never
 * destroyed, so port UUIDs, connections and QML pointers stay valid across
 * reconciliation.
 *
 * Must be called on the main thread with the engine paused. See
 * AudioBusReconcileResult for the caller's obligations.
 */
[[nodiscard]] AudioBusReconcileResult
reconcile_audio_bus_configuration (
  utils::IObjectRegistry         &registry,
  ProcessorBase                  &processor,
  PortFlow                        flow,
  std::span<const AudioBusConfig> desired) [[clang::blocking]];

} // namespace zrythm::dsp
