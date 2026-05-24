// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "dsp/processor_base.h"
#include "utils/units.h"

#include <QObject>

namespace zrythm::dsp
{

/**
 * @brief Bridges hardware MIDI input into the DSP graph as a MidiPort output.
 *
 * During processing, copies MIDI events from a provider callback into the
 * output MidiPort's queued events buffer. The port's process_block will
 * dequeue them to active events for downstream consumers.
 */
class MidiInputProcessor final : public QObject, public ProcessorBase
{
  Q_OBJECT

public:
  using MidiEventProvider = std::function<const juce::MidiBuffer &()>;

  MidiInputProcessor (
    MidiEventProvider       provider,
    utils::IObjectRegistry &registry,
    QObject *               parent = nullptr);

  void custom_process_block (
    dsp::graph::ProcessBlockInfo time_nfo,
    const dsp::ITransport       &transport,
    const dsp::TempoMap &tempo_map) noexcept [[clang::nonblocking]] override;

  dsp::MidiPort &get_output_port () const noexcept;

private:
  MidiEventProvider provider_;
};

}
