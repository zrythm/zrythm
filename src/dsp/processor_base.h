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
   * passes. At prepare time, previously unseen parameters are seeded from
   * their base value and already-tracked parameters keep their last known
   * value, so parameters whose values have not changed since then (e.g.,
   * values that originate from a plugin itself) are not reported.
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

    /** Last known modulated value per parameter. */
    struct TrackedValue
    {
      dsp::ProcessorParameter * param{};
      float                     modulated_value{};
    };

    /** Previously tracked values, parallel to live_params_. */
    std::vector<TrackedValue> prev_values_;

    /**
     * Reseeds tracking for the given parameters.
     *
     * Parameters seen for the first time are seeded from their base value
     * (the effective value before automation/modulation is applied);
     * already-tracked parameters keep their previous value so that edits
     * made since the last process cycle (or before a re-prepare) are still
     * detected.
     */
    void prepare (std::span<dsp::ProcessorParameter * const> params);

    /** Compares current modulated value against previous cycle; records a
     * Change if different. Called once per parameter per process_block(). */
    void record_if_changed (size_t i, dsp::ProcessorParameter * param)
    {
      assert (i < prev_values_.size ());
      float modulated = param->currentValue ();
      if (
        !utils::math::floats_equal (prev_values_[i].modulated_value, modulated))
        {
          changes_.push_back (
            { i, param->baseValue (), param->valueAfterAutomationApplied (),
              modulated, param });
          prev_values_[i].modulated_value = modulated;
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
  ProcessorBase (
    utils::IObjectRegistry &registry,
    utils::Utf8String       name = { u8"ProcessorBase" });

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
    return change_tracker_;
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
    dsp::graph::ProcessBlockInfo time_nfo,
    const dsp::ITransport       &transport,
    const dsp::TempoMap         &tempo_map) noexcept final;
  void prepare_for_processing_impl (
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
    dsp::graph::ProcessBlockInfo time_nfo,
    const dsp::ITransport       &transport,
    const dsp::TempoMap         &tempo_map) noexcept [[clang::nonblocking]];

  virtual void custom_prepare_for_processing (
    const graph::GraphNode * node,
    units::sample_rate_t     sample_rate,
    units::sample_u32_t      max_block_length)
  {
  }

  virtual void custom_release_resources () { }

  auto registry () const -> utils::IObjectRegistry & { return registry_; }

private:
  static constexpr auto kProcessorNameKey = "processorName"sv;
  static constexpr auto kInputPortsKey = "inputPorts"sv;
  static constexpr auto kOutputPortsKey = "outputPorts"sv;
  static constexpr auto kParametersKey = "parameters"sv;
  friend void           to_json (nlohmann::json &j, const ProcessorBase &p);
  friend void           from_json (const nlohmann::json &j, ProcessorBase &p);

private:
  utils::IObjectRegistry                           &registry_;
  utils::Utf8String                                 name_;
  std::vector<dsp::PortUuidReference>               input_ports_;
  std::vector<dsp::PortUuidReference>               output_ports_;
  std::vector<dsp::ProcessorParameterUuidReference> params_;

  // Caches
  std::unique_ptr<BaseProcessingCache> processing_caches_;

  /**
   * Parameter change tracking state.
   *
   * Kept outside processing_caches_ so that tracked values survive
   * re-preparation (processing_caches_ is recreated on each
   * prepare_for_processing()).
   */
  ParameterChangeTracker change_tracker_;

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
