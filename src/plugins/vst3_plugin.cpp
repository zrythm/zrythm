// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense
/*
 * VST3 hosting implemented against the VST3 SDK (MIT-licensed):
 * - public.sdk/source/vst/hosting/module.h (Module, PluginFactory, ClassInfo)
 * - public.sdk/source/vst/hosting/plugprovider.h (PlugProvider)
 * - public.sdk/source/vst/hosting/hostclasses.h (PlugInterfaceSupport,
 *   HostMessage, HostAttributeList)
 * - public.sdk/source/vst/hosting/processdata.h (HostProcessData)
 * - public.sdk/source/vst/hosting/eventlist.h (EventList)
 * - public.sdk/source/vst/vstpresetfile.h (PresetFile)
 * - public.sdk/source/vst/utility/memoryibstream.h (ResizableMemoryIBStream)
 * - public.sdk/source/vst/utility/stringconvert.h (StringConvert)
 * - pluginterfaces/vst headers (IComponent, IAudioProcessor, IEditController,
 *   IComponentHandler, ProcessSetup, ProcessData, ProcessContext, Event)
 */

#include "zrythm-config.h"

#include <algorithm>
#include <cmath>
#include <utility>

#include "dsp/midi_event.h"
#include "plugins/plugin_run_loop.h"
#include "plugins/vst3_plugin.h"
#include "plugins/vst3_plugin_format.h"
#include "utils/logger.h"
#include "utils/math_utils.h"
#include "utils/serialization.h"
#include "utils/views.h"

#include <fmt/format.h>

#if defined(__has_feature) && __has_feature(realtime_sanitizer)
#  include <sanitizer/rtsan_interface.h>
#endif

#include <QCoreApplication>

#include <pluginterfaces/gui/iplugview.h>
#include <pluginterfaces/gui/iplugviewcontentscalesupport.h>
#include <pluginterfaces/vst/ivstaudioprocessor.h>
#include <pluginterfaces/vst/ivstcomponent.h>
#include <pluginterfaces/vst/ivsteditcontroller.h>
#include <pluginterfaces/vst/ivstevents.h>
#include <pluginterfaces/vst/ivstmidicontrollers.h>
#include <pluginterfaces/vst/ivstparameterchanges.h>
#include <pluginterfaces/vst/ivstprocesscontext.h>
#include <public.sdk/source/vst/hosting/eventlist.h>
#include <public.sdk/source/vst/hosting/hostclasses.h>
#include <public.sdk/source/vst/hosting/plugprovider.h>
#include <public.sdk/source/vst/hosting/processdata.h>
#include <public.sdk/source/vst/utility/memoryibstream.h>
#include <public.sdk/source/vst/utility/stringconvert.h>
#include <public.sdk/source/vst/vstpresetfile.h>

