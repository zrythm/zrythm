// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <span>

#include "dsp/processor_base.h"
#include "utils/units.h"

#include <QObject>

namespace zrythm::dsp
{

/**
 * @brief Bridges hardware audio input into the DSP graph as output ports.
 *
 * Creates stereo and mono output ports for each hardware input channel pair.
 * During processing, copies data from the hardware input provider into the
 * appropriate output port buffers.
 */
class AudioInputProcessor final : public QObject, public ProcessorBase
{
  Q_OBJECT

public:
  using InputDataProvider = std::function<std::span<const float * const> ()>;

  AudioInputProcessor (
    InputDataProvider       provider,
    units::channel_count_t  hw_input_channel_count,
    utils::IObjectRegistry &registry,
    QObject *               parent = nullptr);

  void custom_process_block (
    dsp::graph::ProcessBlockInfo time_nfo,
    const dsp::ITransport       &transport,
    const dsp::TempoMap &tempo_map) noexcept [[clang::nonblocking]] override;

  dsp::AudioPort *
  find_output_port (int first_channel_index, bool stereo) const noexcept
    [[clang::nonblocking]];

private:
  InputDataProvider provider_;

  struct PortMapping
  {
    AudioPort * port; ///< Owned by PortRegistry (outlives this processor)
    int         src_channel_start;
    units::channel_count_t src_channel_count;
  };

  std::vector<PortMapping> port_mappings_;
};

}
