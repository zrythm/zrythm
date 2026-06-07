// SPDX-FileCopyrightText: © 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "dsp/port.h"
#include "utils/icloneable.h"

namespace zrythm::dsp
{

/**
 * @brief Control Voltage port.
 *
 * This port provides sample-accurante signals (similar to audio) and can be
 * used to modulate parameters.
 *
 * The range is assumed to be 0 to 1.
 */
class CVPort final : public Port, public PortConnectionsCacheMixin<CVPort>
{
  Q_OBJECT
  QML_ELEMENT
  QML_UNCREATABLE ("")

public:
  CVPort (utils::Utf8String label, PortFlow flow);

  [[gnu::hot]] void process_block (
    dsp::graph::ProcessBlockInfo time_nfo,
    const dsp::ITransport       &transport,
    const dsp::TempoMap         &tempo_map) noexcept override;

  void clear_buffer (std::size_t offset, std::size_t nframes) override;

  friend void
  init_from (CVPort &obj, const CVPort &other, utils::ObjectCloneType clone_type);

  void prepare_for_processing_impl (
    const graph::GraphNode * node,
    units::sample_rate_t     sample_rate,
    units::sample_u32_t      max_block_length) override;
  void release_resources () override;

private:
  static constexpr std::string_view kRangeKey = "range";
  friend void                       to_json (nlohmann::json &j, const CVPort &p)
  {
    to_json (j, static_cast<const Port &> (p));
  }
  friend void from_json (const nlohmann::json &j, CVPort &p)
  {
    from_json (j, static_cast<Port &> (p));
  }

public:
  std::vector<float> buf_;

private:
  BOOST_DESCRIBE_CLASS (CVPort, (Port), (), (), ())
};

} // namespace zrythm::dsp