namespace zrythm::plugins
{

namespace
{
using namespace Steinberg;

/**
 * @brief IHostApplication implementation with proper reference counting.
 *
 * Adapted from the SDK's Vst::HostApplication
 * (public.sdk/source/vst/hosting/hostclasses.{h,cpp}, MIT-licensed), whose
 * addRef()/release() are intentional no-ops designed for a process-wide
 * singleton (see the audiohost/editorhost samples). This version uses
 * U::Implements reference counting so it is destroyed cleanly.
 *
 * Also implements Linux::IRunLoop: the VST3 docs require hosts to provide a
 * run loop to plug-ins on GNU/Linux via the host context passed to
 * IPlugFactory3::setHostContext (which our PlugProvider setup forwards this
 * object to) and/or the IPlugFrame. Recent VSTGUI versions query the run loop
 * from the host context (see the SDK's setupVSTGUIRunloop helper).
 */
class Vst3HostApplication final
#if SMTG_OS_LINUX
    : public U::Implements<U::Directly<Vst::IHostApplication, Linux::IRunLoop>>
#else
    : public U::Implements<U::Directly<Vst::IHostApplication>>
#endif
{
public:
  Vst3HostApplication ()
  {
    plug_interface_support_ = owned (new Vst::PlugInterfaceSupport);
  }

  tresult PLUGIN_API getName (Vst::String128 name) override
  {
    return Vst::StringConvert::convert ("Zrythm", name) ? kResultTrue : kInternalError;
  }

  tresult PLUGIN_API createInstance (TUID cid, TUID _iid, void ** obj) override
  {
    if (
      FUnknownPrivate::iidEqual (cid, Vst::IMessage::iid)
      && FUnknownPrivate::iidEqual (_iid, Vst::IMessage::iid))
      {
        *obj = new Vst::HostMessage;
        return kResultTrue;
      }
    if (
      FUnknownPrivate::iidEqual (cid, Vst::IAttributeList::iid)
      && FUnknownPrivate::iidEqual (_iid, Vst::IAttributeList::iid))
      {
        if (auto al = Vst::HostAttributeList::make ())
          {
            *obj = al.take ();
            return kResultTrue;
          }
        return kOutOfMemory;
      }
    *obj = nullptr;
    return kResultFalse;
  }

  tresult PLUGIN_API queryInterface (const TUID _iid, void ** obj) override
  {
    QUERY_INTERFACE (_iid, obj, FUnknown::iid, Vst::IHostApplication)
    QUERY_INTERFACE (
      _iid, obj, Vst::IHostApplication::iid, Vst::IHostApplication)
#if SMTG_OS_LINUX
    QUERY_INTERFACE (_iid, obj, Linux::IRunLoop::iid, Linux::IRunLoop)
#endif
    if (
      plug_interface_support_ != nullptr
      && plug_interface_support_->queryInterface (_iid, obj) == kResultTrue)
      {
        return kResultOk;
      }
    *obj = nullptr;
    return kResultFalse;
  }

#if SMTG_OS_LINUX
  tresult PLUGIN_API registerEventHandler (
    Linux::IEventHandler * handler,
    Linux::FileDescriptor  fd) override
  {
    if (handler == nullptr)
      return kInvalidArgument;
    const auto token = run_loop ()->register_fd (
      static_cast<int> (fd), true, false,
      [handler, fd] (bool) { handler->onFDIsSet (fd); });
    event_handler_tokens_.insert_or_assign (handler, token);
    return kResultOk;
  }

  tresult PLUGIN_API
  unregisterEventHandler (Linux::IEventHandler * handler) override
  {
    const auto it = event_handler_tokens_.find (handler);
    if (it == event_handler_tokens_.end ())
      return kResultFalse;
    run_loop ()->unregister_fd (it->second);
    event_handler_tokens_.erase (it);
    return kResultOk;
  }

  tresult PLUGIN_API registerTimer (
    Linux::ITimerHandler * handler,
    Linux::TimerInterval   milliseconds) override
  {
    if (handler == nullptr)
      return kInvalidArgument;
    const auto token = run_loop ()->register_timer (
      std::chrono::milliseconds{ milliseconds },
      [handler] { handler->onTimer (); });
    timer_handler_tokens_.insert_or_assign (handler, token);
    return kResultOk;
  }

  tresult PLUGIN_API unregisterTimer (Linux::ITimerHandler * handler) override
  {
    const auto it = timer_handler_tokens_.find (handler);
    if (it == timer_handler_tokens_.end ())
      return kResultFalse;
    run_loop ()->unregister_timer (it->second);
    timer_handler_tokens_.erase (it);
    return kResultOk;
  }

  /**
   * @brief Shared run loop used for all IRunLoop registrations.
   *
   * Parented to the QCoreApplication so it is destroyed before application
   * teardown completes (this singleton outlives it via static storage).
   * Main thread only.
   */
  PluginRunLoop * run_loop ()
  {
    if (run_loop_ == nullptr)
      run_loop_ = new PluginRunLoop (QCoreApplication::instance ());
    return run_loop_;
  }
#endif // SMTG_OS_LINUX

private:
  IPtr<Vst::PlugInterfaceSupport> plug_interface_support_;

#if SMTG_OS_LINUX
  /** Owned by the QCoreApplication. */
  PluginRunLoop * run_loop_ = nullptr;

  std::unordered_map<Linux::IEventHandler *, PluginRunLoop::Token>
    event_handler_tokens_;
  std::unordered_map<Linux::ITimerHandler *, PluginRunLoop::Token>
    timer_handler_tokens_;
#endif // SMTG_OS_LINUX
};

/**
 * @brief Returns the shared process-wide host context.
 *
 * Mirrors the SDK samples' lifetime model (a single host context shared by
 * all plugin instances).
 */
Vst3HostApplication *
shared_host_application ()
{
  static const auto host_app = owned (new Vst3HostApplication);
  return host_app.get ();
}

/**
 * @brief Preallocated IParamValueQueue that never allocates on the audio
 * thread.
 *
 * The SDK's VST3::Hosting::ParameterValueQueue grows a std::vector in
 * addPoint(); this replacement has fixed capacity instead (based on the
 * IParamValueQueue interface contract in
 * pluginterfaces/vst/ivsteditcontroller.h).
 */
class Vst3ParamValueQueue final
    : public U::Implements<U::Directly<Vst::IParamValueQueue>>
{
public:
  void reset (Vst::ParamID id)
  {
    param_id_ = id;
    point_count_ = 0;
  }

  Vst::ParamID PLUGIN_API getParameterId () override { return param_id_; }
  int32 PLUGIN_API        getPointCount () override { return point_count_; }
  tresult PLUGIN_API
  getPoint (int32 index, int32 &sampleOffset, Vst::ParamValue &value) override
  {
    if (index < 0 || index >= point_count_)
      return kResultFalse;
    sampleOffset = points_[static_cast<size_t> (index)].sample_offset;
    value = points_[static_cast<size_t> (index)].value;
    return kResultTrue;
  }
  tresult PLUGIN_API
  addPoint (int32 sampleOffset, Vst::ParamValue value, int32 &index) override
  {
    for (const auto i : std::views::iota (0, point_count_))
      {
        if (points_[static_cast<size_t> (i)].sample_offset == sampleOffset)
          {
            points_[static_cast<size_t> (i)].value = value;
            index = i;
            return kResultTrue;
          }
      }
    if (point_count_ >= static_cast<int32> (kMaxPoints))
      return kResultFalse;
    index = point_count_++;
    points_[static_cast<size_t> (index)] = { sampleOffset, value };
    return kResultTrue;
  }

private:
  static constexpr size_t kMaxPoints = 8;
  struct Point
  {
    int32           sample_offset;
    Vst::ParamValue value;
  };

  Vst::ParamID                  param_id_ = 0;
  int32                         point_count_ = 0;
  std::array<Point, kMaxPoints> points_{};
};

/**
 * @brief Preallocated IParameterChanges with one queue per parameter.
 *
 * The SDK's VST3::Hosting::ParameterChanges allocates in addParameterData()
 * once its initial capacity is exceeded; this replacement is sized up-front
 * on the main thread and never allocates during processing (based on the
 * IParameterChanges interface contract in
 * pluginterfaces/vst/ivsteditcontroller.h).
 */
class Vst3ParameterChanges final
    : public U::Implements<U::Directly<Vst::IParameterChanges>>
{
public:
  /**
   * @brief Creates the queues. Main thread only.
   */
  void prepare (size_t param_count)
  {
    queues_.clear ();
    queues_.reserve (param_count);
    for (const auto _ : std::views::iota (size_t{ 0 }, param_count))
      {
        queues_.emplace_back (owned (new Vst3ParamValueQueue));
      }
    used_count_ = 0;
  }

  /**
   * @brief Marks all queues as unused. Audio-thread safe.
   */
  void clear_queues ()
  {
    for (const auto i : std::views::iota (0, used_count_))
      {
        queues_[static_cast<size_t> (i)]->reset (Vst::kNoParamId);
      }
    used_count_ = 0;
  }

  int32 PLUGIN_API getParameterCount () override { return used_count_; }
  Vst::IParamValueQueue * PLUGIN_API getParameterData (int32 index) override
  {
    if (index < 0 || index >= used_count_)
      return nullptr;
    return queues_[static_cast<size_t> (index)];
  }
  Vst::IParamValueQueue * PLUGIN_API
  addParameterData (const Vst::ParamID &pid, int32 &index) override
  {
    for (const auto i : std::views::iota (0, used_count_))
      {
        if (queues_[static_cast<size_t> (i)]->getParameterId () == pid)
          {
            index = i;
            return queues_[static_cast<size_t> (i)];
          }
      }
    if (used_count_ >= static_cast<int32> (queues_.size ()))
      {
        index = -1;
        return nullptr;
      }
    auto * queue = queues_[static_cast<size_t> (used_count_)].get ();
    queue->reset (pid);
    index = used_count_++;
    return queue;
  }

private:
  std::vector<IPtr<Vst3ParamValueQueue>> queues_;
  int32                                  used_count_ = 0;
};

/**
 * @brief IComponentHandler implementation bridging plugin-initiated edits to
 * Zrythm parameters.
 */
class Vst3ComponentHandler final
    : public U::Implements<U::Directly<Vst::IComponentHandler>>
{
public:
  using PerformEditCallback =
    std::function<void (Vst::ParamID, Vst::ParamValue)>;

  explicit Vst3ComponentHandler (PerformEditCallback perform_edit_cb)
      : perform_edit_cb_ (std::move (perform_edit_cb))
  {
  }

  tresult PLUGIN_API beginEdit (Vst::ParamID) override { return kResultOk; }
  tresult PLUGIN_API
  performEdit (Vst::ParamID id, Vst::ParamValue value) override
  {
    // May arrive on any thread; the callback must be thread-safe
    perform_edit_cb_ (id, value);
    return kResultOk;
  }
  tresult PLUGIN_API endEdit (Vst::ParamID) override { return kResultOk; }
  tresult PLUGIN_API restartComponent (int32 flags) override
  {
    if ((flags & Vst::RestartFlags::kLatencyChanged) != 0)
      {
        latency_changed_.store (true, std::memory_order_release);
      }
    return kResultOk;
  }

  std::atomic<bool> latency_changed_{ false };

private:
  PerformEditCallback perform_edit_cb_;
};

/**
 * @brief IPlugFrame implementation handling plugin-initiated view resizes.
 *
 * Also implements Linux::IRunLoop (forwarding to the shared host
 * application's run loop) since plug-ins may query the run loop from the
 * frame instead of the host context (both are documented locations).
 */
class Vst3PlugFrame final
#if SMTG_OS_LINUX
    : public U::Implements<U::Directly<IPlugFrame, Linux::IRunLoop>>
#else
    : public U::Implements<U::Directly<IPlugFrame>>
#endif
{
public:
  /**
   * @brief Callback to resize the host window, taking logical-pixel sizes.
   */
  using ResizeCallback = std::function<void (int width, int height)>;

  Vst3PlugFrame (IPlugView * view, ResizeCallback resize_cb)
      : view_ (view), resize_cb_ (std::move (resize_cb))
  {
  }

  tresult PLUGIN_API resizeView (IPlugView * view, ViewRect * newSize) override
  {
    if (view != view_ || newSize == nullptr)
      return kInvalidArgument;

    // Let the plug-in adjust the requested size, then resize the host window
    // and notify the plug-in of the final size (per the IPlugView sizing
    // contract)
    view_->checkSizeConstraint (newSize);
    ViewRect current{};
    view_->getSize (&current);
    if (
      current.getWidth () == newSize->getWidth ()
      && current.getHeight () == newSize->getHeight ())
      {
        return kResultOk;
      }
    resize_cb_ (newSize->getWidth (), newSize->getHeight ());
    return view_->onSize (newSize);
  }

#if SMTG_OS_LINUX
  tresult PLUGIN_API registerEventHandler (
    Linux::IEventHandler * handler,
    Linux::FileDescriptor  fd) override
  {
    return shared_host_application ()->registerEventHandler (handler, fd);
  }
  tresult PLUGIN_API
  unregisterEventHandler (Linux::IEventHandler * handler) override
  {
    return shared_host_application ()->unregisterEventHandler (handler);
  }
  tresult PLUGIN_API registerTimer (
    Linux::ITimerHandler * handler,
    Linux::TimerInterval   milliseconds) override
  {
    return shared_host_application ()->registerTimer (handler, milliseconds);
  }
  tresult PLUGIN_API unregisterTimer (Linux::ITimerHandler * handler) override
  {
    return shared_host_application ()->unregisterTimer (handler);
  }
#endif // SMTG_OS_LINUX

private:
  IPlugView *    view_;
  ResizeCallback resize_cb_;
};

/**
 * @brief Converts a VST3 view size to a host window size.
 *
 * ViewRect coordinates are physical pixels on Windows/GNU/Linux and logical
 * units on macOS (per iplugview.h); JUCE window sizes are logical pixels.
 */
static std::pair<int, int>
view_size_to_window_size (int width, int height, float scale_factor)
{
#if defined(Q_OS_MACOS)
  return { width, height };
#else
  const auto scale = std::max (scale_factor, 0.01f);
  return {
    static_cast<int> (std::lround (width / scale)),
    static_cast<int> (std::lround (height / scale))
  };
#endif
}
} // namespace

class Vst3Plugin::Vst3PluginImpl
{
  friend class Vst3Plugin;

public:
  Vst3PluginImpl (Vst3Plugin &owner, PluginHostWindowFactory host_window_factory)
      : owner_ (owner), host_window_factory_ (std::move (host_window_factory))
  {
  }

private:
  Vst3Plugin             &owner_;
  PluginHostWindowFactory host_window_factory_;

