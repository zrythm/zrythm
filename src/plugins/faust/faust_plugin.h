// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "dsp/poly_voice_manager.h"
#include "plugins/faust/faust_controls.h"
#include "plugins/faust/faust_registry.h"
#include "plugins/faust/faust_synth_voice.h"
#include "plugins/internal_plugin_base.h"

#include <juce_audio_basics/juce_audio_basics.h>

namespace zrythm::plugins
{

/**
 * @brief A plugin hosting a bundled Faust dsp compiled into Zrythm.
 *
 * Effects drive a single dsp instance; instruments drive a
 * dsp::PolyVoiceManager with one voice per dsp instance.
 *
 * When the configured descriptor does not match a bundled Faust plugin, this
 * acts as a pass-through.
 */
class FaustPlugin : public InternalPluginBase
{
  Q_OBJECT
  QML_ELEMENT
  QML_UNCREATABLE ("")

public:
  FaustPlugin (utils::IObjectRegistry &registry, QObject * parent = nullptr);
  ~FaustPlugin () override;

protected:
  void prepare_plugin_for_processing (
    units::sample_rate_t sample_rate,
    units::sample_u32_t  max_block_length) override;

  void process_impl (
    dsp::graph::ProcessBlockInfo time_info,
    const dsp::ITransport &,
    const dsp::TempoMap &) noexcept [[clang::nonblocking]] override;

  std::string save_state_impl () const override { return {}; }
  bool        load_state_impl (const std::string &) override { return true; }

protected Q_SLOTS:
  void on_configuration_changed (
    PluginConfiguration * configuration,
    bool                  generateNewPluginPortsAndParams) override;

private:
  void setup_faust (const faust::FaustPluginInfo &info, bool create_ports);

  void create_ports_and_params (bool instrument, int num_ins, int num_outs);

  void add_faust_param (const faust::FaustControl &control);

  /**
   * @brief Rebuilds the parameter-index → Faust-control-index mapping by
   * matching parameter unique IDs against control paths.
   *
   * Works for both freshly created parameters (created with the control path
   * as their unique ID) and parameters restored from a project (which
   * persisted those same unique IDs). Must be called after parameters exist.
   */
  void rebuild_param_mapping ();

  /**
   * @brief Writes all mapped parameter values to the Faust control zones.
   *
   * Called after rebuild_param_mapping() so restored parameter values take
   * effect without requiring a parameter change.
   */
  void push_all_params_to_zones ();

  /** Writes changed Zrythm parameter values to the Faust control zones. */
  void sync_params_to_zones () noexcept [[clang::nonblocking]];

  /**
   * @brief Writes a real (denormalized) value to the given control: to the
   * zone for effects, or to every voice for instruments.
   */
  void write_control_value (size_t control_index, float real_value) noexcept
    [[clang::nonblocking]];

  void process_fx (dsp::graph::ProcessBlockInfo time_info) noexcept
    [[clang::nonblocking]];

  void process_instrument (dsp::graph::ProcessBlockInfo time_info) noexcept
    [[clang::nonblocking]];

private:
  static constexpr size_t kNoControl = std::numeric_limits<size_t>::max ();

  const faust::FaustPluginInfo * info_{};
  bool                           is_instrument_ = false;

  /** The dsp instance (FX mode / instrument prototype). */
  std::unique_ptr<faust::dsp> dsp_;

  /** Controls of the dsp instance (FX mode / instrument prototype). */
  std::vector<faust::FaustControl> controls_;

  /**
   * @brief Parameter index (into get_parameters()) → control index (into
   * controls_), or kNoControl for unmapped parameters (bypass/gain).
   */
  std::vector<size_t> param_to_control_;

  // ============================================================================
  // Instrument mode
  // ============================================================================

  dsp::PolyVoiceManager voice_manager_;

  /**
   * @brief Release-tail configuration for instrument voices, read from the
   * dsp's metadata (`zrythm_release_seconds`, `zrythm_silence_threshold_db`)
   * in setup_faust() and applied to each voice in
   * prepare_plugin_for_processing().
   *
   * Per-voice envelope follower settings determine when a releasing voice is
   * considered silent and freed. Defaults cover most synths; reverb-heavy
   * instruments should override `zrythm_release_seconds` to a larger value.
   */
  float voice_release_seconds_ = 2.f;
  float voice_silence_threshold_db_ = -80.f;

  /** Scratch buffer the voices render into before copying to the ports. */
  juce::AudioBuffer<float> synth_buffer_;

  /** Empty buffer passed to the voice manager when there is no MIDI input. */
  dsp::MidiEventBuffer empty_midi_buffer_;

  // ============================================================================
  // Processing scratch (allocated at prepare time)
  // ============================================================================

  std::vector<FAUSTFLOAT *> in_ptrs_;
  std::vector<FAUSTFLOAT *> out_ptrs_;
};

} // namespace zrythm::plugins
