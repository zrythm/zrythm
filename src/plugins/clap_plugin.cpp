// SPDX-FileCopyrightText: © 2025-2026 Alexandros Theodotou <alex@zrythm.org>
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

#include <algorithm>
#include <memory>
#include <utility>

#include "plugins/CLAPPluginFormat.h"
#include "plugins/clap_plugin.h"
#include "plugins/plugin_library.h"
#include "utils/format_qt.h"
#include "utils/io_utils.h"
#include "utils/raii_utils.h"
#include "utils/views.h"

#include <QFile>
#include <QSemaphore>
#include <QSocketNotifier>
#include <QTimer>

#include <clap/helpers/event-list.hh>
#include <clap/helpers/host.hxx>
#include <clap/helpers/plugin-proxy.hh>
#include <clap/helpers/plugin-proxy.hxx>
#if defined(__has_feature) && __has_feature(realtime_sanitizer)
#  include <sanitizer/rtsan_interface.h>
#endif

namespace zrythm::plugins
{
thread_local bool is_main_thread = false;
// Set by process_impl() for both real-time audio and offline rendering.
thread_local bool is_audio_thread = false;

using ClapPluginProxy = clap::helpers::PluginProxy<
  clap::helpers::MisbehaviourHandler::Terminate,
  clap::helpers::CheckingLevel::Maximal>;

class ClapPlugin::ClapPluginImpl
{
  friend class ClapPlugin;

public:
  ClapPluginImpl (ClapPlugin &owner, PluginHostWindowFactory host_window_factory)
      : owner_ (owner), host_window_factory_ (std::move (host_window_factory))
  {
  }

  struct ClapParamAdapter
  {
    clap_id                   id;
    clap_param_info           info;
    dsp::ProcessorParameter * zrythm_param = nullptr;
    size_t                    param_index = 0;
  };

  enum PluginState
  {
    // The plugin is inactive, only the main thread uses it
    Inactive,

    // Activation failed
    InactiveWithError,

    // The plugin is active and sleeping, the audio engine can call
    // set_processing()
    ActiveAndSleeping,

    // The plugin is processing
    ActiveAndProcessing,

    // The plugin did process but is in error
    ActiveWithError,

    // The plugin is not used anymore by the audio engine and can be
    // deactivated on the main thread
    ActiveAndReadyToDeactivate,
  };
  bool isPluginActive () const;
  bool isPluginProcessing () const;
  bool isPluginSleeping () const;
  void setPluginState (PluginState state);

  /* clap host callbacks */

  [[nodiscard]] bool
         checkValidParamValue (const ClapParamAdapter &param, double value);
  double getParamValue (const clap_param_info &info);
  static bool clapParamsRescanMayValueChange (uint32_t flags)
  {
    return (flags & (CLAP_PARAM_RESCAN_ALL | CLAP_PARAM_RESCAN_VALUES)) != 0u;
  }
  static bool clapParamsRescanMayInfoChange (uint32_t flags)
  {
    return (flags & (CLAP_PARAM_RESCAN_ALL | CLAP_PARAM_RESCAN_INFO)) != 0u;
  }

  /**
   * @brief Performs a CLAP paramsFlush when the plugin is inactive.
   *
   * Calls paramsFlush on the plugin, then drains any output values through
   * param_sync_. Main thread only.
   */
  void paramFlushOnMainThread () [[clang::blocking]];

  /**
   * @brief Processes CLAP output events from the plugin's last process() call.
   *
   * For param value events, stores the normalized value in param_sync_ for
   * cross-thread bridging and sets the feedback guard. Audio thread.
   */
  void handlePluginOutputEvents () noexcept [[clang::nonblocking]];

  /**
   * @brief Generates CLAP param value events for changed parameters only.
   *
   * Uses ParameterChangeTracker to iterate only params that changed this
   * cycle, with feedback prevention via ParamSync. Audio thread only.
   */
  void generateChangedParamInputEvents () noexcept [[clang::nonblocking]];

  /**
   * @brief Generates CLAP param value events for all parameters.
   *
   * Sends current base values for all mapped params. Main thread only
   * (used by paramFlush when plugin is inactive).
   */
  void generateAllParamInputEvents () [[clang::blocking]];

  /** @brief Generates CLAP MIDI events from the MIDI input port. */
  void generateMidiInputEvents () noexcept [[clang::nonblocking]];

  void setup_audio_ports_for_processing (units::sample_u32_t block_size);

  void setPluginWindowVisibility (bool isVisible);

  void eventLoopSetFdNotifierFlags (int fd, int flags);

private:
  ClapPlugin &owner_;

  PluginLibrary library_;

  const clap_plugin_entry *        pluginEntry_ = nullptr;
  const clap_plugin_factory *      pluginFactory_ = nullptr;
  std::unique_ptr<ClapPluginProxy> plugin_;

  /* timers */
  clap_id                                              nextTimerId_ = 0;
  std::unordered_map<clap_id, std::unique_ptr<QTimer>> timers_;

  /* fd events */
  struct Notifiers
  {
    std::unique_ptr<QSocketNotifier> rd;
    std::unique_ptr<QSocketNotifier> wr;
  };
  std::unordered_map<int, std::unique_ptr<Notifiers>> fds_;

  /* thread pool */
  std::vector<std::unique_ptr<QThread>> threadPool_;
  std::atomic<bool>                     threadPoolStop_ = { false };
  std::atomic<int>                      threadPoolTaskIndex_ = { 0 };
  QSemaphore                            threadPoolSemaphoreProd_;
  QSemaphore                            threadPoolSemaphoreDone_;

  /* process stuff */
  std::vector<clap_audio_buffer> audio_in_clap_bufs_;
  std::vector<clap_audio_buffer> audio_out_clap_bufs_;

  // each CLAP port can have multiple channels. there is 1 AudioSampleBuffer per
  // CLAP port
  // FIXME: these temporary buffers can be removed - use audio port buffers
  // directly to avoid unnecessary copies
  std::vector<juce::AudioSampleBuffer> audio_in_bufs_;

  std::vector<juce::AudioSampleBuffer> audio_out_bufs_;

  clap::helpers::EventList evIn_;
  clap::helpers::EventList evOut_;
  clap_process             process_{};

  /* param update queues */
  std::unordered_map<clap_id, ClapParamAdapter> clap_params_;

  PluginState state_{ Inactive };
  bool        stateIsDirty_ = false;

