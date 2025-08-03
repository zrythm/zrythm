// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <utility>

#include "dsp/graph.h"
#include "dsp/graph_node.h"
#include "dsp/parameter.h"
#include "dsp/port_all.h"

#include <QObject>

namespace zrythm::dsp
{

/**
 * @brief A base class for processors in the DSP graph.
 *
 * @note Custom connections to processors should only be made on their output
 * ports (because input ports are clared after processing processors).
 */
class ProcessorBase : public dsp::graph::IProcessable
{
public:
  struct ProcessorBaseDependencies
  {
    dsp::PortRegistry               &port_registry_;
    dsp::ProcessorParameterRegistry &param_registry_;
  };

  ProcessorBase (
    ProcessorBaseDependencies dependencies,
    utils::Utf8String         name = { u8"ProcessorBase" })
      : dependencies_ (dependencies), name_ (std::move (name))
  {
  }

  ~ProcessorBase () override = default;

  /**
   * @brief Set a custom name to be used in the DSP graph.
   */
  void set_name (const utils::Utf8String &name);

  void add_input_port (const dsp::PortUuidReference &uuid);
  void add_output_port (const dsp::PortUuidReference &uuid);
  void add_parameter (const dsp::ProcessorParameterUuidReference &uuid);

  auto &get_input_ports () const { return input_ports_; }
  auto &get_output_ports () const { return output_ports_; }
  auto &get_parameters () const { return params_; }

  // ============================================================================
  // IProcessable Interface
  // ============================================================================

  utils::Utf8String get_node_name () const final { return name_; }

  /**
   * @brief Calls custom_process_block() internally after processing all the
   * parameters.
   *
   * This is done this way to make sure all parameters are processed by the
   * owning processor as intended and to avoid mistakes by leaving it up to
   * processor implementations to do.
   */
  void process_block (EngineProcessTimeInfo time_nfo) final;
  void
  prepare_for_processing (sample_rate_t sample_rate, nframes_t max_block_length)
    override;
  void release_resources () override;

  // ============================================================================

protected:
  /**
   * @brief Custom processor logic after processing all owned parameters.
   *
   * By default, this does passthrough to same-type ports.
   */
  virtual void custom_process_block (EngineProcessTimeInfo time_nfo);

  virtual void custom_prepare_for_processing (
    sample_rate_t sample_rate,
    nframes_t     max_block_length)
  {
  }

  auto dependencies () const { return dependencies_; }

private:
  static constexpr auto kProcessorNameKey = "processorName"sv;
  static constexpr auto kInputPortsKey = "inputPorts"sv;
  static constexpr auto kOutputPortsKey = "outputPorts"sv;
  static constexpr auto kParametersKey = "parameters"sv;
  friend void           to_json (nlohmann::json &j, const ProcessorBase &p)
  {
    j[kProcessorNameKey] = p.name_;
    j[kInputPortsKey] = p.input_ports_;
    j[kOutputPortsKey] = p.output_ports_;
    j[kParametersKey] = p.params_;
  }
  friend void from_json (const nlohmann::json &j, ProcessorBase &p)
  {
    j.at (kProcessorNameKey).get_to (p.name_);
    p.input_ports_.clear ();
    for (const auto &input_port : j.at (kInputPortsKey))
      {
        auto port_ref = dsp::PortUuidReference{ p.dependencies_.port_registry_ };
        from_json (input_port, port_ref);
        p.input_ports_.emplace_back (std::move (port_ref));
      }
    p.output_ports_.clear ();
    for (const auto &output_port : j.at (kOutputPortsKey))
      {
        auto port_ref = dsp::PortUuidReference{ p.dependencies_.port_registry_ };
        from_json (output_port, port_ref);
        p.output_ports_.emplace_back (std::move (port_ref));
      }
    p.params_.clear ();
    for (const auto &param : j.at (kParametersKey))
      {
        auto param_ref = dsp::ProcessorParameterUuidReference{
          p.dependencies_.param_registry_
        };
        from_json (param, param_ref);
        p.params_.emplace_back (std::move (param_ref));
      }
  }

private:
  ProcessorBaseDependencies                         dependencies_;
  utils::Utf8String                                 name_;
  std::vector<dsp::PortUuidReference>               input_ports_;
  std::vector<dsp::PortUuidReference>               output_ports_;
  std::vector<dsp::ProcessorParameterUuidReference> params_;

  // Caches
  sample_rate_t sample_rate_{};
  nframes_t     max_block_length_{};
};

/**
 * @brief Helper class to insert nodes and connections pertaining to a
 * ProcessorBase instance to a graph.
 *
 * Since all processors use a common interface to get their input & output
 * ports, this removes the need to have custom per-processor logic, hence this
 * class is used.
 */
class ProcessorGraphBuilder
{
public:
  static void add_nodes (
    dsp::graph::Graph &graph,
    dsp::ITransport   &transport,
    ProcessorBase     &processor);
  static void
  add_connections (dsp::graph::Graph &graph, ProcessorBase &processor);
};
} // namespace zrythm::dsp
