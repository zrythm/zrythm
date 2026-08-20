// SPDX-FileCopyrightText: © 2018-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense
#include <algorithm>
#include <limits>
#include <optional>

#include "plugins/plugin.h"
#include "utils/enum_utils.h"
#include "utils/exceptions.h"
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

  // Mark the selected preset dirty on deliberate user edits (see
  // ProcessorParameter::setBaseValueByUser)
  QObject::connect (
    this, &ProcessorBase::parameterAdded, this,
    [this] (dsp::ProcessorParameter * param) {
      arm_preset_dirty_tracking_for (*param);
    });
}

void
Plugin::arm_preset_dirty_tracking_for (dsp::ProcessorParameter &param)
{
  const auto uid = param.get_unique_id ();
  if (
    uid == dsp::ProcessorParameter::UniqueId (kBypassParamUniqueId)
    || uid == dsp::ProcessorParameter::UniqueId (kGainParamUniqueId))
    return;
  auto &connection = preset_dirty_connections_[param.get_uuid ()];
  disconnect (connection);
  connection = connect (
    &param, &dsp::ProcessorParameter::baseValueEditedByUser, this,
    [this] (float) { set_preset_dirty (true); });
}

void
Plugin::set_bypass_id (dsp::ProcessorParameter::Uuid id)
{
  bypass_id_ = id;
  arm_bypassed_relay ();
}

int
Plugin::presetIndex () const
{
  if (!selected_preset_id_.has_value ())
    return -1;

  const auto entries = presetEntries ();
  const auto it =
    std::ranges::find (entries, *selected_preset_id_, &PresetEntry::id);
  if (it == entries.end ())
    return -1;
  return static_cast<int> (std::distance (entries.begin (), it));
}

void
Plugin::setPresetIndex (int index)
{
  if (index < 0)
    {
      // Clearing the selection is host-side display state only; plugin
      // formats have no "unselect", so implementations are not notified
      if (!selected_preset_id_.has_value ())
        return;
      selected_preset_id_.reset ();
      set_preset_dirty (false);
      Q_EMIT presetIndexChanged (-1);
      return;
    }

  const auto entries = presetEntries ();
  if (index >= static_cast<int> (entries.size ()))
    {
      z_warning (
        "Refusing to select preset index {}: plugin has {} presets", index,
        entries.size ());
      return;
    }

  const auto new_id = entries[static_cast<size_t> (index)].id;
  const bool changed =
    !selected_preset_id_.has_value () || *selected_preset_id_ != new_id;

  // Re-selecting the current preset re-applies it (revert to the preset's
  // state)
  selected_preset_id_ = new_id;
  // Pass an owned copy: implementations may refresh their entry list while
  // applying, which could mutate the selection state
  apply_preset_impl (new_id);
  set_preset_dirty (false);
  if (changed)
    {
      // Re-resolve: applying may have rebuilt the entry list
      Q_EMIT presetIndexChanged (presetIndex ());
    }
}

void
Plugin::update_selected_preset_from_backend (PresetId id)
{
  if (selected_preset_id_.has_value () && *selected_preset_id_ == id)
    {
      // Same preset: nothing to do. The dirty flag must stay as-is because
      // plugin-side parameter edits can arrive alongside notifications that
      // re-report the unchanged current program
      return;
    }

  selected_preset_id_ = std::move (id);
  set_preset_dirty (false);
  Q_EMIT presetIndexChanged (presetIndex ());
}

void
Plugin::notify_presets_rebuilt ()
{
  Q_EMIT presetsChanged ();
  Q_EMIT presetIndexChanged (presetIndex ());
}

void
Plugin::set_preset_dirty (bool dirty)
{
  if (!selected_preset_id_.has_value ())
    dirty = false;
  if (preset_dirty_ == dirty)
    return;

  preset_dirty_ = dirty;
  Q_EMIT presetDirtyChanged (dirty);
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
    dsp::ProcessorParameter::UniqueId (kBypassParamUniqueId),
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
    dsp::ProcessorParameter::UniqueId (kGainParamUniqueId),
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
  if (p.selected_preset_id_.has_value ())
    {
      if (const auto * index = std::get_if<int> (&*p.selected_preset_id_))
        {
          j[Plugin::kPresetKey] = *index;
        }
      else
        {
          j[Plugin::kPresetKey] = utils::Utf8String::from_qstring (
            std::get<QString> (*p.selected_preset_id_));
        }
    }
  if (p.preset_dirty_)
    {
      j[Plugin::kPresetDirtyKey] = true;
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
  p.selected_preset_id_.reset ();
  p.preset_dirty_ = false;
  if (j.contains (Plugin::kPresetKey))
    {
      const auto &preset_val = j[Plugin::kPresetKey];
      if (preset_val.is_number_integer ())
        {
          const auto preset_index = preset_val.get<int64_t> ();
          if (
            preset_index < 0 || preset_index > std::numeric_limits<int>::max ())
            {
              throw utils::exceptions::ZrythmException (
                fmt::format (
                  "Invalid preset index {} in project file", preset_index));
            }
          p.selected_preset_id_ = static_cast<int> (preset_index);
        }
      else if (preset_val.is_string ())
        {
          p.selected_preset_id_ =
            preset_val.get<utils::Utf8String> ().to_qstring ();
        }
      else
        {
          throw utils::exceptions::ZrythmException (
            fmt::format (
              "Invalid preset value in project file: expected integer or "
              "string, got {}",
              preset_val.type_name ()));
        }
    }
  if (j.contains (Plugin::kPresetDirtyKey))
    {
      const auto &dirty_val = j[Plugin::kPresetDirtyKey];
      if (!dirty_val.is_boolean ())
        {
          throw utils::exceptions::ZrythmException (
            fmt::format (
              "Invalid presetDirty value in project file: expected boolean, "
              "got {}",
              dirty_val.type_name ()));
        }
      // Dirty state requires a selection (the invariant set_preset_dirty
      // enforces at runtime)
      p.preset_dirty_ =
        p.selected_preset_id_.has_value () && dirty_val.get<bool> ();
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
      auto *     param = param_ref.get ();
      const auto param_uid = param->get_unique_id ();
      if (
        param_uid
        == dsp::ProcessorParameter::UniqueId (Plugin::kGainParamUniqueId))
        {
          p.gain_id_ = param->get_uuid ();
        }
      else if (
        param_uid
        == dsp::ProcessorParameter::UniqueId (Plugin::kBypassParamUniqueId))
        {
          p.set_bypass_id (param->get_uuid ());
        }

      // Params restored from JSON don't emit parameterAdded (their objects
      // are not resolvable during ProcessorBase deserialization): arm
      // preset-dirty tracking for them here. Params newly created during
      // set_configuration above were already armed via parameterAdded;
      // re-arming replaces the previous connection
      p.arm_preset_dirty_tracking_for (*param);
    }
}

Plugin::~Plugin () = default;

} // namespace zrythm::plugins