  struct Vst3ParamAdapter
  {
    Vst::ParamID              id;
    dsp::ProcessorParameter * zrythm_param = nullptr;
    size_t                    param_index = 0;
  };

  VST3::Hosting::Module::Ptr                         module_;
  std::unique_ptr<Vst::PlugProvider>                 plug_provider_;
  FUID                                               class_id_{};
  IPtr<Vst::IComponent>                              component_;
  IPtr<Vst::IEditController>                         controller_;
  IPtr<Vst::IAudioProcessor>                         processor_;
  IPtr<Vst::IHostApplication>                        host_app_;
  IPtr<Vst3ComponentHandler>                         component_handler_;
  Vst::HostProcessData                               process_data_;
  Vst::EventList                                     input_events_{ 0 };
  Vst::EventList                                     output_events_{ 0 };
  Vst3ParameterChanges                               input_param_changes_{};
  Vst::ProcessContext                                process_context_{};
  std::unordered_map<Vst::ParamID, Vst3ParamAdapter> vst3_params_;
  std::unordered_map<dsp::ProcessorParameter *, Vst::ParamID> zrythm_to_vst3_;

  /**
   * MIDI CC -> ParamID translation table from the plugin's IMidiMapping
   * ([channel * kCountCtrlNumber + ctrlNumber], kNoParamId if unmapped).
   *
   * Params covered by it are not exposed as Zrythm parameters; MIDI CC
   * events are translated to parameter changes at process time instead.
   */
  std::array<Vst::ParamID, 16 * Vst::kCountCtrlNumber> midi_cc_param_ids_;
  std::unordered_set<Vst::ParamID>                     midi_cc_param_id_set_;

  /** Param flagged kIsProgramChange (if any) and its step count, used to
   * translate MIDI program change messages to parameter changes. */
  std::optional<Vst::ParamID> program_change_param_id_;
  int32                       program_change_step_count_ = 0;

  bool                processing_active_ = false;
  units::sample_u32_t latency_ = units::samples (0u);

  /**
   * Editor view created by IEditController::createView (null if the plugin
   * has no editor). Created lazily on the first hasNativeUi() call and kept
   * for the controller's lifetime (per the IEditController::createView
   * contract).
   */
  IPtr<IPlugView> view_;
  bool            view_check_done_ = false;

