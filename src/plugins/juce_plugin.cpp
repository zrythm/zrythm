// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <utility>

#include "utils/format_qt.h"

#include "plugins/juce_plugin.h"
#include "utils/float_ranges.h"
#include "utils/io_utils.h"

#include <juce_audio_processors/juce_audio_processors.h>
#if defined(__has_feature) && __has_feature(realtime_sanitizer)
#  include <sanitizer/rtsan_interface.h>
#endif

namespace zrythm::plugins
{

JucePlugin::JucePlugin (
  dsp::ProcessorBase::ProcessorBaseDependencies dependencies,
  CreatePluginInstanceAsyncFunc          create_plugin_instance_async_func,
  std::function<units::sample_rate_t ()> sample_rate_provider,
  std::function<units::sample_u32_t ()>  buffer_size_provider,
  PluginHostWindowFactory                top_level_window_provider,
  QObject *                              parent)
    : Plugin (dependencies, parent),
      create_plugin_instance_async_func_ (
        std::move (create_plugin_instance_async_func)),
      sample_rate_provider_ (std::move (sample_rate_provider)),
      buffer_size_provider_ (std::move (buffer_size_provider)),
      top_level_window_provider_ (std::move (top_level_window_provider))
{
  // Connect to configuration changes
  connect (
    this, &Plugin::configurationChanged, this,
    &JucePlugin::on_configuration_changed);

  // Connect to UI visibility changes
  connect (
    this, &Plugin::uiVisibleChanged, this,
    &JucePlugin::on_ui_visibility_changed);

  auto bypass_ref = generate_default_bypass_param ();
  add_parameter (bypass_ref);
  bypass_id_ = bypass_ref.id ();
  auto gain_ref = generate_default_gain_param ();
  add_parameter (gain_ref);
  gain_id_ = gain_ref.id ();
}

JucePlugin::~JucePlugin ()
{
  if (editor_)
    {
      hide_editor ();
    }

  if (juce_plugin_)
    {
      juce_plugin_->releaseResources ();
    }
}

void
JucePlugin::on_configuration_changed (
  PluginConfiguration * configuration,
  bool                  generateNewPluginPortsAndParams)
{
  // Reinitialize plugin with new configuration
  if (juce_plugin_)
    {
      juce_plugin_->releaseResources ();
      juce_plugin_.reset ();
      parameter_mappings_.clear ();
      juce_param_index_to_mapping_.clear ();
      zrythm_param_index_to_mapping_.clear ();
    }

  initialize_juce_plugin_async (generateNewPluginPortsAndParams);
}

void
JucePlugin::on_ui_visibility_changed ()
{
  if (uiVisible () && !editor_visible_)
    {
      show_editor ();
    }
  else if (!uiVisible () && editor_visible_)
    {
      hide_editor ();
    }
}

void
JucePlugin::initialize_juce_plugin_async (bool generateNewPluginPortsAndParams)
{
  if ((configuration () == nullptr) || plugin_loading_)
    return;

  plugin_loading_ = true;

  const auto &descriptor = *configuration ()->descr_;

  // Create plugin description from descriptor
  auto plugin_desc = descriptor.to_juce_description ();

  // Get current sample rate and buffer size
  const double sample_rate = sample_rate_provider_ ().in (units::sample_rate);
  const int    buffer_size = buffer_size_provider_ ().in<int> (units::samples);

  // Create plugin instance asynchronously
  create_plugin_instance_async_func_ (
    *plugin_desc, sample_rate, buffer_size,
    [this, generateNewPluginPortsAndParams] (
      std::unique_ptr<juce::AudioPluginInstance> instance,
      const juce::String                        &error) {
      plugin_loading_ = false;

      if (!instance)
        {
          instantiation_failed_ = true;
          z_warning (
            "Failed to create JUCE plugin instance: {}", error.toStdString ());
          Q_EMIT instantiationFinished (
            false, QString::fromStdString (error.toStdString ()));
          return;
        }

      juce_plugin_ = std::move (instance);

      try
        {
          if (generateNewPluginPortsAndParams)
            {
              create_ports_from_juce_plugin ();
            }

          // Apply any pending state BEFORE building parameter mappings, so that
          // JUCE parameter change listeners (attached by
          // create_parameters_from_juce_plugin) don't fire during state
          // restoration and overwrite deserialized Zrythm values.
          if (state_to_apply_.has_value ())
            {
              z_debug (
                "JUCE: applying saved state ({} bytes)",
                state_to_apply_->getSize ());
              juce_plugin_->setStateInformation (
                state_to_apply_->getData (),
                static_cast<int> (state_to_apply_->getSize ()));
              state_to_apply_.reset ();
            }
          else
            {
              z_debug ("JUCE: no saved state to apply");
            }

          // Always build parameter mappings: when deserializing, this matches
          // existing Zrythm params to JUCE params instead of creating new ones.
          create_parameters_from_juce_plugin ();

          if (generateNewPluginPortsAndParams)
            {
              // Fresh plugin: sync JUCE's current values (after
              // setStateInformation) to Zrythm params directly — we're on the
              // message thread.
              for (const auto &mapping : parameter_mappings_)
                {
                  if (mapping.juce_param && mapping.zrythm_param)
                    mapping.zrythm_param->setBaseValue (
                      mapping.juce_param->getValue ());
                }
            }
          else
            {
              // Deserialization: the state blob is the authoritative source.
              // Read back JUCE parameter values (which reflect the loaded
              // state) and update Zrythm's baseValues to match.
              int updated = 0;
              for (const auto &mapping : parameter_mappings_)
                {
                  if (mapping.juce_param && mapping.zrythm_param)
                    {
                      const auto old_base = mapping.zrythm_param->baseValue ();
                      const auto new_val = mapping.juce_param->getValue ();
                      if (!utils::math::floats_near (old_base, new_val, 0.001f))
                        {
                          mapping.zrythm_param->setBaseValue (new_val);
                          ++updated;
                          z_trace (
                            "JUCE: state-load updated param '{}' "
                            "old={:.4f} new={:.4f}",
                            mapping.zrythm_param->label (), old_base, new_val);
                        }
                    }
                }
              z_debug ("JUCE: state-load read back {} params", updated);
            }

          juce_initialized_ = true;
          instantiation_failed_ = false;

          z_info ("Successfully loaded JUCE plugin: {}", get_name ());
          Q_EMIT instantiationFinished (true, {});
        }
      catch (const std::exception &e)
        {
          instantiation_failed_ = true;
          z_warning ("Failed to initialize JUCE plugin: {}", e.what ());
          Q_EMIT instantiationFinished (false, e.what ());
        }
    });
}

void
JucePlugin::create_ports_from_juce_plugin ()
{
  if (!juce_plugin_)
    return;

  const auto &processor = *juce_plugin_;

  for (bool isInput : { true, false })
    {
      const int n = processor.getBusCount (isInput);
      for (int i = 0; i < n; ++i)
        {
          auto * bus = processor.getBus (isInput, i);
          if (!bus->isEnabled ())
            continue;

          const auto chSet = bus->getCurrentLayout ();
          const auto nCh = static_cast<uint8_t> (chSet.size ());

          dsp::AudioPort::Purpose purpose =
            bus->isMain ()
              ? dsp::AudioPort::Purpose::Main
              : dsp::AudioPort::Purpose::Sidechain;
          dsp::AudioPort::BusLayout layout = [chSet] {
            if (chSet == juce::AudioChannelSet::mono ())
              return dsp::AudioPort::BusLayout::Mono;
            if (chSet == juce::AudioChannelSet::stereo ())
              return dsp::AudioPort::BusLayout::Stereo;
            return dsp::AudioPort::BusLayout{};
          }();

          auto name = bus->getName ();

          auto port = dependencies ().port_registry_.create_object<dsp::AudioPort> (
            utils::Utf8String::from_juce_string (name),
            isInput ? dsp::PortFlow::Input : dsp::PortFlow::Output, layout, nCh,
            purpose);

          if (isInput)
            {
              add_input_port (port);
            }
          else
            {
              add_output_port (port);
            }
        }
    }

  // Create MIDI input port if plugin accepts MIDI
  if (processor.acceptsMidi ())
    {
      auto port_ref =
        dependencies ().port_registry_.create_object<dsp::MidiPort> (
          u8"midi_in", dsp::PortFlow::Input);
      add_input_port (port_ref);
    }

  // Create MIDI output port if plugin produces MIDI
  if (processor.producesMidi ())
    {
      auto port_ref =
        dependencies ().port_registry_.create_object<dsp::MidiPort> (
          u8"midi_out", dsp::PortFlow::Output);
      add_output_port (port_ref);
    }
}

void
JucePlugin::create_parameters_from_juce_plugin ()
{
  if (!juce_plugin_)
    return;

  auto &processor = *juce_plugin_;

  // Clear existing mappings
  parameter_mappings_.clear ();
  juce_param_index_to_mapping_.clear ();
  zrythm_param_index_to_mapping_.clear ();

  for (const auto &param : processor.getParameters ())
    {
      auto * hosted_param =
        processor.getHostedParameter (param->getParameterIndex ());
      const auto unique_id = dsp::ProcessorParameter::UniqueId (
        utils::Utf8String::from_juce_string (hosted_param->getParameterID ()));

      // Try to find an existing Zrythm param with the same unique ID
      // (happens during deserialization — params already restored from JSON)
      dsp::ProcessorParameter * zrythm_param = nullptr;
      size_t                    zrythm_param_index = 0;
      for (const auto &param_ref : get_parameters ())
        {
          auto * p = param_ref.get_object_as<dsp::ProcessorParameter> ();
          if (p->get_unique_id () == unique_id)
            {
              zrythm_param = p;
              break;
            }
          ++zrythm_param_index;
        }

      if (zrythm_param == nullptr)
        {
          // No existing param — create a new one
          dsp::ParameterRange range{
            dsp::ParameterRange::Type::Linear, 0.f, 1.f, 0.f,
            param->getDefaultValue ()
          };
          if (param->isBoolean ())
            {
              range = dsp::ParameterRange::make_toggle (
                param->getDefaultValue () > 0.5f);
            }
          else if (param->isDiscrete ())
            {
              range = dsp::ParameterRange{
                dsp::ParameterRange::Type::Integer, 0.f,
                static_cast<float> (param->getNumSteps ()) - 1.f, 0.f,
                param->getDefaultValue ()
              };
            }

          auto zrythm_param_ref =
            dependencies ().param_registry_.create_object<dsp::ProcessorParameter> (
              dependencies ().port_registry_, unique_id, range,
              utils::Utf8String::from_juce_string (param->getName (100)));
          add_parameter (zrythm_param_ref);
          zrythm_param =
            zrythm_param_ref.get_object_as<dsp::ProcessorParameter> ();
          zrythm_param->set_automatable (param->isAutomatable ());
          zrythm_param_index = get_parameters ().size () - 1;
        }

      // Store mapping (always, even for existing params)
      const int        juce_idx = param->getParameterIndex ();
      ParameterMapping mapping{
        .juce_param = param,
        .zrythm_param = zrythm_param,
        .juce_param_index = juce_idx,
        .zrythm_param_index = zrythm_param_index,
        .juce_param_listener = std::make_unique<JuceParamListener> (*this)
      };
      param->addListener (mapping.juce_param_listener.get ());
      const size_t mapping_index = parameter_mappings_.size ();
      parameter_mappings_.emplace_back (std::move (mapping));
      juce_param_index_to_mapping_[juce_idx] = mapping_index;
      zrythm_param_index_to_mapping_[zrythm_param_index] = mapping_index;
    }
}

units::sample_u32_t
JucePlugin::get_single_playback_latency () const
{
  if (juce_plugin_)
    {
      return units::samples (juce_plugin_->getLatencySamples ());
    }
  return units::samples (0);
}

void
JucePlugin::prepare_for_processing_impl (
  units::sample_rate_t sample_rate,
  units::sample_u32_t  max_block_length)
{
  if (!juce_plugin_ || !juce_initialized_)
    return;

  try
    {
      // Prepare JUCE plugin
      juce_plugin_->prepareToPlay (
        sample_rate.in (units::sample_rate),
        max_block_length.in<int> (units::samples));

      // Resize audio buffers
      const int num_inputs = juce_plugin_->getTotalNumInputChannels ();
      const int num_outputs = juce_plugin_->getTotalNumOutputChannels ();

      juce_audio_buffer_.setSize (
        std::max (num_inputs, num_outputs),
        max_block_length.in<int> (units::samples));

      // Setup channel pointers
      input_channels_.resize (num_inputs);
      output_channels_.resize (num_outputs);

      for (int i = 0; i < num_inputs; ++i)
        {
          input_channels_[i] = juce_audio_buffer_.getWritePointer (i);
        }

      for (int i = 0; i < num_outputs; ++i)
        {
          output_channels_[i] = juce_audio_buffer_.getWritePointer (i);
        }

      // Prepare MIDI buffer
      juce_midi_buffer_.ensureSize (4096);
    }
  catch (const std::exception &e)
    {
      z_error ("Failed to prepare JUCE plugin for processing: {}", e.what ());
    }
}

void
JucePlugin::process_impl (dsp::graph::EngineProcessTimeInfo time_info) noexcept
{
  if (!juce_plugin_ || !juce_initialized_)
    return;

  const int num_juce_plugin_inputs = juce_plugin_->getTotalNumInputChannels ();
  const int num_juce_plugin_outputs = juce_plugin_->getTotalNumOutputChannels ();

  const auto local_offset = time_info.local_offset_;
  const auto nframes = time_info.nframes_;

  // Copy input audio to JUCE buffer
  for (int ch = 0; ch < num_juce_plugin_inputs; ++ch)
    {
      int total_port_chs = 0;
      for (const auto &port : audio_in_ports_)
        {
          for (int i = 0; i < static_cast<int> (port->num_channels ()); ++i)
            {
              if (total_port_chs + i == ch)
                {
                  utils::float_ranges::copy (
                    { &input_channels_[ch][local_offset.in (units::samples)],
                      nframes.in (units::samples) },
                    { port->buffers ()->getReadPointer (
                        i, local_offset.in<int> (units::samples)),
                      nframes.in (units::samples) });
                }
            }
          total_port_chs += port->num_channels ();
        }
    }

  // Clear remaining JUCE buffers
  for (
    int ch = num_juce_plugin_inputs; ch < juce_audio_buffer_.getNumChannels ();
    ++ch)
    {
      utils::float_ranges::fill (
        { &juce_audio_buffer_.getWritePointer (
            ch)[local_offset.in (units::samples)],
          nframes.in (units::samples) },
        0.0f);
    }

  // Clear MIDI buffer
  juce_midi_buffer_.clear ();

  // Process MIDI input if available
  auto * midi_in = midi_in_port_;
  if ((midi_in != nullptr) && juce_plugin_->acceptsMidi ())
    {
      for (const auto &ev : midi_in->midi_events_.active_events_)
        {
          if (ev.time_ >= local_offset && ev.time_ < local_offset + nframes)
            {
              juce_midi_buffer_.addEvent (
                ev.raw_buffer_.data (),
                static_cast<int> (ev.raw_buffer_.size ()),
                ev.time_.in<int> (units::samples));
            }
        }
    }

  // Sync changed parameters to JUCE and process audio.
  //
  // JUCE's VST3Parameter::setValue() internally calls
  // MessageManager::isThisTheMessageThread() which acquires a std::mutex to
  // read the message thread ID. This is a JUCE bug — the check could be
  // lock-free (e.g. using an atomic). The mutex is held only for a thread-ID
  // comparison (~nanoseconds), so priority inversion risk is negligible, but
  // RTSan correctly flags it. processBlock() calls into third-party plugin
  // code which may also perform RTSan-flagged operations (allocations, mutex
  // locks, etc.) that we cannot control.
  juce::AudioBuffer<float> temp_buffer_with_offset (
    juce_audio_buffer_.getArrayOfWritePointers (),
    juce_audio_buffer_.getNumChannels (), local_offset.in<int> (units::samples),
    nframes.in<int> (units::samples));
  {
#if defined(__has_feature) && __has_feature(realtime_sanitizer)
    // TODO: add option to keep this enabled (we might want to test our own
    // plugins in the future)
    __rtsan::ScopedDisabler d;
#endif
    sync_changed_params_to_juce ();
    juce_plugin_->processBlock (temp_buffer_with_offset, juce_midi_buffer_);
  }

  // Copy output audio from JUCE buffer
  for (int ch = 0; ch < num_juce_plugin_outputs; ++ch)
    {
      int total_port_chs = 0;
      for (const auto &port : audio_out_ports_)
        {
          for (int i = 0; i < static_cast<int> (port->num_channels ()); ++i)
            {
              if (total_port_chs + i == ch)
                {
                  utils::float_ranges::copy (
                    { port->buffers ()->getWritePointer (
                        i, local_offset.in<int> (units::samples)),
                      nframes.in (units::samples) },
                    { &output_channels_[ch][local_offset.in (units::samples)],
                      nframes.in (units::samples) });
                }
            }
          total_port_chs += port->num_channels ();
        }
    }

  // Process MIDI output if available
  auto * midi_out = midi_out_port_;
  if ((midi_out != nullptr) && juce_plugin_->producesMidi ())
    {
      for (const auto &ev : juce_midi_buffer_)
        {
          midi_out->midi_events_.queued_events_.add_raw (
            ev.data, ev.numBytes, units::samples (ev.samplePosition));
        }
    }
}

void
JucePlugin::sync_changed_params_to_juce () noexcept
{
  if (!juce_plugin_)
    return;

  const auto &changes = change_tracker ().changes ();
  for (const auto &change : changes)
    {
      auto it = zrythm_param_index_to_mapping_.find (change.index);
      if (it == zrythm_param_index_to_mapping_.end ())
        continue;

      const auto &mapping = parameter_mappings_[it->second];
      if (!mapping.juce_param)
        continue;

      // Feedback prevention: skip if this value originated from the plugin
      auto &entry = param_sync_.entries[change.index];
      if (
        utils::math::floats_equal (
          change.modulated_value, entry.last_from_plugin))
        {
          entry.last_from_plugin = -1.f;
          continue;
        }

      mapping.juce_param->setValue (change.modulated_value);
    }
}

void
JucePlugin::JuceParamListener::parameterValueChanged (
  int   parameterIndex,
  float newValue)
{
  auto it = parent_.juce_param_index_to_mapping_.find (parameterIndex);
  if (it == parent_.juce_param_index_to_mapping_.end ())
    return;

  const auto &mapping = parent_.parameter_mappings_[it->second];
  if (!mapping.zrythm_param)
    return;

  const size_t param_index = mapping.zrythm_param_index;
  auto        &entry = parent_.param_sync_.entries[param_index];
  entry.pending_value.store (newValue, std::memory_order_release);

  if (
    juce::MessageManager::getInstanceWithoutCreating ()
      ->isThisTheMessageThread ())
    {
      // Message thread (e.g., UI interaction): flush immediately for
      // low-latency updates.
      parent_.flush_plugin_values ();
    }
  else
    {
      // Audio thread (during processBlock): set feedback guard for the
      // next cycle's sync_changed_params_to_juce().
      entry.last_from_plugin = newValue;
    }
}

void
JucePlugin::JuceParamListener::parameterGestureChanged (
  int  parameterIndex,
  bool gestureIsStarting)
{
}

void
JucePlugin::show_editor ()
{
  if (!juce_plugin_ || editor_visible_)
    return;

  z_debug ("creating editor for {}", get_name ());
  if (juce_plugin_->hasEditor ())
    {
      editor_ = std::unique_ptr<juce::AudioProcessorEditor> (
        juce_plugin_->createEditorIfNeeded ());
    }
  else
    {
      editor_ =
        std::make_unique<juce::GenericAudioProcessorEditor> (*juce_plugin_);
    }

  if (editor_)
    {
      editor_->setVisible (true);

      top_level_window_ = top_level_window_provider_ (*this);
      top_level_window_->setJuceComponentContentNonOwned (editor_.get ());

      editor_visible_ = true;
    }
}

void
JucePlugin::hide_editor ()
{
  if (!editor_)
    return;

  editor_->setVisible (false);
  top_level_window_.reset ();
  editor_.reset ();
  editor_visible_ = false;
}

std::string
JucePlugin::save_state_impl () const
{
  if (!juce_plugin_)
    return {};

  juce::MemoryBlock state_data;
  juce_plugin_->getStateInformation (state_data);
  return state_data.toBase64Encoding ().toStdString ();
}

void
JucePlugin::load_state_impl (const std::string &base64_state)
{
  state_to_apply_.emplace ();
  state_to_apply_->fromBase64Encoding (base64_state);
}

void
to_json (nlohmann::json &j, const JucePlugin &p)
{
  to_json (j, static_cast<const Plugin &> (p));
  auto state = p.save_state ();
  if (!state.empty ())
    j[JucePlugin::kStateKey] = std::move (state);
}

void
from_json (const nlohmann::json &j, JucePlugin &p)
{
  // State must be deserialized first, because the Plugin deserialization
  // may cause an instantiation
  if (j.contains (JucePlugin::kStateKey))
    p.load_state (j[JucePlugin::kStateKey].get<std::string> ());

  from_json (j, static_cast<Plugin &> (p));
}

} // namespace zrythm::plugins
