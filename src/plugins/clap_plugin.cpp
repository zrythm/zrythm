// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense
/*
 * This file incorporates work covered by the following copyright and
 * permission notice:
 *
 * ---
 *
 * SPDX-FileCopyrightText: Copyright (c) 2021 Alexandre BIQUE
 * SPDX-License-Identifier: MIT
 *
 * Copyright (c) 2021 Alexandre BIQUE
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * ---
 *
 */

#include "zrythm-config.h"

#include <utility>

#include "plugins/clap_plugin.h"
#include "plugins/plugin_view_window.h"

#include <clap/helpers/host.hxx>
#include <clap/helpers/plugin-proxy.hxx>
#include <clap/helpers/reducing-param-queue.hxx>

namespace zrythm::plugins
{
thread_local bool is_main_thread = false;

ClapPlugin::ClapPlugin (
  dsp::ProcessorBase::ProcessorBaseDependencies dependencies,
  StateDirectoryParentPathProvider              state_path_provider,
  AudioThreadChecker                            audio_thread_checker,
  QObject *                                     parent)
    : Plugin (dependencies, std::move (state_path_provider), parent),
      ClapHostBase (
        "Zrythm",
        "Alexandros Theodotou",
        "https://www.zrythm.org",
        PACKAGE_VERSION),
      audio_thread_checker_ (std::move (audio_thread_checker))
{
  is_main_thread = true;

  // Connect to configuration changes
  connect (
    this, &Plugin::configurationChanged, this,
    &ClapPlugin::on_configuration_changed);

  // Connect to UI visibility changes
  connect (
    this, &Plugin::uiVisibleChanged, this,
    &ClapPlugin::on_ui_visibility_changed);

  auto bypass_ref = generate_default_bypass_param ();
  add_parameter (bypass_ref);
  bypass_id_ = bypass_ref.id ();
  auto gain_ref = generate_default_gain_param ();
  add_parameter (gain_ref);
  gain_id_ = gain_ref.id ();
}

ClapPlugin::~ClapPlugin () { }

void
ClapPlugin::on_configuration_changed ()
{
  z_debug ("configuration changed");
  auto success = load_plugin (
    std::get<fs::path> (configuration ()->descriptor ()->path_or_id_), 0);
  Q_EMIT instantiationFinished (success, {});
}

void
ClapPlugin::on_ui_visibility_changed ()
{
  if (uiVisible () && !isGuiVisible_)
    {
      show_editor ();
    }
  else if (!uiVisible () && isGuiVisible_)
    {
      hide_editor ();
    }
}

static clap_window
makeClapWindow (WId window)
{
  clap_window w{};
#ifdef Q_OS_LINUX
  w.api = CLAP_WINDOW_API_X11;
  w.x11 = window;
#elifdef Q_OS_MACX
  w.api = CLAP_WINDOW_API_COCOA;
  w.cocoa = reinterpret_cast<clap_nsview> (window);
#elifdef Q_OS_WIN
  w.api = CLAP_WINDOW_API_WIN32;
  w.win32 = reinterpret_cast<clap_hwnd> (window);
#endif

  return w;
}

void
ClapPlugin::show_editor ()
{
  assert (is_main_thread);

  if (!plugin_->canUseGui ())
    return;

  if (isGuiCreated_)
    {
      plugin_->guiDestroy ();
      isGuiCreated_ = false;
      isGuiVisible_ = false;
    }

  const auto getCurrentClapGuiApi = [] () -> const char * {
#if defined(Q_OS_LINUX)
    return CLAP_WINDOW_API_X11;
#elif defined(Q_OS_WIN)
    return CLAP_WINDOW_API_WIN32;
#elif defined(Q_OS_MACOS)
    return CLAP_WINDOW_API_COCOA;
#else
#  error "unsupported platform"
#endif
  };
  guiApi_ = getCurrentClapGuiApi ();

  isGuiFloating_ = false;
  if (!plugin_->guiIsApiSupported (guiApi_, false))
    {
      if (!plugin_->guiIsApiSupported (guiApi_, true))
        {
          z_warning ("could not find a suitable gui api");
          return;
        }
      isGuiFloating_ = true;
    }

  editor_ = std::make_unique<PluginViewWindow> (
    get_name ().to_juce_string (), [this] () {
      z_debug ("close button pressed on CLAP plugin window");
      setUiVisible (false);
    });

  const auto embed_id = editor_->getEmbedWindowId ();
  auto       w = makeClapWindow (embed_id);
  if (!plugin_->guiCreate (w.api, isGuiFloating_))
    {
      z_warning ("could not create the plugin gui");
      return;
    }

  isGuiCreated_ = true;
  assert (isGuiVisible_ == false);

  if (isGuiFloating_)
    {
      plugin_->guiSetTransient (&w);
      plugin_->guiSuggestTitle ("using clap-host suggested title");
    }
  else
    {
      uint32_t width = 0;
      uint32_t height = 0;

      if (!plugin_->guiGetSize (&width, &height))
        {
          z_warning ("could not get the size of the plugin gui");
          isGuiCreated_ = false;
          plugin_->guiDestroy ();
          return;
        }

      editor_->setSize (static_cast<int> (width), static_cast<int> (height));

      if (!plugin_->guiSetParent (&w))
        {
          z_warning ("could embbed the plugin gui");
          isGuiCreated_ = false;
          plugin_->guiDestroy ();
          return;
        }
    }

  setPluginWindowVisibility (true);
}

void
ClapPlugin::hide_editor ()
{
  setPluginWindowVisibility (false);
}

void
ClapPlugin::setPluginWindowVisibility (bool isVisible)
{
  assert (is_main_thread);

  if (!isGuiCreated_)
    return;

  if (isVisible && !isGuiVisible_)
    {
      plugin_->guiShow ();
      isGuiVisible_ = true;
    }
  else if (!isVisible && isGuiVisible_)
    {
      plugin_->guiHide ();
      editor_->setVisible (false);
      isGuiVisible_ = false;
    }
}

void
ClapPlugin::guiResizeHintsChanged () noexcept
{
  // TODO
}

bool
ClapPlugin::guiRequestResize (uint32_t width, uint32_t height) noexcept
{
  editor_->setSize (static_cast<int> (width), static_cast<int> (height));

  return true;
}

bool
ClapPlugin::guiRequestShow () noexcept
{
  setUiVisible (true);

  return true;
}

bool
ClapPlugin::guiRequestHide () noexcept
{
  setUiVisible (false);

  return true;
}

void
ClapPlugin::guiClosed (bool wasDestroyed) noexcept
{
  assert (is_main_thread);
}

bool
ClapPlugin::timerSupportRegisterTimer (
  uint32_t  periodMs,
  clap_id * timerId) noexcept
{
  assert (is_main_thread);

  // Dexed fails this check even though it uses timer so make it a warning...
  z_warn_if_fail (plugin_->canUseTimerSupport ());

  auto id = nextTimerId_++;
  *timerId = id;
  auto timer = std::make_unique<QTimer> ();

  QObject::connect (timer.get (), &QTimer::timeout, this, [this, id] {
    assert (is_main_thread);
    plugin_->timerSupportOnTimer (id);
  });

  auto t = timer.get ();
  timers_.insert_or_assign (*timerId, std::move (timer));
  t->start (static_cast<int> (periodMs));
  return true;
}

bool
ClapPlugin::timerSupportUnregisterTimer (clap_id timerId) noexcept
{
  assert (is_main_thread);

  z_warn_if_fail (plugin_->canUseTimerSupport ());

  auto it = timers_.find (timerId);
  assert (it != timers_.end ());

  timers_.erase (it);
  return true;
}

void
ClapPlugin::prepare_for_processing_impl (
  sample_rate_t sample_rate,
  nframes_t     max_block_length)
{
  assert (is_main_thread);

  if (!plugin_)
    return;

  setup_audio_ports_for_processing (max_block_length);

  assert (!isPluginActive ());
  if (!plugin_->activate (sample_rate, 1, max_block_length))
    {
      setPluginState (InactiveWithError);
      return;
    }

  scheduleProcess_ = true;
  setPluginState (ActiveAndSleeping);
}

void
ClapPlugin::release_resources_impl ()
{
  assert (is_main_thread);

  if (!isPluginActive ())
    return;

  if (state_ == ActiveAndProcessing)
    {
      force_audio_thread_check_.store (true);
      plugin_->stopProcessing ();
      force_audio_thread_check_.store (false);
    }
  setPluginState (ActiveAndReadyToDeactivate);
  scheduleDeactivate_ = false;

  plugin_->deactivate ();
  setPluginState (Inactive);
}

void
ClapPlugin::process_impl (EngineProcessTimeInfo time_info) noexcept
{
  process_.frames_count = time_info.nframes_;
  process_.steady_time = -1;

  assert (threadCheckIsAudioThread ());

  if (!plugin_)
    return;

  // Can't process a plugin that is not active
  if (!isPluginActive ())
    return;

  // Do we want to deactivate the plugin?
  if (scheduleDeactivate_)
    {
      scheduleDeactivate_ = false;
      if (state_ == ActiveAndProcessing)
        plugin_->stopProcessing ();
      setPluginState (ActiveAndReadyToDeactivate);
      return;
    }

  // We can't process a plugin which failed to start processing
  if (state_ == ActiveWithError)
    return;

  process_.transport = nullptr;

  process_.in_events = evIn_.clapInputEvents ();
  process_.out_events = evOut_.clapOutputEvents ();

  process_.audio_inputs = &audioIn_;
  process_.audio_inputs_count = 1;
  process_.audio_outputs = &audioOut_;
  process_.audio_outputs_count = 1;

  evOut_.clear ();
  generatePluginInputEvents ();

  if (isPluginSleeping ())
    {
      if (!scheduleProcess_ && evIn_.empty ())
        // The plugin is sleeping, there is no request to wake it up and there
        // are no events to process
        return;

      scheduleProcess_ = false;
      if (!plugin_->startProcessing ())
        {
          // the plugin failed to start processing
          setPluginState (ActiveWithError);
          return;
        }

      setPluginState (ActiveAndProcessing);
    }

  [[maybe_unused]] int32_t status = CLAP_PROCESS_SLEEP;
  if (isPluginProcessing ())
    {
      const auto local_offset = time_info.local_offset_;
      const auto nframes = time_info.nframes_;

      // Copy input audio to JUCE buffer
      for (
        const auto &[i, channel_ptr] :
        utils::views::enumerate (audio_in_channel_ptrs_))
        {
          utils::float_ranges::copy (
            &channel_ptr[local_offset], &audio_in_ports_[i]->buf_[local_offset],
            nframes);
        }

      // Run plugin processing
      status = plugin_->process (&process_);

      // Copy output audio from JUCE buffer
      for (
        const auto &[i, channel_ptr] :
        utils::views::enumerate (audio_out_channel_ptrs_))
        {
          utils::float_ranges::copy (
            &audio_out_ports_[i]->buf_[local_offset],
            &channel_ptr[local_offset], nframes);
        }
    }

  handlePluginOutputEvents ();

  evOut_.clear ();
  evIn_.clear ();

  engineToAppValueQueue_.producerDone ();

  // TODO: send plugin to sleep if possible
}

void
ClapPlugin::save_state (std::optional<fs::path> abs_state_dir)
{
}

void
ClapPlugin::load_state (std::optional<fs::path> abs_state_dir)
{
}

bool
ClapPlugin::load_plugin (const fs::path &path, int plugin_index)
{
  assert (is_main_thread);

  if (library_.isLoaded ())
    unload_current_plugin ();

  library_.setFileName (utils::Utf8String::from_path (path).to_qstring ());
  library_.setLoadHints (
    QLibrary::ResolveAllSymbolsHint
#if !defined(__has_feature) || !__has_feature(address_sanitizer)
    | QLibrary::DeepBindHint
#endif
  );
  if (!library_.load ())
    {
      z_warning (
        "Failed to load plugin '{}': {}", path, library_.errorString ());
      return false;
    }

  pluginEntry_ = reinterpret_cast<const struct clap_plugin_entry *> (
    library_.resolve ("clap_entry"));
  if (pluginEntry_ == nullptr)
    {
      z_warning ("Unable to resolve entry point 'clap_entry' in '{}'", path);
      library_.unload ();
      return false;
    }

  if (!pluginEntry_->init (utils::Utf8String::from_path (path).c_str ()))
    {
      z_warning ("clap_entry->init() failed for '{}'", path);
    }

  pluginFactory_ = static_cast<const clap_plugin_factory *> (
    pluginEntry_->get_factory (CLAP_PLUGIN_FACTORY_ID));

  auto count = pluginFactory_->get_plugin_count (pluginFactory_);
  if (plugin_index >= static_cast<int> (count))
    {
      z_warning (
        "plugin index {} is invalid, expected at most {}", plugin_index,
        count - 1);
      return false;
    }

  const auto * desc =
    pluginFactory_->get_plugin_descriptor (pluginFactory_, plugin_index);
  if (desc == nullptr)
    {
      z_warning ("no plugin descriptor");
      return false;
    }

  if (!clap_version_is_compatible (desc->clap_version))
    {
      z_warning (
        "Incompatible clap version: Plugin is: {}.{}.{} Host is: {}.{}.{}",
        desc->clap_version.major, desc->clap_version.minor,
        desc->clap_version.revision, CLAP_VERSION.major, CLAP_VERSION.minor,
        CLAP_VERSION.revision);
      return false;
    }

  z_info ("Loading plugin with id: {}, index: {}", desc->id, plugin_index);

  const auto * const plugin =
    pluginFactory_->create_plugin (pluginFactory_, clapHost (), desc->id);
  if (plugin == nullptr)
    {
      z_warning ("could not create the plugin with id: {}", desc->id);
      return false;
    }

  plugin_ = std::make_unique<ClapPluginProxy> (*plugin, *this);

  if (!plugin_->init ())
    {
      z_warning ("could not init the plugin with id: {}", desc->id);
      return false;
    }

  create_ports_from_clap_plugin ();
  scanParams ();
  // scanQuickControls ();

  pluginLoadedChanged (true);

  return true;
}

void
ClapPlugin::unload_current_plugin ()
{
  assert (is_main_thread);

  pluginLoadedChanged (false);

  if (!library_.isLoaded ())
    return;

  if (isGuiCreated_)
    {
      plugin_->guiDestroy ();
      isGuiCreated_ = false;
      isGuiVisible_ = false;
    }

  release_resources_impl ();

  if (plugin_)
    {
      plugin_->destroy ();
    }

  pluginEntry_->deinit ();
  pluginEntry_ = nullptr;

  library_.unload ();
}

bool
ClapPlugin::isPluginActive () const
{
  switch (state_)
    {
    case Inactive:
    case InactiveWithError:
      return false;
    default:
      return true;
    }
}

bool
ClapPlugin::isPluginProcessing () const
{
  return state_ == ActiveAndProcessing;
}

bool
ClapPlugin::isPluginSleeping () const
{
  return state_ == ActiveAndSleeping;
}

bool
ClapPlugin::threadCheckIsAudioThread () const noexcept
{
  if (force_audio_thread_check_.load ())
    {
      return true;
    }

  return audio_thread_checker_ ();
}
bool
ClapPlugin::threadCheckIsMainThread () const noexcept
{
  return is_main_thread;
}

void
ClapPlugin::generatePluginInputEvents ()
{
  appToEngineValueQueue_.consume (
    [this] (clap_id param_id, const AppToEngineParamQueueValue &value) {
      clap_event_param_value ev{};
      ev.header.time = 0;
      ev.header.type = CLAP_EVENT_PARAM_VALUE;
      ev.header.space_id = CLAP_CORE_EVENT_SPACE_ID;
      ev.header.flags = 0;
      ev.header.size = sizeof (ev);
      ev.param_id = param_id;
      ev.cookie = value.cookie;
      ev.port_index = 0;
      ev.key = -1;
      ev.channel = -1;
      ev.note_id = -1;
      ev.value = value.value;
      evIn_.push (&ev.header);
    });

  appToEngineModQueue_.consume (
    [this] (clap_id param_id, const AppToEngineParamQueueValue &value) {
      clap_event_param_mod ev{};
      ev.header.time = 0;
      ev.header.type = CLAP_EVENT_PARAM_MOD;
      ev.header.space_id = CLAP_CORE_EVENT_SPACE_ID;
      ev.header.flags = 0;
      ev.header.size = sizeof (ev);
      ev.param_id = param_id;
      ev.cookie = value.cookie;
      ev.port_index = 0;
      ev.key = -1;
      ev.channel = -1;
      ev.note_id = -1;
      ev.amount = value.value;
      evIn_.push (&ev.header);
    });
}

void
ClapPlugin::handlePluginOutputEvents ()
{
  for (uint32_t i = 0; i < evOut_.size (); ++i)
    {
      auto * h = evOut_.get (i);
      switch (h->type)
        {
        case CLAP_EVENT_PARAM_GESTURE_BEGIN:
          {
            const auto * ev =
              reinterpret_cast<const clap_event_param_gesture *> (h); // NOLINT
            bool &isAdj = isAdjustingParameter_[ev->param_id];

            if (isAdj)
              throw std::logic_error ("The plugin sent BEGIN_ADJUST twice");
            isAdj = true;

            EngineToAppParamQueueValue v;
            v.has_gesture = true;
            v.is_begin = true;
            engineToAppValueQueue_.setOrUpdate (ev->param_id, v);
            break;
          }

        case CLAP_EVENT_PARAM_GESTURE_END:
          {
            const auto * ev =
              reinterpret_cast<const clap_event_param_gesture *> (h); // NOLINT
            bool &isAdj = isAdjustingParameter_[ev->param_id];

            if (!isAdj)
              throw std::logic_error (
                "The plugin sent END_ADJUST without a preceding BEGIN_ADJUST");
            isAdj = false;
            EngineToAppParamQueueValue v;
            v.has_gesture = true;
            v.is_begin = false;
            engineToAppValueQueue_.setOrUpdate (ev->param_id, v);
            break;
          }

        case CLAP_EVENT_PARAM_VALUE:
          {
            const auto * ev =
              reinterpret_cast<const clap_event_param_value *> (h); // NOLINT
            EngineToAppParamQueueValue v;
            v.has_value = true;
            v.value = ev->value;
            engineToAppValueQueue_.setOrUpdate (ev->param_id, v);
            break;
          }
        default:
          z_debug ("unhandled plugin output event {}", h->type);
          break;
        }
    }
}

void
ClapPlugin::requestRestart () noexcept
{
  scheduleRestart_ = true;
}

void
ClapPlugin::requestProcess () noexcept
{
  scheduleProcess_ = true;
}

void
ClapPlugin::requestCallback () noexcept
{
  scheduleMainThreadCallback_ = true;
}

void
ClapPlugin::logLog (clap_log_severity severity, const char * message)
  const noexcept
{
  switch (severity)
    {
    case CLAP_LOG_DEBUG:
      z_debug ("{}", message);
      break;
    case CLAP_LOG_INFO:
      z_info ("{}", message);
      break;
    case CLAP_LOG_WARNING:
      z_warning ("{}", message);
      break;
    case CLAP_LOG_FATAL:
      z_error ("[fatal CLAP error] {}", message);
      break;
    case CLAP_LOG_HOST_MISBEHAVING:
      z_error ("[CLAP host misbehaving] {}", message);
      break;
    case CLAP_LOG_PLUGIN_MISBEHAVING:
      z_warning ("[CLAP plugin misbehaving] {}", message);
    case CLAP_LOG_ERROR:
    default:
      z_error ("{}", message);
    }
}

void
ClapPlugin::create_ports_from_clap_plugin ()
{
  assert (is_main_thread);
  assert (!isPluginActive ());

  if (plugin_->canUseNotePorts ())
    {
      const auto midi_in_ports = plugin_->notePortsCount (true);
      const auto midi_out_ports = plugin_->notePortsCount (false);
      for (const auto i : std::views::iota (0u, midi_in_ports))
        {
          auto port_ref =
            dependencies ().port_registry_.create_object<dsp::MidiPort> (
              utils::Utf8String::from_utf8_encoded_string (
                fmt::format ("midi_in_{}", i + 1)),
              dsp::PortFlow::Input);
          add_input_port (port_ref);
        }
      for (const auto i : std::views::iota (0u, midi_out_ports))
        {
          auto port_ref =
            dependencies ().port_registry_.create_object<dsp::MidiPort> (
              utils::Utf8String::from_utf8_encoded_string (
                fmt::format ("midi_out_{}", i + 1)),
              dsp::PortFlow::Output);
          add_output_port (port_ref);
        }
    }

  if (plugin_->canUseAudioPorts ())
    {
      const auto audio_in_ports = plugin_->audioPortsCount (true);
      const auto audio_out_ports = plugin_->audioPortsCount (false);
      for (const auto i : std::views::iota (0u, audio_in_ports))
        {
          clap_audio_port_info_t nfo{};
          plugin_->audioPortsGet (i, true, &nfo);
          for (const auto ch : std::views::iota (0u, nfo.channel_count))
            {
              auto port_ref =
                dependencies ().port_registry_.create_object<dsp::AudioPort> (
                  utils::Utf8String::from_utf8_encoded_string (
                    fmt::format (
                      "{}_ch_{}",
                      std::strlen (nfo.name) > 0
                        ? nfo.name
                        : fmt::format ("audio_in_{}", i + 1),
                      ch + 1)),
                  dsp::PortFlow::Input);
              add_input_port (port_ref);
            }
        }
      for (const auto i : std::views::iota (0u, audio_out_ports))
        {
          clap_audio_port_info_t nfo{};
          plugin_->audioPortsGet (i, false, &nfo);
          for (const auto ch : std::views::iota (0u, nfo.channel_count))
            {
              auto port_ref =
                dependencies ().port_registry_.create_object<dsp::AudioPort> (
                  utils::Utf8String::from_utf8_encoded_string (
                    fmt::format (
                      "{}_ch_{}",
                      std::strlen (nfo.name) > 0
                        ? nfo.name
                        : fmt::format ("audio_out_{}", i + 1),
                      ch + 1)),
                  dsp::PortFlow::Output);
              add_output_port (port_ref);
            }
        }
    }
}

void
ClapPlugin::setup_audio_ports_for_processing (nframes_t block_size)
{
  const auto num_inputs = audio_in_ports_.size ();
  const auto num_outputs = audio_out_ports_.size ();

  // Resize audio buffers
  audio_in_buf_.setSize (
    static_cast<int> (num_inputs), static_cast<int> (block_size));
  audio_out_buf_.setSize (
    static_cast<int> (num_outputs), static_cast<int> (block_size));

  // Setup channel pointers
  audio_in_channel_ptrs_.resize (num_inputs);
  audio_out_channel_ptrs_.resize (num_outputs);

  for (int i = 0; i < static_cast<int> (num_inputs); ++i)
    {
      audio_in_channel_ptrs_[i] = audio_in_buf_.getWritePointer (i);
    }

  for (int i = 0; i < static_cast<int> (num_outputs); ++i)
    {
      audio_out_channel_ptrs_[i] = audio_out_buf_.getWritePointer (i);
    }

  audioIn_.channel_count = num_inputs;
  audioIn_.data32 = audio_in_channel_ptrs_.data ();
  audioIn_.data64 = nullptr;
  audioIn_.constant_mask = 0;
  audioIn_.latency = 0;

  audioOut_.channel_count = num_outputs;
  audioOut_.data32 = audio_out_channel_ptrs_.data ();
  audioOut_.data64 = nullptr;
  audioOut_.constant_mask = 0;
  audioOut_.latency = 0;
}

void
ClapPlugin::checkValidParamValue (const ClapPluginParam &param, double value)
{
  assert (is_main_thread);

  if (!param.isValueValid (value))
    {
      std::ostringstream msg;
      msg << "Invalid value for param. ";
      param.printInfo (msg);
      msg << "; value: " << value;
      // std::cerr << msg.str() << std::endl;
      throw std::invalid_argument (msg.str ());
    }
}

void
ClapPlugin::scanParams ()
{
  paramsRescan (CLAP_PARAM_RESCAN_ALL);
}

void
ClapPlugin::paramsRescan (uint32_t flags) noexcept
{
  assert (is_main_thread);

  if (!plugin_->canUseParams ())
    return;

  // 1. it is forbidden to use CLAP_PARAM_RESCAN_ALL if the plugin is active
  assert (!isPluginActive () || !(flags & CLAP_PARAM_RESCAN_ALL));

  // 2. scan the params.
  auto                        count = plugin_->paramsCount ();
  std::unordered_set<clap_id> paramIds (count * 2);

  for (const auto i : std::views::iota (0u, count))
    {
      clap_param_info info{};
      assert (plugin_->paramsGetInfo (i, &info));

      assert (info.id != CLAP_INVALID_ID);

      auto it = params_.find (info.id);

      // check that the parameter is not declared twice
      assert (!paramIds.contains (info.id));
      paramIds.insert (info.id);

      if (it == params_.end ())
        {
          assert ((flags & CLAP_PARAM_RESCAN_ALL) != 0u);

          double value = getParamValue (info);
          auto   param = std::make_unique<ClapPluginParam> (info, value, this);
          checkValidParamValue (*param, value);
          params_.insert_or_assign (info.id, std::move (param));
        }
      else
        {
          // update param info
          if (!it->second->isInfoEqualTo (info))
            {
              assert (clapParamsRescanMayInfoChange (flags));
              assert (
                ((flags & CLAP_PARAM_RESCAN_ALL) != 0u)
                || it->second->isInfoCriticallyDifferentTo (info));
              it->second->setInfo (info);
            }

          double value = getParamValue (info);
          if (it->second->value () != value)
            {
              assert (clapParamsRescanMayValueChange (flags));

              // update param value
              checkValidParamValue (*it->second, value);
              it->second->setValue (value);
              it->second->setModulation (value);
            }
        }
    }

  // remove parameters which are gone
  for (auto it = params_.begin (); it != params_.end ();)
    {
      if (paramIds.contains (it->first))
        ++it;
      else
        {
          assert ((flags & CLAP_PARAM_RESCAN_ALL) != 0u);
          it = params_.erase (it);
        }
    }

  if ((flags & CLAP_PARAM_RESCAN_ALL) != 0u)
    paramsChanged ();
}

void
ClapPlugin::paramsClear (clap_id paramId, clap_param_clear_flags flags) noexcept
{
  assert (is_main_thread);
}

void
ClapPlugin::paramFlushOnMainThread ()
{
  assert (is_main_thread);

  assert (!isPluginActive ());

  scheduleParamFlush_ = false;

  evIn_.clear ();
  evOut_.clear ();

  generatePluginInputEvents ();

  if (plugin_->canUseParams ())
    plugin_->paramsFlush (evIn_.clapInputEvents (), evOut_.clapOutputEvents ());
  handlePluginOutputEvents ();

  evOut_.clear ();
  engineToAppValueQueue_.producerDone ();
}

void
ClapPlugin::paramsRequestFlush () noexcept
{
  if (!isPluginActive () && threadCheckIsMainThread ())
    {
      // Perform the flush immediately
      paramFlushOnMainThread ();
      return;
    }

  scheduleParamFlush_ = true;
}

double
ClapPlugin::getParamValue (const clap_param_info &info)
{
  assert (is_main_thread);

  if (!plugin_->canUseParams ())
    return 0;

  double value{};
  if (plugin_->paramsGetValue (info.id, &value))
    return value;

  throw std::logic_error (
    fmt::format (
      "Failed to get the param value, id: {}, name: {}, module: {}", info.id,
      info.name, info.module));
}

void
ClapPlugin::setPluginState (PluginState state)
{
  switch (state)
    {
    case Inactive:
      Q_ASSERT (state_ == ActiveAndReadyToDeactivate);
      break;

    case InactiveWithError:
      Q_ASSERT (state_ == Inactive);
      break;

    case ActiveAndSleeping:
      Q_ASSERT (state_ == Inactive || state_ == ActiveAndProcessing);
      break;

    case ActiveAndProcessing:
      Q_ASSERT (state_ == ActiveAndSleeping);
      break;

    case ActiveWithError:
      Q_ASSERT (state_ == ActiveAndProcessing);
      break;

    case ActiveAndReadyToDeactivate:
      Q_ASSERT (
        state_ == ActiveAndProcessing || state_ == ActiveAndSleeping
        || state_ == ActiveWithError);
      break;

    default:
      std::terminate ();
    }

  state_ = state;
}

} // namespace zrythm::plugins
