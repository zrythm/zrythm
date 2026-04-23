// SPDX-FileCopyrightText: © 2025-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

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
  /**
   * @brief Tracks parameter value changes across processing cycles.
   *
   * Built during the existing parameter loop in process_block() — no extra
   * passes. prev_values_ starts at -1.f (sentinel, since normalized values
   * are [0,1]) so the first cycle naturally marks all params as changed.
   *
   * Only valid to read during custom_process_block(); cleared afterward.
   */
  struct ParameterChangeTracker
  {
    struct Change
    {
      size_t                    index{};
      float                     base_value{};
      float                     automated_value{};
      float                     modulated_value{};
      dsp::ProcessorParameter * param{};
    };

    /** Returns the changes accumulated during the current cycle. */
    const auto &changes () const { return changes_; }

  private:
    friend class ProcessorBase;

    /** Changes accumulated during the current cycle's parameter loop. */
    std::vector<Change> changes_;

    /** Previous cycle's modulated values, parallel to live_params_. */
    std::vector<float> prev_values_;

    /** Resets state for a new parameter count (called from
     * prepare_for_processing()). */
    void prepare (size_t count);

    /** Compares current modulated value against previous cycle; records a
     * Change if different. Called once per parameter per process_block(). */
    void record_if_changed (size_t i, dsp::ProcessorParameter * param)
    {
      float modulated = param->currentValue ();
      if (!utils::math::floats_equal (prev_values_[i], modulated))
        {
          changes_.push_back (
            { i, param->baseValue (), param->valueAfterAutomationApplied (),
              modulated, param });
          prev_values_[i] = modulated;
        }
    }

    /** Clears the change list after custom_process_block() returns. */
    void clear () { changes_.clear (); }
  };

private:
  struct BaseProcessingCache
  {
    units::sample_rate_t sample_rate_;
    units::sample_u32_t  max_block_length_{};

    std::vector<dsp::ProcessorParameter *> live_params_;
    std::vector<dsp::PortPtrVariant>       live_input_ports_;
    std::vector<dsp::PortPtrVariant>       live_output_ports_;

    ParameterChangeTracker change_tracker_;

    /**
     * @brief True while inside process_block(), false otherwise.
     *
     * Used to enforce that change_tracker_ is only accessed during
     * custom_process_block(). Set at the start of process_block() and
     * cleared before returning.
     */
    bool is_processing_ = false;
  };

public:
  struct ProcessorBaseDependencies
  {
    dsp::PortRegistry               &port_registry_;
    dsp::ProcessorParameterRegistry &param_registry_;
  };

  ProcessorBase (
    ProcessorBaseDependencies dependencies,
    utils::Utf8String         name = { u8"ProcessorBase" });

  ~ProcessorBase () override;

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

  /**
   * @brief Returns the change tracker.
   *
   * Only valid to call during custom_process_block() (while is_processing_
   * is true).
   */
  const ParameterChangeTracker &change_tracker () const noexcept
  {
    assert (processing_caches_ && processing_caches_->is_processing_);
    return processing_caches_->change_tracker_;
  }

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
  void process_block (
    dsp::graph::EngineProcessTimeInfo time_nfo,
    const dsp::ITransport            &transport,
    const dsp::TempoMap              &tempo_map) noexcept final;
  void prepare_for_processing (
    const graph::GraphNode * node,
    units::sample_rate_t     sample_rate,
    units::sample_u32_t      max_block_length) final;
  void release_resources () final;

  // ============================================================================

protected:
  /**
   * @brief Custom processor logic after processing all owned parameters.
   *
   * By default, this does passthrough to same-type ports.
   */
  virtual void custom_process_block (
    dsp::graph::EngineProcessTimeInfo time_nfo,
    const dsp::ITransport            &transport,
    const dsp::TempoMap &tempo_map) noexcept [[clang::nonblocking]];

  virtual void custom_prepare_for_processing (
    const graph::GraphNode * node,
    units::sample_rate_t     sample_rate,
    units::sample_u32_t      max_block_length)
  {
  }

  virtual void custom_release_resources () { }

  auto dependencies () const { return dependencies_; }

private:
  static constexpr auto kProcessorNameKey = "processorName"sv;
  static constexpr auto kInputPortsKey = "inputPorts"sv;
  static constexpr auto kOutputPortsKey = "outputPorts"sv;
  static constexpr auto kParametersKey = "parameters"sv;
  friend void           to_json (nlohmann::json &j, const ProcessorBase &p);
  friend void           from_json (const nlohmann::json &j, ProcessorBase &p);

private:
  ProcessorBaseDependencies                         dependencies_;
  utils::Utf8String                                 name_;
  std::vector<dsp::PortUuidReference>               input_ports_;
  std::vector<dsp::PortUuidReference>               output_ports_;
  std::vector<dsp::ProcessorParameterUuidReference> params_;

  // Caches
  std::unique_ptr<BaseProcessingCache> processing_caches_;

  BOOST_DESCRIBE_CLASS (ProcessorBase, (), (), (), (name_))
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
  static void add_nodes (dsp::graph::Graph &graph, ProcessorBase &processor);
  static void
  add_connections (dsp::graph::Graph &graph, ProcessorBase &processor);
};
} // namespace zrythm::dsp