  // TODO: scheduleRestart_ is stored but never checked — wire into
  // process_impl() to handle the CLAP host requestRestart() callback.
  std::atomic_bool scheduleRestart_{ false };
  std::atomic_bool scheduleDeactivate_{ false };

  std::atomic_bool scheduleProcess_{ true };

  std::atomic_bool scheduleParamFlush_{ false };

  const char * guiApi_ = nullptr;
  bool         isGuiCreated_ = false;
  bool         isGuiVisible_ = false;
  bool         isGuiFloating_ = false;

  // TODO: scheduleMainThreadCallback_ is stored but never checked — wire
  // into the main thread event loop to handle the CLAP requestCallback().
  std::atomic_bool scheduleMainThreadCallback_{ false };

  // work-around the fact that stopProcessing() requires being called by an
  // audio thread for whatever reason
  std::atomic_bool force_audio_thread_check_{ false };

  PluginHostWindowFactory            host_window_factory_;
  std::unique_ptr<IPluginHostWindow> editor_;

  units::sample_u32_t latency_;
};

ClapPlugin::ClapPlugin (
  dsp::ProcessorBase::ProcessorBaseDependencies dependencies,
  PluginHostWindowFactory                       host_window_factory,
  QObject *                                     parent)
    : Plugin (dependencies, parent),
      ClapHostBase (
        "Zrythm",
        "Alexandros Theodotou",
        "https://www.zrythm.org",
        PACKAGE_VERSION),
      pimpl_ (
        std::make_unique<ClapPluginImpl> (*this, std::move (host_window_factory)))
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

ClapPlugin::~ClapPlugin ()
{
  if (pimpl_ && pimpl_->library_.is_loaded ())
    unload_current_plugin ();
}

void
ClapPlugin::on_configuration_changed (
  PluginConfiguration * config,
  bool                  generateNewPluginPortsAndParams)
{
  z_debug ("configuration changed");
  auto success = load_plugin (
    std::get<std::filesystem::path> (
      configuration ()->descriptor ()->path_or_id_),
    configuration ()->descriptor ()->unique_id_,
    generateNewPluginPortsAndParams);
  Q_EMIT instantiationFinished (success, {});
}

void
ClapPlugin::on_ui_visibility_changed ()
{
  if (uiVisible () && !pimpl_->isGuiVisible_)
    {
      show_editor ();
    }
  else if (!uiVisible () && pimpl_->isGuiVisible_)
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
#elifdef Q_OS_MACOS
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

  if (!pimpl_->plugin_->canUseGui ())
    return;

  if (pimpl_->isGuiCreated_)
    {
      pimpl_->plugin_->guiDestroy ();
      pimpl_->isGuiCreated_ = false;
      pimpl_->isGuiVisible_ = false;
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
  pimpl_->guiApi_ = getCurrentClapGuiApi ();

  pimpl_->isGuiFloating_ = false;
  if (!pimpl_->plugin_->guiIsApiSupported (pimpl_->guiApi_, false))
    {
      if (!pimpl_->plugin_->guiIsApiSupported (pimpl_->guiApi_, true))
        {
          z_warning ("could not find a suitable gui api");
          return;
        }
      pimpl_->isGuiFloating_ = true;
    }

  pimpl_->editor_ = pimpl_->host_window_factory_ (*this);

  const auto embed_id = pimpl_->editor_->getEmbedWindowId ();
  auto       w = makeClapWindow (embed_id);
  if (!pimpl_->plugin_->guiCreate (w.api, pimpl_->isGuiFloating_))
    {
      z_warning ("could not create the plugin gui");
      return;
    }

  pimpl_->isGuiCreated_ = true;
  assert (pimpl_->isGuiVisible_ == false);

  if (pimpl_->isGuiFloating_)
    {
      pimpl_->plugin_->guiSetTransient (&w);
      pimpl_->plugin_->guiSuggestTitle ("using clap-host suggested title");
    }
  else
    {
      uint32_t width = 0;
      uint32_t height = 0;

      if (!pimpl_->plugin_->guiGetSize (&width, &height))
        {
          z_warning ("could not get the size of the plugin gui");
          pimpl_->isGuiCreated_ = false;
          pimpl_->plugin_->guiDestroy ();
          pimpl_->editor_->setVisible (false);
          setUiVisible (false);
          return;
        }

      pimpl_->editor_->setSizeAndCenter (
        static_cast<int> (width), static_cast<int> (height));

      if (!pimpl_->plugin_->guiSetParent (&w))
        {
          z_warning ("could not embbed the plugin gui");
          pimpl_->isGuiCreated_ = false;
          pimpl_->plugin_->guiDestroy ();
          pimpl_->editor_->setVisible (false);
          setUiVisible (false);
          return;
        }
    }

  pimpl_->setPluginWindowVisibility (true);
}

void
ClapPlugin::hide_editor ()
{
  pimpl_->setPluginWindowVisibility (false);
}

void
ClapPlugin::ClapPluginImpl::setPluginWindowVisibility (bool isVisible)
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
  pimpl_->editor_->setSize (static_cast<int> (width), static_cast<int> (height));

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
ClapPlugin::posixFdSupportRegisterFd (int fd, clap_posix_fd_flags_t flags) noexcept
{
  assert (is_main_thread);

  z_warn_if_fail (pimpl_->plugin_->canUsePosixFdSupport ())

    [[maybe_unused]] auto it = pimpl_->fds_.find (fd);
  assert (it == pimpl_->fds_.end ());

  pimpl_->fds_.insert_or_assign (
    fd, std::make_unique<ClapPluginImpl::Notifiers> ());
  pimpl_->eventLoopSetFdNotifierFlags (fd, static_cast<int> (flags));
  return true;
}

bool
ClapPlugin::posixFdSupportModifyFd (int fd, clap_posix_fd_flags_t flags) noexcept
{
  assert (is_main_thread);

  z_warn_if_fail (pimpl_->plugin_->canUsePosixFdSupport ());

  [[maybe_unused]] auto it = pimpl_->fds_.find (fd);
  assert (it != pimpl_->fds_.end ());

  pimpl_->fds_.insert_or_assign (
    fd, std::make_unique<ClapPluginImpl::Notifiers> ());
  pimpl_->eventLoopSetFdNotifierFlags (fd, static_cast<int> (flags));
  return true;
}

bool
ClapPlugin::posixFdSupportUnregisterFd (int fd) noexcept
{
  assert (is_main_thread);

  z_warn_if_fail (pimpl_->plugin_->canUsePosixFdSupport ());

  auto it = pimpl_->fds_.find (fd);
  assert (it != pimpl_->fds_.end ());

  pimpl_->fds_.erase (it);
  return true;
}

void
ClapPlugin::ClapPluginImpl::eventLoopSetFdNotifierFlags (int fd, int flags)
{
  assert (is_main_thread);

  auto it = fds_.find (fd);
  Q_ASSERT (it != fds_.end ());

  if ((flags & CLAP_POSIX_FD_READ) != 0)
    {
      if (!it->second->rd)
        {
          it->second->rd = std::make_unique<QSocketNotifier> (
            (qintptr) fd, QSocketNotifier::Read);
          QObject::connect (
            it->second->rd.get (), &QSocketNotifier::activated, &owner_,
            [this, fd] {
              assert (is_main_thread);
              plugin_->posixFdSupportOnFd (fd, CLAP_POSIX_FD_READ);
            });
        }
      it->second->rd->setEnabled (true);
    }
  else if (it->second->rd)
    it->second->rd->setEnabled (false);

  if ((flags & CLAP_POSIX_FD_WRITE) != 0)
    {
      if (!it->second->wr)
        {
          it->second->wr = std::make_unique<QSocketNotifier> (
            (qintptr) fd, QSocketNotifier::Write);
          QObject::connect (
            it->second->wr.get (), &QSocketNotifier::activated, &owner_,
            [this, fd] {
              assert (is_main_thread);
              plugin_->posixFdSupportOnFd (fd, CLAP_POSIX_FD_WRITE);
            });
        }
      it->second->wr->setEnabled (true);
    }
  else if (it->second->wr)
    it->second->wr->setEnabled (false);
}

bool
ClapPlugin::threadPoolRequestExec (uint32_t numTasks) noexcept
{
  assert (threadCheckIsAudioThread ());

  z_warn_if_fail (pimpl_->plugin_->canUseThreadPool ());

  Q_ASSERT (!pimpl_->threadPoolStop_);
  Q_ASSERT (!pimpl_->threadPool_.empty ());

  if (numTasks == 0)
    return true;

  if (numTasks == 1)
    {
      pimpl_->plugin_->threadPoolExec (0);
      return true;
    }

  pimpl_->threadPoolTaskIndex_ = 0;
  pimpl_->threadPoolSemaphoreProd_.release (static_cast<int> (numTasks));
  pimpl_->threadPoolSemaphoreDone_.acquire (static_cast<int> (numTasks));
  return true;
}

bool
ClapPlugin::timerSupportRegisterTimer (
  uint32_t  periodMs,
  clap_id * timerId) noexcept
{
  assert (is_main_thread);

  // Dexed fails this check even though it uses timer so make it a warning...
  z_warn_if_fail (pimpl_->plugin_->canUseTimerSupport ());

  auto id = pimpl_->nextTimerId_++;
  *timerId = id;
  auto timer = std::make_unique<QTimer> ();

  QObject::connect (timer.get (), &QTimer::timeout, this, [this, id] {
    assert (is_main_thread);
    pimpl_->plugin_->timerSupportOnTimer (id);
  });

  auto t = timer.get ();
  pimpl_->timers_.insert_or_assign (*timerId, std::move (timer));
  t->start (static_cast<int> (periodMs));
  return true;
}

bool
ClapPlugin::timerSupportUnregisterTimer (clap_id timerId) noexcept
{
  assert (is_main_thread);

  z_warn_if_fail (pimpl_->plugin_->canUseTimerSupport ());

  auto it = pimpl_->timers_.find (timerId);
  assert (it != pimpl_->timers_.end ());

  pimpl_->timers_.erase (it);
  return true;
}

void
ClapPlugin::presetLoadLoaded (
  uint32_t     locationKind,
  const char * location,
  const char * loadKey) noexcept
{
  assert (is_main_thread);
  z_info (
    "CLAP preset loaded: location_kind={} location='{}' load_key='{}'",
    locationKind, location, loadKey);
}

void
ClapPlugin::presetLoadOnError (
  uint32_t     locationKind,
  const char * location,
  const char * loadKey,
  int32_t      osError,
  const char * msg) noexcept
{
  assert (is_main_thread);
  z_warning (
    "CLAP preset load error: location_kind={} location='{}' load_key='{}' "
    "os_error={} msg='{}'",
    locationKind, location, loadKey, osError, msg);
}

void
ClapPlugin::stateMarkDirty () noexcept
{
  assert (is_main_thread);

  z_warn_if_fail (pimpl_->plugin_->canUseState ());

  pimpl_->stateIsDirty_ = true;
}

void
ClapPlugin::latencyChanged () noexcept
{
  z_debug (
    "{} latency changed to {}", get_name (), pimpl_->plugin_->latencyGet ());
  pimpl_->latency_ = units::samples (pimpl_->plugin_->latencyGet ());
}

void
ClapPlugin::prepare_for_processing_impl (
  units::sample_rate_t sample_rate,
  units::sample_u32_t  max_block_length)
{
  assert (is_main_thread);

  if (!pimpl_->plugin_)
    return;

  // Clear resources if already active (this also deactivates the plugin)
  if (pimpl_->isPluginActive ())
    {
      release_resources_impl ();
    }

  pimpl_->setup_audio_ports_for_processing (max_block_length);

  // Pre-allocate the input event list so that
  // generateChangedParamInputEvents() never needs to grow the vector on the
  // audio thread. Reserve space for all parameters plus headroom for MIDI
  // events.
  pimpl_->evIn_.reserveEvents (
    zrythm_to_clap_.size ()
    + static_cast<size_t> (get_descriptor ().num_midi_ins_) * 16);

  if (
    !pimpl_->plugin_->activate (
      sample_rate.in (units::sample_rate), 1,
      max_block_length.in (units::samples)))
    {
      pimpl_->setPluginState (ClapPluginImpl::InactiveWithError);
      return;
    }

  pimpl_->scheduleProcess_ = true;
  pimpl_->setPluginState (ClapPluginImpl::ActiveAndSleeping);
  if (pimpl_->plugin_->canUseLatency ())
    {
      pimpl_->latency_ = units::samples (pimpl_->plugin_->latencyGet ());
    }
}

void
ClapPlugin::release_resources_impl ()
{
  assert (is_main_thread);

  if (!pimpl_->isPluginActive ())
    return;

  if (pimpl_->state_ == ClapPluginImpl::ActiveAndProcessing)
    {
      // Pretend to be the audio thread — stopProcessing() is called here
      // from the main thread during release_resources, but the CLAP plugin
      // expects it from the audio thread.
      AtomicBoolRAII audio_thread_check{ pimpl_->force_audio_thread_check_ };
      pimpl_->plugin_->stopProcessing ();
    }
  pimpl_->setPluginState (ClapPluginImpl::ActiveAndReadyToDeactivate);
  pimpl_->scheduleDeactivate_ = false;

  pimpl_->plugin_->deactivate ();
  pimpl_->setPluginState (ClapPluginImpl::Inactive);
}

void
ClapPlugin::process_impl (dsp::graph::EngineProcessTimeInfo time_info) noexcept
{
  ScopedBool audio_thread_guard{ is_audio_thread };

  pimpl_->process_.frames_count = time_info.nframes_.in (units::samples);
  pimpl_->process_.steady_time = -1;

  if (!pimpl_->plugin_)
    return;

  // Can't process a plugin that is not active
  if (!pimpl_->isPluginActive ())
    return;

  // Do we want to deactivate the plugin?
  if (pimpl_->scheduleDeactivate_.load (std::memory_order_acquire))
    {
      pimpl_->scheduleDeactivate_.store (false, std::memory_order_release);
      if (pimpl_->state_ == ClapPluginImpl::ActiveAndProcessing)
        pimpl_->plugin_->stopProcessing ();
      pimpl_->setPluginState (ClapPluginImpl::ActiveAndReadyToDeactivate);
      return;
    }

  // We can't process a plugin which failed to start processing
  if (pimpl_->state_ == ClapPluginImpl::ActiveWithError)
    return;

  pimpl_->process_.transport = nullptr;

  pimpl_->process_.in_events = pimpl_->evIn_.clapInputEvents ();
  pimpl_->process_.out_events = pimpl_->evOut_.clapOutputEvents ();

  pimpl_->process_.audio_inputs = pimpl_->audio_in_clap_bufs_.data ();
  pimpl_->process_.audio_inputs_count =
    static_cast<uint32_t> (pimpl_->audio_in_clap_bufs_.size ());
  pimpl_->process_.audio_outputs = pimpl_->audio_out_clap_bufs_.data ();
  pimpl_->process_.audio_outputs_count =
    static_cast<uint32_t> (pimpl_->audio_out_clap_bufs_.size ());

  pimpl_->evOut_.clear ();

  pimpl_->generateChangedParamInputEvents ();
  pimpl_->generateMidiInputEvents ();

  if (pimpl_->isPluginSleeping ())
    {
      if (
        !pimpl_->scheduleProcess_.load (std::memory_order_acquire)
        && pimpl_->evIn_.empty ())
        // The plugin is sleeping, there is no request to wake it up and there
        // are no events to process
        return;

      pimpl_->scheduleProcess_.store (false, std::memory_order_release);
      if (!pimpl_->plugin_->startProcessing ())
        {
          // the plugin failed to start processing
          pimpl_->setPluginState (ClapPluginImpl::ActiveWithError);
          return;
        }

      pimpl_->setPluginState (ClapPluginImpl::ActiveAndProcessing);
    }

  int32_t status = CLAP_PROCESS_SLEEP;
  if (pimpl_->isPluginProcessing ())
    {
      const auto local_offset = time_info.local_offset_;
      const auto nframes = time_info.nframes_;

      // Copy input audio to JUCE buffer
      for (
        const auto &[in_buf, port] :
        std::views::zip (pimpl_->audio_in_bufs_, audio_in_ports_))
        {
          for (const auto ch : std::views::iota (0, in_buf.getNumChannels ()))
            {
              in_buf.copyFrom (
                ch, local_offset.in<int> (units::samples), *port->buffers (),
                ch, local_offset.in<int> (units::samples),
                nframes.in<int> (units::samples));
            }
        }

      // Run plugin processing
      {
#if defined(__has_feature) && __has_feature(realtime_sanitizer)
        // Not our code, we don't care about RTSan violations here.
        // TODO: add option to keep this enabled (we might want to test our own
        // CLAP plugins in the future)
        __rtsan::ScopedDisabler d;
#endif
        status = pimpl_->plugin_->process (&pimpl_->process_);
      }

      // Copy output audio from JUCE buffer
      for (
        const auto &[buf, port] :
        std::views::zip (pimpl_->audio_out_bufs_, audio_out_ports_))
        {
          if (status == CLAP_PROCESS_ERROR) [[unlikely]]
            {
              port->buffers ()->clear (
                local_offset.in<int> (units::samples),
                nframes.in<int> (units::samples));
            }
          else
            {
              // TODO: handle other states
              for (const auto ch : std::views::iota (0, buf.getNumChannels ()))
                {
                  port->buffers ()->copyFrom (
                    ch, local_offset.in<int> (units::samples), buf, ch,
                    local_offset.in<int> (units::samples),
                    nframes.in<int> (units::samples));
                }
            }
        }
    }

  pimpl_->handlePluginOutputEvents ();

  pimpl_->evOut_.clear ();
  pimpl_->evIn_.clear ();

  // TODO: send plugin to sleep if possible
}

units::sample_u32_t
ClapPlugin::get_single_playback_latency () const
{
  return pimpl_->latency_;
}

bool
ClapPlugin::load_plugin (
  const std::filesystem::path &path,
  int64_t                      plugin_unique_id,
  bool                         generate_new_ports)
{
  assert (is_main_thread);

  if (pimpl_->library_.is_loaded ())
    unload_current_plugin ();

  if (!pimpl_->library_.load (utils::Utf8String::from_path (path)))
    {
      z_warning (
        "Failed to load plugin '{}': {}", path,
        pimpl_->library_.error_string ());
      return false;
    }

  pimpl_->pluginEntry_ = reinterpret_cast<const struct clap_plugin_entry *> (
    pimpl_->library_.resolve ("clap_entry"));
  if (pimpl_->pluginEntry_ == nullptr)
    {
      z_warning ("Unable to resolve entry point 'clap_entry' in '{}'", path);
      pimpl_->library_.unload ();
      return false;
    }

  if (!pimpl_->pluginEntry_->init (utils::Utf8String::from_path (path).c_str ()))
    {
      z_warning ("clap_entry->init() failed for '{}'", path);
    }

  pimpl_->pluginFactory_ = static_cast<const clap_plugin_factory *> (
    pimpl_->pluginEntry_->get_factory (CLAP_PLUGIN_FACTORY_ID));

  const auto * const desc = [&] () -> const clap_plugin_descriptor_t * {
    const auto count =
      pimpl_->pluginFactory_->get_plugin_count (pimpl_->pluginFactory_);
    for (const auto i : std::views::iota (0u, count))
      {
        const auto * cur_desc = pimpl_->pluginFactory_->get_plugin_descriptor (
          pimpl_->pluginFactory_, i);
        if (
          CLAPPluginFormat::get_hash_for_range (std::string (cur_desc->id))
          == plugin_unique_id)
          {
            return cur_desc;
          }
      }
    return nullptr;
  }();

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

  z_info ("Loading plugin with id: {}", desc->id);

  const auto * const plugin = pimpl_->pluginFactory_->create_plugin (
    pimpl_->pluginFactory_, clapHost (), desc->id);
  if (plugin == nullptr)
    {
      z_warning ("could not create the plugin with id: {}", desc->id);
      return false;
    }

  pimpl_->plugin_ = std::make_unique<ClapPluginProxy> (*plugin, *this);

  if (!pimpl_->plugin_->init ())
    {
      z_warning ("could not init the plugin with id: {}", desc->id);
      return false;
    }

  if (generate_new_ports)
    {
      create_ports_from_clap_plugin ();
    }
  paramsRescan (CLAP_PARAM_RESCAN_ALL);
  // scanQuickControls ();

  // Apply any pending state from JSON deserialization
  if (state_to_apply_.has_value ())
    {
      z_debug (
        "CLAP: applying saved state ({} bytes)", state_to_apply_->size ());

      apply_state_from_byte_array (*state_to_apply_);
      state_to_apply_.reset ();

      // Per the CLAP spec, plugins may defer parameter updates until the
      // next paramsFlush() after state load. Flush to ensure parameter
      // values are current before reading them back.
      if (pimpl_->plugin_->canUseParams ())
        {
          pimpl_->evIn_.clear ();
          pimpl_->evOut_.clear ();
          pimpl_->plugin_->paramsFlush (
            pimpl_->evIn_.clapInputEvents (),
            pimpl_->evOut_.clapOutputEvents ());
          // Discard output events — the flush is only to trigger internal
          // plugin state resolution, not to process output parameter values.
          pimpl_->evOut_.clear ();
        }

      // Per the CLAP spec, the plugin is responsible for persisting its
      // parameter values via its state. Read back the parameter values (which
      // reflect the loaded state) and update Zrythm's baseValues to match.
      if (pimpl_->plugin_->canUseParams ())
        {
          int updated = 0;
          for (auto &[clap_id_val, adapter] : pimpl_->clap_params_)
            {
              double value = 0;
              if (!pimpl_->plugin_->paramsGetValue (clap_id_val, &value))
                continue;

              auto * zrythm_param = adapter.zrythm_param;
              if (zrythm_param == nullptr)
                continue;

              const auto old_base = zrythm_param->baseValue ();
              const auto range = zrythm_param->range ();
              const auto normalized =
                range.convertTo0To1 (static_cast<float> (value));
              if (!utils::math::floats_near (old_base, normalized, 0.001f))
                {
                  zrythm_param->setBaseValue (normalized);
                  ++updated;
                  z_trace (
                    "CLAP: get_value updated '{}' "
                    "old={:.4f} new={:.4f}",
                    zrythm_param->label (), old_base, normalized);
                }
            }
          z_debug ("CLAP: get_value updated {} params", updated);
        }
    }
  else
    {
      z_debug ("CLAP: no saved state to apply");
    }

  Q_EMIT pluginLoadedChanged (true);

  return true;
}

void
ClapPlugin::unload_current_plugin ()
{
  assert (is_main_thread);

  pimpl_->clap_params_.clear ();
  zrythm_to_clap_.clear ();

  Q_EMIT pluginLoadedChanged (false);

  if (!pimpl_->library_.is_loaded ())
    return;

  if (pimpl_->isGuiCreated_)
    {
      pimpl_->plugin_->guiDestroy ();
      pimpl_->isGuiCreated_ = false;
      pimpl_->isGuiVisible_ = false;
    }

  release_resources_impl ();

  if (pimpl_->plugin_)
    {
      pimpl_->plugin_->destroy ();
    }

  pimpl_->pluginEntry_->deinit ();
  pimpl_->pluginEntry_ = nullptr;

  pimpl_->library_.unload ();
}

bool
ClapPlugin::ClapPluginImpl::isPluginActive () const
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
ClapPlugin::ClapPluginImpl::isPluginProcessing () const
{
  return state_ == ActiveAndProcessing;
}

bool
ClapPlugin::ClapPluginImpl::isPluginSleeping () const
{
  return state_ == ActiveAndSleeping;
}

bool
ClapPlugin::threadCheckIsAudioThread () const noexcept
{
  if (pimpl_->force_audio_thread_check_.load ())
    {
      return true;
    }

  return is_audio_thread;
}
bool
ClapPlugin::threadCheckIsMainThread () const noexcept
{
  return is_main_thread;
}

void
ClapPlugin::ClapPluginImpl::generateChangedParamInputEvents () noexcept
{
  // Audio thread: send only changed params with feedback prevention.
  for (const auto &change : owner_.change_tracker ().changes ())
    {
      auto &entry = owner_.param_sync_.entries[change.index];

      // Feedback prevention: skip if value came from the plugin
      if (
        utils::math::floats_equal (
          change.modulated_value, entry.last_from_plugin))
        {
          entry.last_from_plugin = -1.f;
          continue;
        }

      // Find CLAP ID for this param
      auto * param = change.param;
      auto   it = owner_.zrythm_to_clap_.find (param);
      if (it == owner_.zrythm_to_clap_.end ())
        continue;

      const clap_id clap_id_val = it->second;
      auto          adapter_it = clap_params_.find (clap_id_val);
      if (adapter_it == clap_params_.end ())
        continue;

      const auto  &adapter = adapter_it->second;
      const auto   range = param->range ();
      const double clap_value = range.convertFrom0To1 (change.modulated_value);

      clap_event_param_value ev{};
      ev.header.time = 0;
      ev.header.type = CLAP_EVENT_PARAM_VALUE;
      ev.header.space_id = CLAP_CORE_EVENT_SPACE_ID;
      ev.header.flags = 0;
      ev.header.size = sizeof (ev);
      ev.param_id = clap_id_val;
      ev.cookie = adapter.info.cookie;
      ev.port_index = 0;
      ev.key = -1;
      ev.channel = -1;
      ev.note_id = -1;
      ev.value = clap_value;
      evIn_.push (&ev.header);
    }
}

void
ClapPlugin::ClapPluginImpl::generateAllParamInputEvents ()
{
  // Main thread path (paramFlush): send all base values.
  for (const auto &[zrythm_param, clap_id_val] : owner_.zrythm_to_clap_)
    {
      auto it = clap_params_.find (clap_id_val);
      if (it == clap_params_.end ())
        continue;

      const auto  &adapter = it->second;
      const auto   range = zrythm_param->range ();
      const float  zrythm_value = zrythm_param->baseValue ();
      const double clap_value = range.convertFrom0To1 (zrythm_value);

      clap_event_param_value ev{};
      ev.header.time = 0;
      ev.header.type = CLAP_EVENT_PARAM_VALUE;
      ev.header.space_id = CLAP_CORE_EVENT_SPACE_ID;
      ev.header.flags = 0;
      ev.header.size = sizeof (ev);
      ev.param_id = clap_id_val;
      ev.cookie = adapter.info.cookie;
      ev.port_index = 0;
      ev.key = -1;
      ev.channel = -1;
      ev.note_id = -1;
      ev.value = clap_value;
      evIn_.push (&ev.header);
    }
}

void
ClapPlugin::ClapPluginImpl::generateMidiInputEvents () noexcept
{
  // Fill MIDI events from the MIDI input port
  if (owner_.get_descriptor ().num_midi_ins_ > 0)
    {
      for (const auto &ev : owner_.midi_in_port_->midi_events_.active_events_)
        {
          clap_event_midi clap_ev{};
          clap_ev.header.time = ev.time_.in<uint32_t> (units::samples);
          clap_ev.header.type = CLAP_EVENT_MIDI;
          clap_ev.header.space_id = CLAP_CORE_EVENT_SPACE_ID;
          clap_ev.header.flags = 0;
          clap_ev.header.size = sizeof (clap_ev);
          clap_ev.port_index = 0;
          std::ranges::copy (ev.raw_buffer_, clap_ev.data);
          evIn_.push (&clap_ev.header);
        }
    }
}

void
ClapPlugin::ClapPluginImpl::handlePluginOutputEvents () noexcept
{
  for (uint32_t i = 0; i < evOut_.size (); ++i)
    {
      auto * h = evOut_.get (i);
      switch (h->type)
        {
        case CLAP_EVENT_PARAM_VALUE:
          {
            const auto * ev =
              reinterpret_cast<const clap_event_param_value *> (h); // NOLINT
            auto it = clap_params_.find (ev->param_id);
            if (it == clap_params_.end ())
              break;

            const auto &adapter = it->second;
            auto *      zrythm_param = adapter.zrythm_param;
            if (zrythm_param == nullptr)
              break;

            const size_t param_index = adapter.param_index;
            assert (param_index < owner_.param_sync_.entries.size ());
            const auto  range = zrythm_param->range ();
            const float normalized =
              range.convertTo0To1 (static_cast<float> (ev->value));

            auto &entry = owner_.param_sync_.entries[param_index];
            entry.pending_value.store (normalized, std::memory_order_release);
            entry.last_from_plugin = normalized;
            break;
          }
        default:
          break;
        }
    }
}

void
ClapPlugin::requestRestart () noexcept
{
  pimpl_->scheduleRestart_.store (true, std::memory_order_release);
}

void
ClapPlugin::requestProcess () noexcept
{
  pimpl_->scheduleProcess_.store (true, std::memory_order_release);
}

void
ClapPlugin::requestCallback () noexcept
{
  pimpl_->scheduleMainThreadCallback_.store (true, std::memory_order_release);
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
      break;
    case CLAP_LOG_ERROR:
    default:
      z_error ("{}", message);
    }
}

void
ClapPlugin::create_ports_from_clap_plugin ()
{
  assert (is_main_thread);
  assert (!pimpl_->isPluginActive ());

  if (pimpl_->plugin_->canUseNotePorts ())
    {
      const auto midi_in_ports = pimpl_->plugin_->notePortsCount (true);
      const auto midi_out_ports = pimpl_->plugin_->notePortsCount (false);
      for (const auto i : std::views::iota (0u, midi_in_ports))
        {
          auto port_ref =
            dependencies ().port_registry_.create_object<dsp::MidiPort> (
              utils::Utf8String::from_utf8_encoded_string (
                fmt::format ("MIDI Input {}", i + 1)),
              dsp::PortFlow::Input);
          add_input_port (port_ref);
        }
      for (const auto i : std::views::iota (0u, midi_out_ports))
        {
          auto port_ref =
            dependencies ().port_registry_.create_object<dsp::MidiPort> (
              utils::Utf8String::from_utf8_encoded_string (
                fmt::format ("MIDI Output {}", i + 1)),
              dsp::PortFlow::Output);
          add_output_port (port_ref);
        }
    }

  if (pimpl_->plugin_->canUseAudioPorts ())
    {
      const auto create_port = [&] (bool is_input, auto index) {
        clap_audio_port_info_t nfo{};
        pimpl_->plugin_->audioPortsGet (index, is_input, &nfo);
        const dsp::AudioPort::BusLayout layout = [nfo] () {
          if (nfo.port_type != nullptr)
            {
              if (std::string (nfo.port_type) == std::string (CLAP_PORT_STEREO))
                {
                  return dsp::AudioPort::BusLayout::Stereo;
                }
              if (std::string (nfo.port_type) == std::string (CLAP_PORT_MONO))
                {
                  return dsp::AudioPort::BusLayout::Mono;
                }
            }
          return dsp::AudioPort::BusLayout{};
        }();
        auto port_ref =
          dependencies ().port_registry_.create_object<dsp::AudioPort> (
            utils::Utf8String::from_utf8_encoded_string (nfo.name),
            is_input ? dsp::PortFlow::Input : dsp::PortFlow::Output, layout,
            nfo.channel_count,
            index == 0
              ? dsp::AudioPort::Purpose::Main
              : dsp::AudioPort::Purpose::Sidechain);
        if (is_input)
          {
            add_input_port (port_ref);
          }
        else
          {
            add_output_port (port_ref);
          }
      };

      const auto audio_in_ports = pimpl_->plugin_->audioPortsCount (true);
      const auto audio_out_ports = pimpl_->plugin_->audioPortsCount (false);
      for (const auto i : std::views::iota (0u, audio_in_ports))
        {
          create_port (true, i);
        }
      for (const auto i : std::views::iota (0u, audio_out_ports))
        {
          create_port (false, i);
        }
    }
}

void
ClapPlugin::ClapPluginImpl::setup_audio_ports_for_processing (
  units::sample_u32_t block_size)
{
  const auto setup_for_direction = [&] (bool is_input) {
    auto &audio_bufs = is_input ? audio_in_bufs_ : audio_out_bufs_;
    auto &audio_clap_bufs =
      is_input ? audio_in_clap_bufs_ : audio_out_clap_bufs_;
    audio_bufs.resize (plugin_->audioPortsCount (is_input));
    audio_clap_bufs.resize (plugin_->audioPortsCount (is_input));
    for (const auto &[i, juce_buf] : utils::views::enumerate (audio_bufs))
      {
        clap_audio_port_info_t nfo;
        plugin_->audioPortsGet (i, is_input, &nfo);
        juce_buf.setSize (
          static_cast<int> (nfo.channel_count),
          block_size.in<int> (units::samples));
        auto &clap_buf = audio_clap_bufs.at (i);
        clap_buf.channel_count = nfo.channel_count;
        clap_buf.data32 =
          const_cast<float **> (juce_buf.getArrayOfWritePointers ());
        clap_buf.data64 = nullptr;
        clap_buf.constant_mask = 0;
        clap_buf.latency = 0;
      }
  };

  setup_for_direction (true);
  setup_for_direction (false);
}

bool
ClapPlugin::ClapPluginImpl::checkValidParamValue (
  const ClapParamAdapter &adapter,
  double                  value)
{
  assert (is_main_thread);

  if (adapter.info.min_value > value || value > adapter.info.max_value)
    {
      z_warning (
        "Invalid value for param id: {}, name: '{}'; value: {}", adapter.id,
        adapter.info.name, value);
      return false;
    }

  return true;
}

void
ClapPlugin::paramsRescan (uint32_t flags) noexcept
{
  assert (is_main_thread);

  if (!pimpl_->plugin_->canUseParams ())
    return;

  // 1. it is forbidden to use CLAP_PARAM_RESCAN_ALL if the plugin is active
  // Note: not an assertion because some plugins fail this check
  z_warn_if_fail (
    !pimpl_->isPluginActive () || !(flags & CLAP_PARAM_RESCAN_ALL));

  // 2. Build a lookup from ProcessorParameter* to its index in
  //    get_parameters(). This avoids repeated O(n) linear scans.
  std::unordered_map<dsp::ProcessorParameter *, size_t> param_index_map;
  for (
    const auto &[idx, param_ref] : utils::views::enumerate (get_parameters ()))
    {
      param_index_map[param_ref.get_object_as<dsp::ProcessorParameter> ()] =
        static_cast<size_t> (idx);
    }

  // 3. scan the params.
  auto                        count = pimpl_->plugin_->paramsCount ();
  std::unordered_set<clap_id> param_ids (count * 2);

  for (const auto i : std::views::iota (0u, count))
    {
      clap_param_info info{};
      z_return_if_fail (pimpl_->plugin_->paramsGetInfo (i, &info));

      assert (info.id != CLAP_INVALID_ID);

      // check that the parameter is not declared twice
      assert (!param_ids.contains (info.id));
      param_ids.insert (info.id);

      auto it = pimpl_->clap_params_.find (info.id);

      if (it == pimpl_->clap_params_.end ())
        {
          assert ((flags & CLAP_PARAM_RESCAN_ALL) != 0u);

          double                           value = pimpl_->getParamValue (info);
          ClapPluginImpl::ClapParamAdapter adapter{
            .id = info.id, .info = info, .zrythm_param = nullptr, .param_index = 0
          };
          const bool value_valid = pimpl_->checkValidParamValue (adapter, value);

          // Look up or create a ProcessorParameter
          const auto unique_id = dsp::ProcessorParameter::UniqueId (
            utils::Utf8String::from_utf8_encoded_string (
              std::to_string (info.id)));

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
              dsp::ParameterRange range{
                dsp::ParameterRange::Type::Linear,
                static_cast<float> (info.min_value),
                static_cast<float> (info.max_value), 0.f,
                static_cast<float> (info.default_value)
              };

              if (info.flags & CLAP_PARAM_IS_STEPPED)
                {
                  if (info.flags & CLAP_PARAM_IS_BYPASS)
                    {
                      range = dsp::ParameterRange::make_toggle (
                        info.default_value > 0.5);
                    }
                  else
                    {
                      range = dsp::ParameterRange{
                        dsp::ParameterRange::Type::Integer,
                        static_cast<float> (info.min_value),
                        static_cast<float> (info.max_value), 0.f,
                        static_cast<float> (info.default_value)
                      };
                    }
                }

              auto zrythm_param_ref =
                dependencies ()
                  .param_registry_.create_object<dsp::ProcessorParameter> (
                    dependencies ().port_registry_, unique_id, range,
                    utils::Utf8String::from_utf8_encoded_string (info.name));
              add_parameter (zrythm_param_ref);
              zrythm_param =
                zrythm_param_ref.get_object_as<dsp::ProcessorParameter> ();
              zrythm_param->set_automatable (
                (info.flags & CLAP_PARAM_IS_AUTOMATABLE) != 0);

              // Update index map for the newly added parameter
              param_index_map[zrythm_param] = get_parameters ().size () - 1;

              if (value_valid)
                {
                  const auto normalized_live =
                    zrythm_param->range ().convertTo0To1 (
                      static_cast<float> (value));
                  zrythm_param->setBaseValue (normalized_live);
                }
            }

          adapter.zrythm_param = zrythm_param;
          auto index_it = param_index_map.find (zrythm_param);
          assert (index_it != param_index_map.end ());
          adapter.param_index = index_it->second;

          pimpl_->clap_params_.insert_or_assign (info.id, adapter);
          zrythm_to_clap_[zrythm_param] = info.id;
        }
      else
        {
          auto &adapter = it->second;

          // Check if param info changed
          const auto &old_info = adapter.info;
          bool        info_changed = !(
            old_info.cookie == info.cookie
            && old_info.default_value == info.default_value
            && old_info.max_value == info.max_value
            && old_info.min_value == info.min_value
            && old_info.flags == info.flags && old_info.id == info.id
            && std::strncmp (old_info.name, info.name, sizeof (info.name)) == 0
            && std::strncmp (old_info.module, info.module, sizeof (info.module))
                 == 0);

          if (info_changed)
            {
              z_warn_if_fail (pimpl_->clapParamsRescanMayInfoChange (flags));
              constexpr uint32_t critical_flags =
                CLAP_PARAM_IS_AUTOMATABLE | CLAP_PARAM_IS_AUTOMATABLE_PER_NOTE_ID
                | CLAP_PARAM_IS_AUTOMATABLE_PER_KEY
                | CLAP_PARAM_IS_AUTOMATABLE_PER_CHANNEL
                | CLAP_PARAM_IS_AUTOMATABLE_PER_PORT | CLAP_PARAM_IS_MODULATABLE
                | CLAP_PARAM_IS_MODULATABLE_PER_NOTE_ID
                | CLAP_PARAM_IS_MODULATABLE_PER_KEY
                | CLAP_PARAM_IS_MODULATABLE_PER_CHANNEL
                | CLAP_PARAM_IS_MODULATABLE_PER_PORT | CLAP_PARAM_IS_READONLY
                | CLAP_PARAM_REQUIRES_PROCESS;
              assert (
                ((flags & CLAP_PARAM_RESCAN_ALL) != 0u)
                || ((old_info.flags & critical_flags)
                      == (info.flags & critical_flags)
                    && old_info.min_value == info.min_value
                    && old_info.max_value == info.max_value));
              adapter.info = info;
            }

          // Check if value changed and sync to ProcessorParameter
          double value = pimpl_->getParamValue (info);
          if (adapter.zrythm_param != nullptr)
            {
              if (pimpl_->checkValidParamValue (adapter, value))
                {
                  const auto range = adapter.zrythm_param->range ();
                  const auto current_normalized =
                    adapter.zrythm_param->baseValue ();
                  const auto new_normalized =
                    range.convertTo0To1 (static_cast<float> (value));
                  if (!utils::math::floats_near (
                        current_normalized, new_normalized, 0.0001f))
                    {
                      assert (pimpl_->clapParamsRescanMayValueChange (flags));
                      adapter.zrythm_param->setBaseValue (new_normalized);
                    }
                }
            }
        }
    }

