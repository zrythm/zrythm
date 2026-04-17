// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <utility>

#include "plugins/juce_plugin.h"
#include "utils/dsp.h"
#include "utils/io_utils.h"

#include <juce_wrapper.h>
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
      juce_param_flush_timer_ (utils::make_qobject_unique<QTimer> (this)),
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

  // Connect param flush timer once; started when mappings are built
  connect (juce_param_flush_timer_.get (), &QTimer::timeout, this, [this] () {
    flush_juce_to_zrythm_queue ();
  });

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
      juce_param_flush_timer_->stop ();
      juce_plugin_->releaseResources ();
      juce_plugin_.reset ();
      parameter_mappings_.clear ();
      juce_param_index_to_mapping_.clear ();
      juce_to_zrythm_queue_.clear ();
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
              juce_plugin_->setStateInformation (
                state_to_apply_->getData (),
                static_cast<int> (state_to_apply_->getSize ()));
              state_to_apply_.reset ();
            }

          // Always build parameter mappings: when deserializing, this matches
          // existing Zrythm params to JUCE params instead of creating new ones.
          create_parameters_from_juce_plugin ();

          if (generateNewPluginPortsAndParams)
            {
              // Fresh plugin: queue all param indices so the initial JUCE values
              // (after setStateInformation) get flushed to Zrythm params.
              for (const auto &mapping : parameter_mappings_)
                {
                  juce_to_zrythm_queue_.push_back (mapping.juce_param_index);
                }
              flush_juce_to_zrythm_queue ();
            }
          else
            {
              // Deserialization: Zrythm params already have correct values from
              // JSON. Push base values to JUCE so the plugin starts with the
              // right state.
              for (const auto &mapping : parameter_mappings_)
                {
                  if (mapping.juce_param && mapping.zrythm_param)
                    {
                      mapping.juce_param->setValue (
                        mapping.zrythm_param->baseValue ());
                    }
                }
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
  juce_to_zrythm_queue_.clear ();

  for (const auto &param : processor.getParameters ())
    {
      auto * hosted_param =
        processor.getHostedParameter (param->getParameterIndex ());
      const auto unique_id = dsp::ProcessorParameter::UniqueId (
        utils::Utf8String::from_juce_string (hosted_param->getParameterID ()));

      // Try to find an existing Zrythm param with the same unique ID
      // (happens during deserialization — params already restored from JSON)
      dsp::ProcessorParameter * zrythm_param = nullptr;
      for (const auto &param_ref : get_parameters ())
        {
          auto * p = param_ref.get_object_as<dsp::ProcessorParameter> ();
          if (p->get_unique_id () == unique_id)
            {
              zrythm_param = p;
              break;
            }
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
        }

      // Store mapping (always, even for existing params)
      const int        juce_idx = param->getParameterIndex ();
      ParameterMapping mapping{
        .juce_param = param,
        .zrythm_param = zrythm_param,
        .juce_param_index = juce_idx,
        .juce_param_listener =
          std::make_unique<JuceParamListener> (juce_to_zrythm_queue_, *this)
      };
      param->addListener (mapping.juce_param_listener.get ());
      parameter_mappings_.emplace_back (std::move (mapping));
      juce_param_index_to_mapping_[juce_idx] = parameter_mappings_.size () - 1;
    }

  // Ensure the queue can hold all params changing simultaneously
  // (use 2x to avoid drops when the audio thread pushes faster than the
  // timer drains)
  juce_to_zrythm_queue_.reserve (parameter_mappings_.size () * 2);

  // Start timer to flush JUCE -> Zrythm param changes on the message thread
  juce_param_flush_timer_->start (std::chrono::milliseconds (20));
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
                    &input_channels_[ch][local_offset.in (units::samples)],
                    port->buffers ()->getReadPointer (
                      i, local_offset.in<int> (units::samples)),
                    nframes.in (units::samples));
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
        &juce_audio_buffer_.getWritePointer (
          ch)[local_offset.in (units::samples)],
        0.0f, nframes.in (units::samples));
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

  // Sync parameters to JUCE and process audio.
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
    sync_parameters_to_juce ();
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
                    port->buffers ()->getWritePointer (
                      i, local_offset.in<int> (units::samples)),
                    &output_channels_[ch][local_offset.in (units::samples)],
                    nframes.in (units::samples));
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
JucePlugin::sync_parameters_to_juce () noexcept
{
  if (!juce_plugin_)
    return;

  for (const auto &mapping : parameter_mappings_)
    {
      if ((mapping.juce_param != nullptr) && (mapping.zrythm_param != nullptr))
        {
          const float value = mapping.zrythm_param->currentValue ();
          if (
            !utils::math::floats_equal (value, mapping.juce_param->getValue ()))
            {
              mapping.juce_param->setValue (value);
            }
        }
    }
}

void
JucePlugin::JuceParamListener::parameterValueChanged (
  int   parameterIndex,
  float newValue)
{
  // TODO: Potential feedback loop — if JUCE fires this listener in response
  // to sync_parameters_to_juce() calling setValue() on the audio thread, the
  // message-thread flush will call setBaseValue() again with a potentially
  // quantized/normalized value, causing drift. Consider adding a round-trip
  // tolerance check or a "syncing" flag to suppress callbacks during
  // Zrythm->JUCE sync.
  queue_.push_back (parameterIndex);
  if (
    juce::MessageManager::getInstanceWithoutCreating ()
      ->isThisTheMessageThread ())
    {
      // On the message thread: flush immediately for low-latency UI updates.
      parent_.flush_juce_to_zrythm_queue ();
    }
  else
    {
      // On audio thread or other: deferred to the message thread via QTimer.
    }
}

void
JucePlugin::JuceParamListener::parameterGestureChanged (
  int  parameterIndex,
  bool gestureIsStarting)
{
}

void
JucePlugin::flush_juce_to_zrythm_queue ()
{
  int param_index;
  while (juce_to_zrythm_queue_.pop_front (param_index))
    {
      auto it = juce_param_index_to_mapping_.find (param_index);
      if (it == juce_param_index_to_mapping_.end ())
        continue;

      const auto &mapping = parameter_mappings_[it->second];
      if (mapping.juce_param && mapping.zrythm_param)
        {
          mapping.zrythm_param->setBaseValue (mapping.juce_param->getValue ());
        }
    }
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

void
to_json (nlohmann::json &j, const JucePlugin &p)
{
  to_json (j, static_cast<const Plugin &> (p));
  if (p.juce_plugin_)
    {
      juce::MemoryBlock state_data;
      p.juce_plugin_->getStateInformation (state_data);
      auto encoded = state_data.toBase64Encoding ();
      j[JucePlugin::kStateKey] = encoded.toStdString ();
    }
}
void
from_json (const nlohmann::json &j, JucePlugin &p)
{
  // Note that state must be deserialized first, because the Plugin
  // deserialization may cause an instantiation
  if (j.contains (JucePlugin::kStateKey))
    {
      std::string state_str;
      j[JucePlugin::kStateKey].get_to (state_str);
      p.state_to_apply_.emplace ();
      p.state_to_apply_->fromBase64Encoding (state_str);
    }

  // Now that we have the state, continue deserializing base Plugin...
  from_json (j, static_cast<Plugin &> (p));
}

} // namespace zrythm::plugins
