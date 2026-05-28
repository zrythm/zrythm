// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <memory>

#include "dsp/processor_base.h"
#include "utils/units.h"

#include <QObject>

namespace zrythm::dsp
{

class MidiDeviceBuffer;

/**
 * @brief Bridges hardware MIDI input into the DSP graph as a MidiPort output.
 *
 * During processing, drains events from a MidiDeviceBuffer into the
 * output MidiPort's queued events buffer.
 */
class MidiInputProcessor final : public QObject, public ProcessorBase
{
  Q_OBJECT

public:
  MidiInputProcessor (
    std::shared_ptr<dsp::MidiDeviceBuffer> buffer,
    utils::IObjectRegistry                &registry,
    QObject *                              parent = nullptr);

  ~MidiInputProcessor () override;

  void custom_prepare_for_processing (
    const dsp::graph::GraphNode * node,
    units::sample_rate_t          sample_rate,
    units::sample_u32_t           max_block_length) override;

  void custom_process_block (
    dsp::graph::ProcessBlockInfo time_nfo,
    const dsp::ITransport       &transport,
    const dsp::TempoMap &tempo_map) noexcept [[clang::nonblocking]] override;

  dsp::MidiPort &get_output_port () const noexcept;

  void set_block_start_time (units::precise_second_t time);

private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};

}