  // remove parameters which are gone
  for (
    auto it = pimpl_->clap_params_.begin (); it != pimpl_->clap_params_.end ();)
    {
      if (param_ids.contains (it->first))
        ++it;
      else
        {
          assert ((flags & CLAP_PARAM_RESCAN_ALL) != 0u);
          if (it->second.zrythm_param != nullptr)
            {
              zrythm_to_clap_.erase (it->second.zrythm_param);
            }
          it = pimpl_->clap_params_.erase (it);
        }
    }

  // Defensive: rebuild param_index for all remaining adapters in case
  // get_parameters() ordering ever changes.
  for (auto &[clap_id_val, adapter] : pimpl_->clap_params_)
    {
      if (adapter.zrythm_param == nullptr)
        continue;
      auto index_it = param_index_map.find (adapter.zrythm_param);
      if (index_it != param_index_map.end ())
        adapter.param_index = index_it->second;
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
ClapPlugin::ClapPluginImpl::paramFlushOnMainThread ()
{
  assert (is_main_thread);

  assert (!isPluginActive ());

  scheduleParamFlush_.store (false, std::memory_order_release);

  evIn_.clear ();
  evOut_.clear ();

  generateAllParamInputEvents ();

  if (plugin_->canUseParams ())
    plugin_->paramsFlush (evIn_.clapInputEvents (), evOut_.clapOutputEvents ());

  z_trace ("CLAP: paramFlushOnMainThread got {} output events", evOut_.size ());
  handlePluginOutputEvents ();

  evOut_.clear ();
  owner_.flush_plugin_values ();
}

void
ClapPlugin::paramsRequestFlush () noexcept
{
  if (!pimpl_->isPluginActive () && threadCheckIsMainThread ())
    {
      pimpl_->paramFlushOnMainThread ();
      return;
    }

  pimpl_->scheduleParamFlush_.store (true, std::memory_order_release);
}

double
ClapPlugin::ClapPluginImpl::getParamValue (const clap_param_info &info)
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
ClapPlugin::ClapPluginImpl::setPluginState (PluginState state)
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

QByteArray
ClapPlugin::save_state_to_byte_array () const
{
  if (!pimpl_ || !pimpl_->plugin_ || !pimpl_->plugin_->canUseState ())
    return {};

  QByteArray     state_data;
  clap_ostream_t ostream{};
  ostream.ctx = &state_data;
  ostream.write =
    +[] (const clap_ostream * stream, const void * buffer, uint64_t size)
    -> int64_t {
    auto * data = static_cast<QByteArray *> (stream->ctx);
    data->append (
      static_cast<const char *> (buffer), static_cast<qsizetype> (size));
    return static_cast<int64_t> (size);
  };
  pimpl_->plugin_->stateSave (&ostream);
  return state_data;
}

std::string
ClapPlugin::save_state_impl () const
{
  auto data = save_state_to_byte_array ();
  return data.toBase64 ().toStdString ();
}

void
ClapPlugin::load_state_impl (const std::string &base64_state)
{
  state_to_apply_ =
    QByteArray::fromBase64 (QByteArray::fromStdString (base64_state));
}

void
ClapPlugin::apply_state_from_byte_array (const QByteArray &data)
{
  if (!pimpl_ || !pimpl_->plugin_ || !pimpl_->plugin_->canUseState ())
    return;

  QByteArray     mutable_data = data;
  clap_istream_t istream{};
  istream.ctx = &mutable_data;
  istream.read =
    +[] (const clap_istream * stream, void * buffer, uint64_t size) -> int64_t {
    auto * d = static_cast<QByteArray *> (stream->ctx);
    if (d->isEmpty ())
      return 0;
    size = std::min<uint64_t> (size, d->size ());
    std::memcpy (buffer, d->constData (), size);
    d->remove (0, static_cast<qsizetype> (size));
    return static_cast<int64_t> (size);
  };
  pimpl_->plugin_->stateLoad (&istream);
}

void
to_json (nlohmann::json &j, const ClapPlugin &p)
{
  to_json (j, static_cast<const Plugin &> (p));
  auto state = p.save_state_impl ();
  if (!state.empty ())
    j[ClapPlugin::kStateKey] = std::move (state);
}

void
from_json (const nlohmann::json &j, ClapPlugin &p)
{
  // State must be deserialized first, because the Plugin deserialization
  // may cause an instantiation
  if (j.contains (ClapPlugin::kStateKey))
    p.load_state_impl (j[ClapPlugin::kStateKey].get<std::string> ());

  from_json (j, static_cast<Plugin &> (p));
}

} // namespace zrythm::plugins
