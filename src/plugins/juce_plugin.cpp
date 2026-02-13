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
  StateDirectoryParentPathProvider              state_path_provider,
  CreatePluginInstanceAsyncFunc          create_plugin_instance_async_func,
  std::function<units::sample_rate_t ()> sample_rate_provider,
  std::function<nframes_t ()>            buffer_size_provider,
  PluginHostWindowFactory                top_level_window_provider,
  QObject *                              parent)
    : Plugin (dependencies, std::move (state_path_provider), parent),
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
JucePlugin::on_configuration_changed ()
{
  // Reinitialize plugin with new configuration
  if (juce_plugin_)
    {
      juce_plugin_->releaseResources ();
      juce_plugin_.reset ();
      parameter_mappings_.clear ();
    }

  initialize_juce_plugin_async ();
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
JucePlugin::initialize_juce_plugin_async ()
{
  if ((configuration () == nullptr) || plugin_loading_)
    return;

  plugin_loading_ = true;

  const auto &descriptor = *configuration ()->descr_;

  // Create plugin description from descriptor
  auto plugin_desc = descriptor.to_juce_description ();

  // Get current sample rate and buffer size
  const double sample_rate = sample_rate_provider_ ().in (units::sample_rate);
  const int    buffer_size = static_cast<int> (buffer_size_provider_ ());

  // Create plugin instance asynchronously
  create_plugin_instance_async_func_ (
    *plugin_desc, sample_rate, buffer_size,
    [this] (
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
          // Create ports and parameters
          create_ports_from_juce_plugin ();
          create_parameters_from_juce_plugin ();

          // Apply any pending state
          if (state_to_apply_.has_value ())
            {
              juce_plugin_->setStateInformation (
                state_to_apply_->getData (),
                static_cast<int> (state_to_apply_->getSize ()));
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

  // Clear existing parameters
  parameter_mappings_.clear ();

  for (const auto &param : processor.getParameters ())
    {
      // Create parameter name
      const juce::String param_name = param->getName (64);
      const juce::String param_label = param->getLabel ();

      auto * hosted_param =
        processor.getHostedParameter (param->getParameterIndex ());

      // Determine parameter type based on characteristics
      dsp::ParameterRange range{
        dsp::ParameterRange::Type::Linear, 0.f, 1.f, 0.f,
        param->getDefaultValue ()
      };
      if (param->isBoolean ())
        {
          range =
            dsp::ParameterRange::make_toggle (param->getDefaultValue () > 0.5f);
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
          dependencies ().port_registry_,
          dsp::ProcessorParameter::UniqueId (
            utils::Utf8String::from_juce_string (hosted_param->getParameterID ())),
          range, utils::Utf8String::from_juce_string (param->getName (100)));
      add_parameter (zrythm_param_ref);

      auto * zrythm_param =
        zrythm_param_ref.get_object_as<dsp::ProcessorParameter> ();
      zrythm_param->set_automatable (param->isAutomatable ());

      // Store mapping
      // TODO: consider using a map for lookups by ID/index
      // see https://forum.juce.com/t/how-to-use-paramterid-in-the-host
      ParameterMapping mapping{
        .juce_param = param,
        .zrythm_param = zrythm_param,
        .juce_param_index = param->getParameterIndex (),
        .juce_param_listener = std::make_unique<JuceParamListener> (
          *zrythm_param_ref.get_object_as<dsp::ProcessorParameter> ())
      };
      param->addListener (mapping.juce_param_listener.get ());
      parameter_mappings_.emplace_back (std::move (mapping));
    }

  // Sync initial parameter values from JUCE plugin to Zrythm
  update_parameter_values ();
}

nframes_t
JucePlugin::get_single_playback_latency () const
{
  if (juce_plugin_)
    {
      return juce_plugin_->getLatencySamples ();
    }
  return 0;
}

void
JucePlugin::prepare_for_processing_impl (
  units::sample_rate_t sample_rate,
  nframes_t            max_block_length)
{
  if (!juce_plugin_ || !juce_initialized_)
    return;

  try
    {
      // Prepare JUCE plugin
      juce_plugin_->prepareToPlay (
        sample_rate.in (units::sample_rate),
        static_cast<int> (max_block_length));

      // Resize audio buffers
      const int num_inputs = juce_plugin_->getTotalNumInputChannels ();
      const int num_outputs = juce_plugin_->getTotalNumOutputChannels ();

      juce_audio_buffer_.setSize (
        std::max (num_inputs, num_outputs), static_cast<int> (max_block_length));

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
JucePlugin::process_impl (EngineProcessTimeInfo time_info) noexcept
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
                    &input_channels_[ch][local_offset],
                    port->buffers ()->getReadPointer (
                      i, static_cast<int> (local_offset)),
                    nframes);
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
        &juce_audio_buffer_.getWritePointer (ch)[local_offset], 0.0f, nframes);
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
                static_cast<int> (ev.time_));
            }
        }
    }

  // Sync parameters to JUCE
  sync_parameters_to_juce ();

  // Process audio through JUCE plugin
  juce::AudioBuffer<float> temp_buffer_with_offset (
    juce_audio_buffer_.getArrayOfWritePointers (),
    juce_audio_buffer_.getNumChannels (), static_cast<int> (local_offset),
    static_cast<int> (nframes));
  {
#if defined(__has_feature) && __has_feature(realtime_sanitizer)
    // RTSan violations are outside our control here.
    // TODO: add option to keep this enabled (we might want to test our own
    // CLAP plugins in the future)
    __rtsan::ScopedDisabler d;
#endif
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
                      i, static_cast<int> (local_offset)),
                    &output_channels_[ch][local_offset], nframes);
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
            ev.data, ev.numBytes, ev.samplePosition);
        }
    }
}

