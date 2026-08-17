// SPDX-FileCopyrightText: © 2018-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense
#include <optional>

#include "plugins/plugin.h"
#include "utils/enum_utils.h"
#include "utils/logger.h"
#include "utils/serialization.h"
#include "utils/tracy.h"

#include <juce_audio_basics/juce_audio_basics.h>

namespace zrythm::plugins
{

Plugin::Plugin (utils::IObjectRegistry &registry, QObject * parent)
    : dsp::ProcessorBase (registry, u8"Plugin", parent), self_guard_ (this),
      load_measurer_ (std::make_unique<juce::AudioProcessLoadMeasurer> ())
{
  QObject::connect (
    this, &Plugin::instantiationFinished, this, [this] (bool successful) {
      instantiation_status_ =
        successful ? InstantiationStatus::Successful : InstantiationStatus::Failed;
      Q_EMIT instantiationStatusChanged (instantiation_status_);
    });

  QObject::connect (
    this, &Plugin::uiVisibleChanged, this, &Plugin::on_ui_visibility_changed);
}

void
Plugin::set_bypass_id (dsp::ProcessorParameter::Uuid id)
{
  bypass_id_ = id;
  arm_bypassed_relay ();
}

void
Plugin::arm_bypassed_relay ()
{
  disconnect (bypassed_relay_connection_);
  if (!bypass_id_.has_value ())
    return;
  auto * bypass = bypassParameter ();
  bypassed_relay_connection_ = connect (
    bypass, &dsp::ProcessorParameter::baseValueChanged, this,
    [this, bypass] (float value) {
      Q_EMIT bypassedChanged (bypass->range ().isToggled (value));
    });
}

dsp::ProcessorParameter *
Plugin::bypassParameter () const
{
  return &utils::get_typed<dsp::ProcessorParameter> (
    registry (), bypass_id_.value ());
}

dsp::ProcessorParameter *
Plugin::gainParameter () const
{
  return &utils::get_typed<dsp::ProcessorParameter> (
    registry (), gain_id_.value ());
}

dsp::ProcessorParameterUuidReference
Plugin::generate_default_bypass_param () const
{
  auto bypass_id = utils::create_object<dsp::ProcessorParameter> (
    registry (), registry (),
    dsp::ProcessorParameter::UniqueId (u8"/zrythm-bypass"),
    dsp::ParameterRange{ dsp::ParameterRange::Type::Toggle, 0.f, 1.f, 0.f, 0.f },
    utils::Utf8String::from_qstring (QObject::tr ("Bypass")));
  bypass_id.get ()->set_description (
    utils::Utf8String::from_qstring (
      QObject::tr ("Enables or disables the plugin")));
  return bypass_id;
}

dsp::ProcessorParameterUuidReference
Plugin::generate_default_gain_param () const
{
  auto gain_id = utils::create_object<dsp::ProcessorParameter> (
    registry (), registry (),
    dsp::ProcessorParameter::UniqueId (u8"/zrythm-gain"),
    dsp::ParameterRange{
      dsp::ParameterRange::Type::GainAmplitude, 0.f, 8.f, 0.f, 1.f },
    utils::Utf8String::from_qstring (QObject::tr ("Gain")));
  return gain_id;
}

void
Plugin::set_configuration (const PluginConfiguration &setting)
{
  assert (!set_configuration_called_);
  set_configuration_called_ = true;

  configuration_ = utils::clone_unique (setting);
  const auto &descr = configuration_->descr_;

  z_debug (
    "setting setting for plugin: {} ({})", descr->name_,
    ENUM_NAME (descr->protocol_));

  z_debug ("{} ({})", get_name (), ENUM_NAME (get_protocol ()));

  set_name (get_name ());

  /* If ports/params were already restored (e.g., from JSON deserialization),
   * tell handlers to skip generation and only reinitialize the underlying
   * plugin instance. */
  const bool generate_new =
    get_all_input_ports ().empty () && get_all_output_ports ().empty ();
  Q_EMIT configurationChanged (configuration_.get (), generate_new);

  // If the UI was marked visible before loading completed (e.g., during
  // project deserialization), queue the UI restore so that callers finish
  // setting up before native windows are created
  if (uiVisible ())
    {
      QMetaObject::invokeMethod (
        this, [this] () { on_ui_visibility_changed (); }, Qt::QueuedConnection);
    }
}

// ============================================================================
// IProcessable Interface
// ============================================================================

void
Plugin::custom_prepare_for_processing (
  const dsp::graph::GraphNode * node,
  units::sample_rate_t          sample_rate,
  units::sample_u32_t           max_block_length)
{
  init_param_caches ();
  param_sync_.prepare (get_parameters ().size ());
  load_measurer_->reset (
    static_cast<double> (sample_rate.in (units::sample_rate)),
    static_cast<int> (max_block_length.in (units::samples)));
  const auto latency_before = latencySamples ();
  prepare_plugin_for_processing (sample_rate, max_block_length);
  if (latencySamples () != latency_before)
    {
      post_main_thread_action ([this] {
        Q_EMIT latencySamplesChanged (latencySamples ());
      });
    }
}

void
Plugin::custom_process_block (
  const dsp::graph::ProcessBlockInfo time_nfo,
  const dsp::ITransport             &transport,
  const dsp::TempoMap               &tempo_map) noexcept
{
  ZoneScopedN ("Plugin process");
  ZoneText (get_name ().c_str (), strlen (get_name ().c_str ()));

  // Measure every path (including passthrough) against the actual block
  // length so the reported load reflects the current block and decays
  // while bypassed. ProcessorBase can rewrite a bad block to 0 frames;
  // measuring that would divide by a zero budget. The measurer's internal
  // spinlock is uncontended: the audio thread is its only writer
  std::optional<juce::AudioProcessLoadMeasurer::ScopedTimer> scoped_timer;
  if (time_nfo.nframes_ > units::samples (0)) [[likely]]
    {
      scoped_timer.emplace (
        *load_measurer_,
        static_cast<int> (time_nfo.nframes_.in (units::samples)));
    }

  if (instantiation_failed_)
    return;

  if (!currently_enabled_rt ())
    {
      process_passthrough_impl (time_nfo, transport, tempo_map);
      return;
    }

  process_impl (time_nfo, transport, tempo_map);
}

void
Plugin::custom_release_resources ()
{
  param_sync_.entries.clear ();
  load_measurer_->reset ();
  release_resources_impl ();
}

void
Plugin::process_passthrough_impl (
  const dsp::graph::ProcessBlockInfo time_nfo,
  const dsp::ITransport             &transport,
  const dsp::TempoMap               &tempo_map) noexcept
{
  // ProcessorBase's processing logic does passthrough
  dsp::ProcessorBase::custom_process_block (time_nfo, transport, tempo_map);
}

// ============================================================================

void
Plugin::init_param_caches ()
{
  audio_in_ports_.clear ();
  cv_in_ports_.clear ();
  audio_out_ports_.clear ();
  midi_in_ports_.clear ();
  midi_out_ports_.clear ();

  for (const auto &port_ref : get_attached_input_ports ())
    {
      auto * port = port_ref.get ();
      if (auto * audio = qobject_cast<dsp::AudioPort *> (port))
        audio_in_ports_.push_back (audio);
      else if (auto * cv = qobject_cast<dsp::CVPort *> (port))
        cv_in_ports_.push_back (cv);
      else if (auto * midi = qobject_cast<dsp::MidiPort *> (port))
        midi_in_ports_.push_back (midi);
    }

  for (const auto &port_ref : get_attached_output_ports ())
    {
      auto * port = port_ref.get ();
      if (auto * audio = qobject_cast<dsp::AudioPort *> (port))
        audio_out_ports_.push_back (audio);
      else if (auto * midi = qobject_cast<dsp::MidiPort *> (port))
        midi_out_ports_.push_back (midi);
    }

  bypass_param_rt_ = bypass_id_.has_value () ? bypassParameter () : nullptr;

  arm_bypassed_relay ();
}

std::string
Plugin::save_state () const
{
  return save_state_impl ();
}

bool
Plugin::load_state (const std::string &base64_state)
{
  return load_state_impl (base64_state);
}

bool
Plugin::bypassed () const
{
  // No bypass parameter: possible transiently during deserialization
  // (visible_ is restored before the parameter IDs are re-resolved)
  if (!bypass_id_.has_value ())
    return false;
  const auto * bypass = bypassParameter ();
  return bypass->range ().isToggled (bypass->baseValue ());
}

void
Plugin::setBypassed (bool bypassed)
{
  if (!bypass_id_.has_value ())
    {
      z_warning (
        "Plugin '{}': no bypass parameter registered", get_node_name ());
      return;
    }
  auto * bypass = bypassParameter ();
  bypass->setBaseValue (bypassed ? 1.f : 0.f);
}

void
Plugin::switchAbState ()
{
  const auto current = save_state ();
  if (current.empty ())
    {
      z_warning ("A/B: plugin '{}' has no state to save", get_node_name ());
      return;
    }

  auto &active_slot = ab_b_active_ ? ab_state_b_ : ab_state_a_;
  auto &other_slot = ab_b_active_ ? ab_state_a_ : ab_state_b_;
  active_slot = current;
  if (other_slot.empty ())
    other_slot = current;
  else if (!load_state (other_slot))
    {
      // The plugin rejected the other slot's state: stay on the current
      // slot or the UI would show a state that isn't playing
      z_warning (
        "A/B: plugin '{}' failed to load the other slot's state; staying on "
        "slot {}",
        get_node_name (), ab_b_active_ ? "B" : "A");
      return;
    }
  ab_b_active_ = !ab_b_active_;
  Q_EMIT abActiveChanged (ab_b_active_);
}

void
Plugin::set_param_pending_from_plugin (size_t index, float value) noexcept
{
  if (index >= param_sync_.entries.size ()) [[unlikely]]
    return;
  param_sync_.entries[index].pending_value.store (
    value, std::memory_order_release);
  param_sync_.dirty.store (true, std::memory_order_release);
}

void
Plugin::flush_plugin_values ()
{
  if (!param_sync_.dirty.exchange (false, std::memory_order_acq_rel))
    return;

  for (
    size_t i = 0;
    i < param_sync_.entries.size () && i < get_parameters ().size (); ++i)
    {
      auto &entry = param_sync_.entries[i];
      float val = entry.pending_value.exchange (-1.f, std::memory_order_acq_rel);
      if (val >= 0.f)
        {
          const auto &param_ref = get_parameters ()[i];
          auto *      param = param_ref.get ();
          param->setBaseValue (val);
        }
    }
}

double
Plugin::dspLoadPercentage () const
{
  return load_measurer_->getLoadAsPercentage ();
}

int
Plugin::latencySamples () const
{
  return static_cast<int> (get_single_playback_latency ().in (units::samples));
}

void
Plugin::notify_latency_changed () noexcept
{
  post_main_thread_action ([this] {
    if (main_thread_callbacks_.latency_recalc_)
      {
        main_thread_callbacks_.latency_recalc_ ();
      }
    Q_EMIT latencySamplesChanged (latencySamples ());
  });
}

void
to_json (nlohmann::json &j, const Plugin &p)
{
  to_json (j, static_cast<const Plugin::UuidIdentifiableObject &> (p));
  to_json (j, static_cast<const dsp::ProcessorBase &> (p));
  j[Plugin::kConfigurationKey] = p.configuration_;
  if (p.program_index_.has_value ())
    {
      j[Plugin::kProgramIndexKey] = *p.program_index_;
    }
  j[Plugin::kProtocolKey] = p.get_protocol ();
  j[Plugin::kVisibleKey] = p.visible_;
}

void
from_json (const nlohmann::json &j, Plugin &p)
{
  from_json (j, static_cast<Plugin::UuidIdentifiableObject &> (p));
  from_json (j, static_cast<dsp::ProcessorBase &> (p));
  j.at (Plugin::kConfigurationKey).get_to (p.configuration_);
  if (j.contains (Plugin::kProgramIndexKey))
    {
      p.program_index_ = j[Plugin::kProgramIndexKey].get<int> ();
    }
  j.at (Plugin::kVisibleKey).get_to (p.visible_);

  if (p.configuration_)
    {
      p.set_configuration (*p.configuration_);
    }

  p.gain_id_.reset ();
  p.bypass_id_.reset ();
  for (const auto &param_ref : p.get_parameters ())
    {
      const auto * param = param_ref.get ();
      if (
        param->get_unique_id ()
        == dsp::ProcessorParameter::UniqueId (u8"/zrythm-gain"))
        {
          p.gain_id_ = param->get_uuid ();
        }
      else if (
        param->get_unique_id ()
        == dsp::ProcessorParameter::UniqueId (u8"/zrythm-bypass"))
        {
          p.set_bypass_id (param->get_uuid ());
        }

      if (p.gain_id_.has_value () && p.bypass_id_.has_value ())
        break;
    }
}

Plugin::~Plugin () = default;

} // namespace zrythm::plugins
