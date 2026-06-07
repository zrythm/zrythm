// SPDX-FileCopyrightText: © 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "dsp/midi_event_buffer.h"
#include "dsp/port.h"
#include "utils/icloneable.h"

namespace zrythm::dsp
{

class MidiPort final : public Port, public PortConnectionsCacheMixin<MidiPort>
{
  Q_OBJECT
  QML_ELEMENT
  QML_UNCREATABLE ("")

public:
  MidiPort (utils::Utf8String label, PortFlow flow);

  [[gnu::hot]] void process_block (
    dsp::graph::ProcessBlockInfo time_nfo,
    const dsp::ITransport       &transport,
    const dsp::TempoMap         &tempo_map) noexcept override;

  void prepare_for_processing_impl (
    const graph::GraphNode * node,
    units::sample_rate_t     sample_rate,
    units::sample_u32_t      max_block_length) override;

  void release_resources () override;

  void clear_buffer (std::size_t offset, std::size_t nframes) override;

  friend void init_from (
    MidiPort              &obj,
    const MidiPort        &other,
    utils::ObjectCloneType clone_type);

private:
  friend void to_json (nlohmann::json &j, const MidiPort &p)
  {
    to_json (j, static_cast<const Port &> (p));
  }
  friend void from_json (const nlohmann::json &j, MidiPort &p)
  {
    from_json (j, static_cast<Port &> (p));
  }

public:
  dsp::MidiEventBuffer buffer_;

  BOOST_DESCRIBE_CLASS (MidiPort, (Port), (), (), ())
};

} // namespace zrythm::dsp