void
JucePlugin::update_parameter_values ()
{
  if (!juce_plugin_)
    return;

  // FIXME: we are iterating over all params
  for (const auto &mapping : parameter_mappings_)
    {
      if ((mapping.juce_param != nullptr) && (mapping.zrythm_param != nullptr))
        {
          const float value = mapping.juce_param->getValue ();
          // is this correct? what if we're automating?
          mapping.zrythm_param->setBaseValue (value);
        }
    }
}

void
JucePlugin::sync_parameters_to_juce ()
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
JucePlugin::save_state (std::optional<fs::path> abs_state_dir)
{
  if (!juce_plugin_)
    return;

  if (!abs_state_dir)
    {
      abs_state_dir = get_state_directory ();
    }

  try
    {
      // Create state directory if it doesn't exist
      fs::create_directories (*abs_state_dir);

      // Save JUCE plugin state
      juce::MemoryBlock state_data;
      juce_plugin_->getStateInformation (state_data);

      const fs::path state_file = *abs_state_dir / "juce_plugin_state.dat";
      QFile          file (state_file);
      if (!file.open (QFile::OpenModeFlag::WriteOnly))
        {
          throw std::runtime_error (
            fmt::format (
              "Failed to open state file for writing at {}", state_file));
        }
      auto bytes_written = file.write (
        static_cast<const char *> (state_data.getData ()),
        static_cast<qint64> (state_data.getSize ()));
      if (bytes_written == -1)
        {
          throw std::runtime_error (
            fmt::format ("Failed to write state file at {}", state_file));
        }

      // do we need to save parameter values separately?
    }
  catch (const std::exception &e)
    {
      z_error ("Failed to save JUCE plugin state: {}", e.what ());
    }
}

void
JucePlugin::load_state (std::optional<fs::path> abs_state_dir)
{
  if (!juce_plugin_)
    return;

  if (!abs_state_dir)
    {
      abs_state_dir = get_state_directory ();
    }

  try
    {
      // Load JUCE plugin state
      const fs::path state_file = *abs_state_dir / "juce_plugin_state.dat";
      if (fs::exists (state_file))
        {
          auto contents = utils::io::read_file_contents (state_file);
          juce_plugin_->setStateInformation (
            contents.constData (), static_cast<int> (contents.size ()));
        }

      // do we need to load parameter values separately?

      // Sync parameters to JUCE
      sync_parameters_to_juce ();
    }
  catch (const std::exception &e)
    {
      z_error ("Failed to load JUCE plugin state: {}", e.what ());
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

} // namespace zrythm::plugins