  /** Host window embedding the editor view, plus its plug frame. */
  std::unique_ptr<plugins::PluginHostWindow> editor_window_;
  IPtr<Vst3PlugFrame>                        plug_frame_;
  bool                                       view_attached_ = false;
};

Vst3Plugin::Vst3Plugin (
  utils::IObjectRegistry &registry,
  PluginHostWindowFactory host_window_factory,
  QObject *               parent)
    : Plugin (registry, parent),
      pimpl_ (
        std::make_unique<Vst3PluginImpl> (*this, std::move (host_window_factory)))
{
  connect (
    this, &Plugin::configurationChanged, this,
    &Vst3Plugin::on_configuration_changed);

  auto bypass_ref = generate_default_bypass_param ();
  add_parameter (bypass_ref);
  bypass_id_ = bypass_ref.id ();
  auto gain_ref = generate_default_gain_param ();
  add_parameter (gain_ref);
  gain_id_ = gain_ref.id ();
}

Vst3Plugin::~Vst3Plugin ()
{
  if (pimpl_ && pimpl_->module_)
    unload_current_plugin ();
}

void
Vst3Plugin::on_configuration_changed (
  PluginConfiguration * config,
  bool                  generateNewPluginPortsAndParams)
{
  const auto &path = std::get<std::filesystem::path> (
    configuration ()->descriptor ()->path_or_id_);
  const auto success = load_plugin (
    path, configuration ()->descriptor ()->unique_id_,
    generateNewPluginPortsAndParams);
  Q_EMIT instantiationFinished (
    success,
    success
      ? QString{}
      : tr ("Failed to load VST3 plugin from %1")
          .arg (utils::Utf8String::from_path (path).to_qstring ()));
}

bool
Vst3Plugin::hasNativeUi () const
{
  if (pimpl_->controller_ == nullptr)
    return false;

  if (!pimpl_->view_check_done_)
    {
      pimpl_->view_check_done_ = true;
      // createView transfers ownership of the returned reference
      pimpl_->view_ =
        owned (pimpl_->controller_->createView (Vst::ViewType::kEditor));
    }
  return pimpl_->view_ != nullptr;
}

void
Vst3Plugin::on_ui_visibility_changed ()
{
  if (uiVisible ())
    {
      if (pimpl_->view_attached_)
        pimpl_->editor_window_->setVisible (true);
      else
        show_editor ();
    }
  else if (pimpl_->view_attached_)
    {
      hide_editor ();
    }
}

void
Vst3Plugin::show_editor ()
{
  if (!hasNativeUi ())
    {
      setUiVisible (false);
      return;
    }
  if (pimpl_->view_attached_)
    return;

  pimpl_->editor_window_ = pimpl_->host_window_factory_ (*this);
  if (pimpl_->editor_window_ == nullptr)
    {
      z_warning ("VST3: no host window available for plugin editor");
      setUiVisible (false);
      return;
    }

  const auto platform_type = [] (plugins::WindowSystem window_system) {
    switch (window_system)
      {
      case plugins::WindowSystem::X11:
        return kPlatformTypeX11EmbedWindowID;
      case plugins::WindowSystem::Wayland:
        return kPlatformTypeWaylandSurfaceID;
      case plugins::WindowSystem::Cocoa:
        return kPlatformTypeNSView;
      case plugins::WindowSystem::Win32:
        return kPlatformTypeHWND;
      }
    std::unreachable ();
  }(pimpl_->editor_window_->windowSystem ());

  auto * view = pimpl_->view_.get ();
  if (view->isPlatformTypeSupported (platform_type) != kResultTrue)
    {
      z_warning ("VST3: plugin editor does not support {}", platform_type);
      pimpl_->editor_window_.reset ();
      setUiVisible (false);
      return;
    }

  const auto scale_factor = pimpl_->editor_window_->contentScaleFactor ();

  // The frame must be set before the scale factor and before attaching: the
  // plug-in may call IPlugFrame::resizeView() in response to either
  pimpl_->plug_frame_ = owned (
    new Vst3PlugFrame (view, [this, scale_factor] (int width, int height) {
      if (pimpl_->editor_window_ == nullptr)
        return;
      const auto [w, h] = view_size_to_window_size (width, height, scale_factor);
      pimpl_->editor_window_->setSize (w, h);
    }));
  view->setFrame (pimpl_->plug_frame_);

  if (auto scale_support = U::cast<IPlugViewContentScaleSupport> (view))
    {
      scale_support->setContentScaleFactor (scale_factor);
    }

  ViewRect size{};
  if (view->getSize (&size) == kResultOk)
    {
      const auto [w, h] = view_size_to_window_size (
        size.getWidth (), size.getHeight (), scale_factor);
      pimpl_->editor_window_->setSizeAndCenter (w, h);
    }

  pimpl_->editor_window_->setResizable (view->canResize () == kResultTrue);

  const auto embed_id = pimpl_->editor_window_->getEmbedWindowId ();

  if (
    view->attached (reinterpret_cast<void *> (embed_id), platform_type)
    != kResultOk)
    {
      z_warning ("VST3: failed to attach plugin editor view");
      view->setFrame (nullptr);
      pimpl_->plug_frame_.reset ();
      pimpl_->editor_window_.reset ();
      setUiVisible (false);
      return;
    }

  pimpl_->view_attached_ = true;
  pimpl_->editor_window_->setVisible (true);
  pimpl_->editor_window_->completeNativeEmbedding ();
}

void
Vst3Plugin::hide_editor ()
{
  if (!pimpl_->view_attached_)
    return;

  // Keep the native GUI alive while hidden: only unmap the host window. The
  // view is detached in unload_current_plugin()
  pimpl_->editor_window_->setVisible (false);
}

void
Vst3Plugin::detach_editor ()
{
  if (!pimpl_->view_attached_)
    return;

  pimpl_->view_->removed ();
  pimpl_->view_->setFrame (nullptr);
  pimpl_->plug_frame_.reset ();
  pimpl_->editor_window_.reset ();
  pimpl_->view_attached_ = false;
}

units::sample_u32_t
Vst3Plugin::get_single_playback_latency () const
{
  return pimpl_->latency_;
}

bool
Vst3Plugin::load_plugin (
  const std::filesystem::path &path,
  int64_t                      plugin_unique_id,
  bool                         generate_new_ports)
{
  if (pimpl_->module_ != nullptr)
    unload_current_plugin ();

  std::string error;
  auto        module = VST3::Hosting::Module::create (
    utils::Utf8String::from_path (path).str (), error);
  if (module == nullptr)
    {
      z_warning ("Failed to load VST3 module '{}': {}", path, error);
      return false;
    }

  // Find the class matching the scanned unique ID (hash of the TUID string),
  // falling back to name matching for projects saved before the native
  // scanner existed
  const auto &class_infos = module->getFactory ().classInfos ();
  auto        class_info_it =
    std::ranges::find_if (class_infos, [plugin_unique_id] (const auto &ci) {
      return Vst3PluginFormat::get_hash_for_range (ci.ID ().toString ())
             == plugin_unique_id;
    });
  if (class_info_it == class_infos.end ())
    {
      class_info_it = std::ranges::find_if (class_infos, [this] (const auto &ci) {
        return ci.name () == get_name ().str ();
      });
      if (class_info_it != class_infos.end ())
        {
          z_debug ("VST3: matched class by name (uid hash mismatch)");
        }
    }
  if (class_info_it == class_infos.end ())
    {
      z_warning ("VST3: no matching class found in module '{}'", path);
      return false;
    }

  pimpl_->host_app_ = shared_host_application ();
  Vst::PluginContextFactory::instance ().setPluginContext (pimpl_->host_app_);
  pimpl_->plug_provider_ = std::make_unique<Vst::PlugProvider> (
    module->getFactory (), *class_info_it, true);
  if (!pimpl_->plug_provider_->initialize ())
    {
      z_warning ("VST3: PlugProvider initialization failed for '{}'", path);
      pimpl_->plug_provider_.reset ();
      pimpl_->host_app_ = nullptr;
      return false;
    }

  pimpl_->component_ = pimpl_->plug_provider_->getComponentPtr ();
  pimpl_->controller_ = pimpl_->plug_provider_->getControllerPtr ();
  pimpl_->processor_ = U::cast<Vst::IAudioProcessor> (pimpl_->component_);
  if (
    pimpl_->component_ == nullptr || pimpl_->controller_ == nullptr
    || pimpl_->processor_ == nullptr)
    {
      z_warning (
        "VST3: failed to get component/controller/processor for '{}'", path);
      pimpl_->plug_provider_.reset ();
      pimpl_->host_app_ = nullptr;
      return false;
    }
  pimpl_->class_id_ = FUID::fromTUID (class_info_it->ID ().data ());

  pimpl_->component_handler_ = owned (
    new Vst3ComponentHandler ([this] (Vst::ParamID id, Vst::ParamValue value) {
      const auto it = pimpl_->vst3_params_.find (id);
      if (it == pimpl_->vst3_params_.end ())
        return;
      const auto param_index = it->second.param_index;
      if (param_index >= param_sync_.entries.size ())
        return;
      auto      &entry = param_sync_.entries[param_index];
      const auto normalized = static_cast<float> (value);
      entry.pending_value.store (normalized, std::memory_order_release);
      entry.last_from_plugin = normalized;
    }));
  pimpl_->controller_->setComponentHandler (pimpl_->component_handler_);

  // Activate all audio and event buses
  for (
    const auto media_type : { Vst::MediaTypes::kAudio, Vst::MediaTypes::kEvent })
    {
      for (const auto dir : { Vst::kInput, Vst::kOutput })
        {
          const auto bus_count =
            pimpl_->component_->getBusCount (media_type, dir);
          for (const auto i : std::views::iota (0, bus_count))
            {
              pimpl_->component_->activateBus (media_type, dir, i, true);
            }
        }
    }

  pimpl_->module_ = std::move (module);

  if (generate_new_ports)
    {
      create_ports_from_vst3_component ();
    }

  // Build the MIDI CC -> ParamID translation table from the plugin's
  // IMidiMapping (the spec's CC mechanism). Params covered by it are not
  // exposed as Zrythm parameters; MIDI CC events are translated to parameter
  // changes at process time instead.
  pimpl_->midi_cc_param_ids_.fill (Vst::kNoParamId);
  pimpl_->midi_cc_param_id_set_.clear ();
  if (auto midi_mapping = U::cast<Vst::IMidiMapping> (pimpl_->controller_))
    {
      for (const auto ch : std::views::iota (0, 16))
        {
          for (const auto ctrl : std::views::iota (0, Vst::kCountCtrlNumber))
            {
              Vst::ParamID param_id = 0;
              if (
                midi_mapping->getMidiControllerAssignment (
                  0, static_cast<int16> (ch),
                  static_cast<Vst::CtrlNumber> (ctrl), param_id)
                == kResultOk)
                {
                  pimpl_->midi_cc_param_ids_[static_cast<size_t> (
                    ch * Vst::kCountCtrlNumber + ctrl)] = param_id;
                  pimpl_->midi_cc_param_id_set_.insert (param_id);
                }
            }
        }
    }

  create_parameters_from_vst3_controller ();

  // Apply any pending state from JSON deserialization
  if (state_to_apply_.has_value ())
    {
      apply_state_from_byte_array (*state_to_apply_);
      state_to_apply_.reset ();
    }

  Q_EMIT hasNativeUiChanged ();
  return true;
}

void
Vst3Plugin::unload_current_plugin ()
{
  detach_editor ();
  release_resources_impl ();

  if (pimpl_->controller_ != nullptr && pimpl_->component_handler_ != nullptr)
    {
      pimpl_->controller_->setComponentHandler (nullptr);
    }
  pimpl_->component_handler_.reset ();
  // The view's lifetime must not exceed the controller's
  pimpl_->view_.reset ();
  pimpl_->view_check_done_ = false;
  pimpl_->processor_.reset ();
  pimpl_->controller_.reset ();
  pimpl_->component_.reset ();
  pimpl_->plug_provider_.reset ();
  pimpl_->host_app_ = nullptr;
  pimpl_->process_data_.unprepare ();
  pimpl_->module_.reset ();
}

void
Vst3Plugin::create_ports_from_vst3_component ()
{
  const auto create_midi_ports = [&] (Vst::BusDirection dir) {
    const auto bus_count =
      pimpl_->component_->getBusCount (Vst::MediaTypes::kEvent, dir);
    const bool is_input = dir == Vst::kInput;
    for (const auto i : std::views::iota (0, bus_count))
      {
        auto port_ref = utils::create_object<dsp::MidiPort> (
          registry (),
          utils::Utf8String::from_utf8_encoded_string (
            fmt::format ("MIDI {} {}", is_input ? "Input" : "Output", i + 1)),
          is_input ? dsp::PortFlow::Input : dsp::PortFlow::Output);
        if (is_input)
          add_input_port (port_ref);
        else
          add_output_port (port_ref);
      }
  };
  create_midi_ports (Vst::kInput);
  create_midi_ports (Vst::kOutput);

  const auto create_audio_ports = [&] (Vst::BusDirection dir) {
    const auto bus_count =
      pimpl_->component_->getBusCount (Vst::MediaTypes::kAudio, dir);
    const bool is_input = dir == Vst::kInput;
    for (const auto i : std::views::iota (0, bus_count))
      {
        Vst::BusInfo bus_info{};
        pimpl_->component_->getBusInfo (
          Vst::MediaTypes::kAudio, dir, i, bus_info);
        const dsp::AudioPort::BusLayout layout =
          bus_info.channelCount == 1 ? dsp::AudioPort::BusLayout::Mono
          : bus_info.channelCount == 2
            ? dsp::AudioPort::BusLayout::Stereo
            : dsp::AudioPort::BusLayout{};
        const auto name = utils::Utf8String::from_utf8_encoded_string (
          Steinberg::Vst::StringConvert::convert (bus_info.name));
        auto port_ref = utils::create_object<dsp::AudioPort> (
          registry (), name,
          is_input ? dsp::PortFlow::Input : dsp::PortFlow::Output, layout,
          bus_info.channelCount,
          i == 0
            ? dsp::AudioPort::Purpose::Main
            : dsp::AudioPort::Purpose::Sidechain);
        if (is_input)
          add_input_port (port_ref);
        else
          add_output_port (port_ref);
      }
  };
  create_audio_ports (Vst::kInput);
  create_audio_ports (Vst::kOutput);
}

void
Vst3Plugin::create_parameters_from_vst3_controller ()
{
  pimpl_->vst3_params_.clear ();
  pimpl_->zrythm_to_vst3_.clear ();
  pimpl_->program_change_param_id_.reset ();
  pimpl_->program_change_step_count_ = 0;

  const auto param_count = pimpl_->controller_->getParameterCount ();
  for (const auto i : std::views::iota (0, param_count))
    {
      Vst::ParameterInfo info{};
      if (pimpl_->controller_->getParameterInfo (i, info) != kResultOk)
        continue;
      if ((info.flags & Vst::ParameterInfo::kIsHidden) != 0)
        continue;

      // MIDI-CC-mapped params are handled via MIDI event translation, not as
      // Zrythm parameters
      if (pimpl_->midi_cc_param_id_set_.contains (info.id))
        continue;

      if ((info.flags & Vst::ParameterInfo::kIsProgramChange) != 0)
        {
          pimpl_->program_change_param_id_ = info.id;
          pimpl_->program_change_step_count_ = info.stepCount;
        }

      const auto unique_id = dsp::ProcessorParameter::UniqueId (
        utils::Utf8String::from_utf8_encoded_string (
          fmt::format ("vst3-param-{}", info.id)));

      // Try to find an existing Zrythm param with the same unique ID
      // (happens during deserialization — params already restored from JSON)
      dsp::ProcessorParameter * zrythm_param = nullptr;
      size_t                    param_index = 0;
      for (const auto &param_ref : get_parameters ())
        {
          auto * p = param_ref.get ();
          if (p->get_unique_id () == unique_id)
            {
              zrythm_param = p;
              break;
            }
          ++param_index;
        }

      if (zrythm_param == nullptr)
        {
          dsp::ParameterRange range{
            dsp::ParameterRange::Type::Linear, 0.f, 1.f, 0.f,
            static_cast<float> (info.defaultNormalizedValue)
          };
          if (info.stepCount == 1)
            {
              range = dsp::ParameterRange::make_toggle (
                info.defaultNormalizedValue > 0.5);
            }
          auto param_ref = utils::create_object<dsp::ProcessorParameter> (
            registry (), registry (), unique_id, range,
            utils::Utf8String::from_utf8_encoded_string (
              Steinberg::Vst::StringConvert::convert (info.title)));
          add_parameter (param_ref);
          zrythm_param = param_ref.get ();
          zrythm_param->set_automatable (
            (info.flags & Vst::ParameterInfo::kCanAutomate) != 0);
          zrythm_param->setBaseValue (
            static_cast<float> (
              pimpl_->controller_->getParamNormalized (info.id)));
          param_index = get_parameters ().size () - 1;
        }

      pimpl_->vst3_params_.emplace (
        info.id,
        Vst3PluginImpl::Vst3ParamAdapter{ info.id, zrythm_param, param_index });
      pimpl_->zrythm_to_vst3_.emplace (zrythm_param, info.id);
    }
}

void
Vst3Plugin::prepare_plugin_for_processing (
  units::sample_rate_t sample_rate,
  units::sample_u32_t  max_block_length)
{
  if (pimpl_->processor_ == nullptr)
    return;

  if (pimpl_->processing_active_)
    {
      release_resources_impl ();
    }

  const auto max_block = max_block_length.in<int32_t> (units::samples);

  // Queue capacity: one per exposed param plus a budget for MIDI-CC-translated
  // queues (CCs beyond the budget within a single block are dropped)
  static constexpr size_t kMaxMidiCcQueuesPerBlock = 256;
  pimpl_->input_param_changes_.prepare (
    pimpl_->vst3_params_.size () + kMaxMidiCcQueuesPerBlock);

  // Pre-size event lists so addEvent() never allocates on the audio thread
  pimpl_->input_events_.setMaxSize (max_block * 4);
  pimpl_->output_events_.setMaxSize (max_block * 4);

  pimpl_->process_data_.unprepare ();
  if (!pimpl_->process_data_.prepare (
        *pimpl_->component_, 0, Vst::SymbolicSampleSizes::kSample32))
    {
      z_warning ("VST3: failed to prepare process data");
      return;
    }

  if (
    pimpl_->processor_->canProcessSampleSize (Vst::SymbolicSampleSizes::kSample32)
    != kResultOk)
    {
      z_warning ("VST3: plugin does not support 32-bit sample processing");
      return;
    }

  if (pimpl_->component_->setActive (true) != kResultOk)
    {
      z_warning ("VST3: setActive(true) failed");
      return;
    }

  Vst::ProcessSetup setup{
    Vst::ProcessModes::kRealtime, Vst::SymbolicSampleSizes::kSample32, max_block,
    static_cast<Steinberg::Vst::SampleRate> (sample_rate.in (units::sample_rate))
  };
  if (pimpl_->processor_->setupProcessing (setup) != kResultOk)
    {
      z_warning ("VST3: setupProcessing failed");
      pimpl_->component_->setActive (false);
      return;
    }

  // setProcessing is optional for plugins - the SDK's AudioEffect base class
  // returns kNotImplemented, which must not be treated as an error
  if (pimpl_->processor_->setProcessing (true) == kResultFalse)
    {
      z_warning ("VST3: setProcessing(true) failed");
      pimpl_->component_->setActive (false);
      return;
    }

  pimpl_->processing_active_ = true;
  pimpl_->latency_ = units::samples (pimpl_->processor_->getLatencySamples ());
}

void
Vst3Plugin::release_resources_impl ()
{
  if (!pimpl_->processing_active_)
    return;

  pimpl_->processor_->setProcessing (false);
  pimpl_->component_->setActive (false);
  pimpl_->processing_active_ = false;
}

void
Vst3Plugin::process_impl (dsp::graph::ProcessBlockInfo time_info) noexcept
{
  auto &impl = *pimpl_;

  if (!impl.processing_active_)
    return;

  const auto nframes = time_info.nframes_.in<int32_t> (units::samples);
  const auto local_offset =
    time_info.buffer_offset_.in<int32_t> (units::samples);

  // Fill input events from the MIDI input port. Note on/off become VST note
  // events; polyphonic aftertouch becomes a poly pressure event; SysEx
  // becomes a data event; MIDI CC / channel pressure / pitch bend / program
  // change are translated to parameter changes (via the plugin's IMidiMapping
  // table / kIsProgramChange param).
  impl.input_events_.clear ();
  impl.input_param_changes_.clear_queues ();
  if (midi_in_port_ != nullptr)
    {
      for (const auto &ev : midi_in_port_->buffer_)
        {
          const auto ev_data = ev.data ();
          const auto sample_offset = ev.time ().in<int32_t> (units::samples);

          // SysEx -> data event (bytes must stay valid during process(); the
          // MIDI port buffer outlives the process call)
          if (!ev_data.empty () && ev_data[0] == 0xF0)
            {
              Vst::Event vst_ev{};
              vst_ev.busIndex = 0;
              vst_ev.sampleOffset = sample_offset;
              vst_ev.ppqPosition = 0;
              vst_ev.flags = Vst::Event::kIsLive;
              vst_ev.type = Vst::Event::kDataEvent;
              vst_ev.data.size = static_cast<uint32_t> (ev_data.size ());
              vst_ev.data.type = Vst::DataEvent::kMidiSysEx;
              vst_ev.data.bytes =
                reinterpret_cast<const uint8_t *> (ev_data.data ());
              impl.input_events_.addEvent (vst_ev);
              continue;
            }

          if (ev_data.size () < 2)
            continue;

          const auto status = ev_data[0] & 0xF0;
          const auto channel = ev_data[0] & 0x0F;

          // Program change -> kIsProgramChange param
          if (status == 0xC0)
            {
              if (
                pimpl_->program_change_param_id_.has_value ()
                && pimpl_->program_change_step_count_ > 0)
                {
                  int32  queue_index = 0;
                  auto * queue = impl.input_param_changes_.addParameterData (
                    *pimpl_->program_change_param_id_, queue_index);
                  if (queue != nullptr)
                    {
                      int32 point_index = 0;
                      queue->addPoint (
                        sample_offset,
                        static_cast<Vst::ParamValue> (ev_data[1])
                          / pimpl_->program_change_step_count_,
                        point_index);
                    }
                }
              continue;
            }

          if (ev_data.size () < 3)
            continue;

          // Polyphonic aftertouch -> poly pressure event
          if (status == 0xA0)
            {
              Vst::Event vst_ev{};
              vst_ev.busIndex = 0;
              vst_ev.sampleOffset = sample_offset;
              vst_ev.ppqPosition = 0;
              vst_ev.flags = Vst::Event::kIsLive;
              vst_ev.type = Vst::Event::kPolyPressureEvent;
              vst_ev.polyPressure.channel = static_cast<int16_t> (channel);
              vst_ev.polyPressure.pitch = static_cast<int16_t> (ev_data[1]);
              vst_ev.polyPressure.pressure =
                static_cast<float> (ev_data[2]) / 127.f;
              vst_ev.polyPressure.noteId = -1;
              impl.input_events_.addEvent (vst_ev);
              continue;
            }

          // MIDI CC / channel pressure / pitch bend -> parameter change
          std::optional<Vst::CtrlNumber> ctrl_number;
          Vst::ParamValue                ctrl_value = 0.;
          if (status == 0xB0)
            {
              ctrl_number = static_cast<Vst::CtrlNumber> (ev_data[1]);
              ctrl_value = static_cast<Vst::ParamValue> (ev_data[2]) / 127.;
            }
          else if (status == 0xD0)
            {
              ctrl_number = Vst::kAfterTouch;
              ctrl_value = static_cast<Vst::ParamValue> (ev_data[1]) / 127.;
            }
          else if (status == 0xE0)
            {
              ctrl_number = Vst::kPitchBend;
              ctrl_value =
                static_cast<Vst::ParamValue> (
                  (static_cast<int> (ev_data[2]) << 7) | ev_data[1])
                / 16383.;
            }
          if (ctrl_number.has_value ())
            {
              const auto param_id = impl.midi_cc_param_ids_[static_cast<size_t> (
                channel * Vst::kCountCtrlNumber + *ctrl_number)];
              if (param_id != Vst::kNoParamId)
                {
                  int32  queue_index = 0;
                  auto * queue = impl.input_param_changes_.addParameterData (
                    param_id, queue_index);
                  if (queue != nullptr)
                    {
                      int32 point_index = 0;
                      queue->addPoint (sample_offset, ctrl_value, point_index);
                    }
                }
              continue;
            }

          const bool is_note_on = status == 0x90 && ev_data[2] != 0;
          const bool is_note_off =
            status == 0x80 || (status == 0x90 && ev_data[2] == 0);
          if (!is_note_on && !is_note_off)
            continue;

          Vst::Event vst_ev{};
          vst_ev.busIndex = 0;
          vst_ev.sampleOffset = sample_offset;
          vst_ev.ppqPosition = 0;
          vst_ev.flags = Vst::Event::kIsLive;
          if (is_note_on)
            {
              vst_ev.type = Vst::Event::kNoteOnEvent;
              vst_ev.noteOn.channel = static_cast<int16_t> (channel);
              vst_ev.noteOn.pitch = static_cast<int16_t> (ev_data[1]);
              vst_ev.noteOn.tuning = 0.f;
              vst_ev.noteOn.velocity = static_cast<float> (ev_data[2]) / 127.f;
              vst_ev.noteOn.length = 0;
              vst_ev.noteOn.noteId = -1;
            }
          else
            {
              vst_ev.type = Vst::Event::kNoteOffEvent;
              vst_ev.noteOff.channel = static_cast<int16_t> (channel);
              vst_ev.noteOff.pitch = static_cast<int16_t> (ev_data[1]);
              vst_ev.noteOff.velocity = 0.f;
              vst_ev.noteOff.tuning = 0.f;
              vst_ev.noteOff.noteId = -1;
            }
          impl.input_events_.addEvent (vst_ev);
        }
    }

  // Point the bus channel buffers directly at the Zrythm port buffers
  // (zero-copy)
  for (const auto &[bus_index, port] : utils::views::enumerate (audio_in_ports_))
    {
      auto &bus_buffers = impl.process_data_.inputs[bus_index];
      for (const auto ch : std::views::iota (0, bus_buffers.numChannels))
        {
          bus_buffers.channelBuffers32[ch] =
            const_cast<float *> (
              port->buffers ()->getReadPointer (static_cast<int> (ch)))
            + local_offset;
        }
      bus_buffers.silenceFlags = 0;
    }
  for (
    const auto &[bus_index, port] : utils::views::enumerate (audio_out_ports_))
    {
      auto &bus_buffers = impl.process_data_.outputs[bus_index];
      for (const auto ch : std::views::iota (0, bus_buffers.numChannels))
        {
          bus_buffers.channelBuffers32[ch] =
            port->buffers ()->getWritePointer (static_cast<int> (ch))
            + local_offset;
        }
      bus_buffers.silenceFlags = 0;
    }

  // Sync changed Zrythm parameters to the plugin. Values are normalized, so
  // no conversion is needed (all VST3 params use [0,1] ranges). Feedback
  // guard: skip values that came from the plugin itself. Queues were already
  // cleared (and possibly pre-populated with MIDI-CC-translated points) in
  // the MIDI input pass above.
  for (const auto &change : change_tracker ().changes ())
    {
      auto &entry = param_sync_.entries[change.index];
      if (
        utils::math::floats_equal (
          change.modulated_value, entry.last_from_plugin))
        {
          entry.last_from_plugin = -1.f;
          continue;
        }
      const auto it = impl.zrythm_to_vst3_.find (change.param);
      if (it == impl.zrythm_to_vst3_.end ())
        continue;
      int32  queue_index = 0;
      auto * queue =
        impl.input_param_changes_.addParameterData (it->second, queue_index);
      if (queue == nullptr)
        continue;
      int32 point_index = 0;
      queue->addPoint (0, change.modulated_value, point_index);
    }

  impl.process_data_.numSamples = nframes;
  impl.process_data_.processMode = Vst::ProcessModes::kRealtime;
  impl.process_data_.inputParameterChanges = &impl.input_param_changes_;
  impl.process_data_.outputParameterChanges = nullptr;
  impl.process_data_.inputEvents = &impl.input_events_;
  impl.process_data_.outputEvents = &impl.output_events_;
  impl.process_context_ = {};
  impl.process_context_.projectTimeSamples =
    time_info.transport_position_.in<int64_t> (units::samples);
  impl.process_data_.processContext = &impl.process_context_;

  {
#if defined(__has_feature) && __has_feature(realtime_sanitizer)
    // Not our code, we don't care about RTSan violations here
    __rtsan::ScopedDisabler d;
#endif
    impl.processor_->process (impl.process_data_);
  }

  // Drain output events into the MIDI output port (note on/off only)
  if (midi_out_port_ != nullptr)
    {
      const auto event_count = impl.output_events_.getEventCount ();
      for (const auto i : std::views::iota (0, event_count))
        {
          Vst::Event vst_ev{};
          if (impl.output_events_.getEvent (i, vst_ev) != kResultOk)
            continue;

          const auto time =
            units::samples (static_cast<uint64_t> (vst_ev.sampleOffset));
          if (vst_ev.type == Vst::Event::kNoteOnEvent)
            {
              const auto midi_ev = dsp::midi_event::make_note_on (
                static_cast<midi_byte_t> (vst_ev.noteOn.channel),
                static_cast<midi_byte_t> (vst_ev.noteOn.pitch),
                static_cast<midi_byte_t> (vst_ev.noteOn.velocity * 127.f), time);
              midi_out_port_->buffer_.push_back (midi_ev.time_, midi_ev.data ());
            }
          else if (vst_ev.type == Vst::Event::kNoteOffEvent)
            {
              const auto midi_ev = dsp::midi_event::make_note_off (
                static_cast<midi_byte_t> (vst_ev.noteOff.channel),
                static_cast<midi_byte_t> (vst_ev.noteOff.pitch), time);
              midi_out_port_->buffer_.push_back (midi_ev.time_, midi_ev.data ());
            }
        }
    }
  impl.output_events_.clear ();
}

std::string
Vst3Plugin::save_state_impl () const
{
  if (!pimpl_ || pimpl_->component_ == nullptr || pimpl_->controller_ == nullptr)
    return {};

  // Standard .vstpreset container: header + component state + controller
  // state + chunk list
  Steinberg::ResizableMemoryIBStream stream;
  if (!Steinberg::Vst::PresetFile::savePreset (
        &stream, pimpl_->class_id_, pimpl_->component_, pimpl_->controller_))
    {
      return {};
    }

  const auto       data = stream.take ();
  const QByteArray bytes (
    reinterpret_cast<const char *> (data.data ()),
    static_cast<qsizetype> (data.size ()));
  return utils::to_std_string (bytes.toBase64 ());
}

void
Vst3Plugin::load_state_impl (const std::string &base64_state)
{
  auto data = QByteArray::fromBase64 (QByteArray::fromStdString (base64_state));

  if (!pimpl_ || pimpl_->component_ == nullptr || pimpl_->controller_ == nullptr)
    {
      // Not instantiated yet - apply during instantiation
      state_to_apply_ = std::move (data);
      return;
    }

  // IComponent::setState and IEditController::setComponentState/setState are
  // UI-thread calls allowed while processing, so the state can be applied
  // immediately
  apply_state_from_byte_array (data);
}

void
Vst3Plugin::apply_state_from_byte_array (const QByteArray &data)
{
  if (pimpl_->component_ == nullptr || pimpl_->controller_ == nullptr)
    return;

  Steinberg::ResizableMemoryIBStream stream;
  stream.write (
    const_cast<char *> (data.constData ()),
    static_cast<Steinberg::int32> (data.size ()), nullptr);
  stream.rewind ();
  // Validates the stored class ID against the plugin's component ID, then
  // restores: IComponent::setState -> IEditController::setComponentState ->
  // IEditController::setState
  if (!Steinberg::Vst::PresetFile::loadPreset (
        &stream, pimpl_->class_id_, pimpl_->component_, pimpl_->controller_))
    {
      z_warning ("VST3: failed to restore plugin state");
      return;
    }

  // Read back parameter values from the controller so Zrythm's base values
  // reflect the loaded state
  for (const auto &[param_id, adapter] : pimpl_->vst3_params_)
    {
      if (adapter.zrythm_param == nullptr)
        continue;
      const auto value =
        static_cast<float> (pimpl_->controller_->getParamNormalized (param_id));
      const auto old_base = adapter.zrythm_param->baseValue ();
      if (!utils::math::floats_near (old_base, value, 0.001f))
        {
          adapter.zrythm_param->setBaseValue (value);
        }
    }
}

void
to_json (nlohmann::json &j, const Vst3Plugin &p)
{
  to_json (j, static_cast<const Plugin &> (p));
  auto state = p.save_state_impl ();
  if (!state.empty ())
    j[Vst3Plugin::kStateKey] = std::move (state);
}

void
from_json (const nlohmann::json &j, Vst3Plugin &p)
{
  // State must be deserialized first, because the Plugin deserialization
  // may cause an instantiation
  if (j.contains (Vst3Plugin::kStateKey))
    p.load_state_impl (j[Vst3Plugin::kStateKey].get<std::string> ());

  from_json (j, static_cast<Plugin &> (p));
}

} // namespace zrythm::plugins
