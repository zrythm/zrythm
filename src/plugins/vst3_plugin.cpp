// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense
/*
 * VST3 hosting implemented against the VST3 SDK.
 */

#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <optional>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include <fmt/std.h>

#include "dsp/midi_event.h"
#include "plugins/gl_context_utils.h"
#include "plugins/host_window_units.h"
#include "plugins/plugin_run_loop.h"
#include "plugins/plugin_transport_context.h"
#include "plugins/vst3_channel_mapping.h"
#include "plugins/vst3_event_validation.h"
#include "plugins/vst3_plugin.h"
#include "plugins/vst3_plugin_format.h"
#include "plugins/vst3_speaker_arrangement.h"
#include "utils/logger.h"
#include "utils/math_utils.h"
#include "utils/qt.h"
#include "utils/serialization.h"
#include "utils/views.h"

#include <farbot/RealtimeObject.hpp>

#if defined(__has_feature) && __has_feature(realtime_sanitizer)
#  include <sanitizer/rtsan_interface.h>
#endif

#include <QCoreApplication>
#include <QTimer>

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

// The helpers below are deliberately not in an anonymous namespace:
// Vst3PluginImpl holds them as members, and members with internal linkage
// make it ill-formed to use the enclosing class across translation units
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
    // One handler may be registered for multiple fds (unregisterEventHandler
    // takes no fd, so it must remove all of a handler's registrations)
    event_handler_tokens_.emplace (handler, token);
    return kResultOk;
  }

  tresult PLUGIN_API
  unregisterEventHandler (Linux::IEventHandler * handler) override
  {
    const auto range = event_handler_tokens_.equal_range (handler);
    if (range.first == range.second)
      return kResultFalse;
    for (
      const auto &token :
      std::ranges::subrange (range.first, range.second) | std::views::values)
      {
        run_loop ()->unregister_fd (token);
      }
    event_handler_tokens_.erase (range.first, range.second);
    return kResultOk;
  }

  tresult PLUGIN_API registerTimer (
    Linux::ITimerHandler * handler,
    Linux::TimerInterval   milliseconds) override
  {
    if (handler == nullptr)
      return kInvalidArgument;
    // Re-registering replaces the previous timer (interval change): the
    // handler would have no way to distinguish multiple timers anyway
    if (
      const auto it = timer_handler_tokens_.find (handler);
      it != timer_handler_tokens_.end ())
      {
        run_loop ()->unregister_timer (it->second);
      }
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
   * @brief Run loop for this context's IRunLoop registrations, created on
   * demand.
   *
   * Owned by this context (dying with it on plugin unload) and parented to
   * the QCoreApplication so it is still destroyed before application
   * teardown completes if the plugin outlives it. Main thread only.
   */
  PluginRunLoop * run_loop ()
  {
    if (run_loop_ == nullptr)
      run_loop_ = utils::make_qobject_unique<PluginRunLoop> (
        QCoreApplication::instance ());
    return run_loop_.get ();
  }
#endif // SMTG_OS_LINUX

  /**
   * @brief Removes all fd watches and timers registered through this
   * context.
   *
   * Called when the owning plugin unloads: plug-ins may destroy their
   * handlers and close their fds without unregistering them, and the
   * watches would otherwise fire into dead handlers or observe closed
   * fds.
   *
   * Only GNU/Linux hands plug-ins a run loop, so there is nothing to
   * remove elsewhere and this does nothing.
   */
  void unregister_all ()
  {
#if SMTG_OS_LINUX
    if (run_loop_ == nullptr)
      return;
    for (const auto token : event_handler_tokens_ | std::views::values)
      {
        run_loop_->unregister_fd (token);
      }
    event_handler_tokens_.clear ();
    for (const auto token : timer_handler_tokens_ | std::views::values)
      {
        run_loop_->unregister_timer (token);
      }
    timer_handler_tokens_.clear ();
#endif
  }

private:
  IPtr<Vst::PlugInterfaceSupport> plug_interface_support_;

#if SMTG_OS_LINUX
  utils::QObjectUniquePtr<PluginRunLoop> run_loop_;

  /** One entry per (handler, fd) registration: a handler may watch
   * multiple fds. */
  std::unordered_multimap<Linux::IEventHandler *, PluginRunLoop::Token>
    event_handler_tokens_;

  /** One timer per handler (re-registration replaces the interval). */
  std::unordered_map<Linux::ITimerHandler *, PluginRunLoop::Token>
    timer_handler_tokens_;
#endif // SMTG_OS_LINUX
};

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
    // Points are kept sorted by ascending sample offset: the
    // IParamValueQueue contract expects a time-ordered automation curve
    for (const auto i : std::views::iota (0, point_count_))
      {
        auto &point = points_[static_cast<size_t> (i)];
        if (point.sample_offset == sampleOffset)
          {
            point.value = value;
            index = i;
            return kResultTrue;
          }
        if (point.sample_offset > sampleOffset)
          {
            if (point_count_ >= static_cast<int32> (kMaxPoints))
              return kResultFalse;
            std::move_backward (
              points_.begin () + i, points_.begin () + point_count_,
              points_.begin () + point_count_ + 1);
            point = { sampleOffset, value };
            index = i;
            ++point_count_;
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
  using RestartCallback = std::function<void (int32 flags)>;

  explicit Vst3ComponentHandler (
    PerformEditCallback perform_edit_cb,
    RestartCallback     restart_cb)
      : perform_edit_cb_ (std::move (perform_edit_cb)),
        restart_cb_ (std::move (restart_cb))
  {
  }

  tresult PLUGIN_API beginEdit (Vst::ParamID) override { return kResultOk; }
  tresult PLUGIN_API
  performEdit (Vst::ParamID id, Vst::ParamValue value) override
  {
    // IComponentHandler methods are UI-thread calls per the SDK contract
    perform_edit_cb_ (id, value);
    return kResultOk;
  }
  tresult PLUGIN_API endEdit (Vst::ParamID) override { return kResultOk; }
  tresult PLUGIN_API restartComponent (int32 flags) override
  {
    restart_cb_ (flags);
    return kResultOk;
  }

private:
  PerformEditCallback perform_edit_cb_;
  RestartCallback     restart_cb_;
};

/**
 * @brief IPlugFrame implementation handling plugin-initiated view resizes.
 *
 * Also implements Linux::IRunLoop (forwarding to the owning plugin's host
 * application) since plug-ins may query the run loop from the frame
 * instead of the host context (both are documented locations).
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

  Vst3PlugFrame (
    IPlugView *               view,
    IPtr<Vst3HostApplication> host_app,
    ResizeCallback            resize_cb)
      : view_ (view), host_app_ (std::move (host_app)),
        resize_cb_ (std::move (resize_cb))
  {
  }

  tresult PLUGIN_API resizeView (IPlugView * view, ViewRect * newSize) override
  {
    if (view != view_ || newSize == nullptr)
      return kInvalidArgument;

    if (newSize->getWidth () <= 0 || newSize->getHeight () <= 0)
      {
        z_warning (
          "VST3 plugin requested an invalid resize to {}x{}; refusing",
          newSize->getWidth (), newSize->getHeight ());
        return kResultFalse;
      }

    // Let the plug-in adjust the requested size, then resize the host window
    // and notify the plug-in of the final size (per the IPlugView sizing
    // contract)
    const ScopedGlContextRelease gl_release;
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
    return host_app_->registerEventHandler (handler, fd);
  }
  tresult PLUGIN_API
  unregisterEventHandler (Linux::IEventHandler * handler) override
  {
    return host_app_->unregisterEventHandler (handler);
  }
  tresult PLUGIN_API registerTimer (
    Linux::ITimerHandler * handler,
    Linux::TimerInterval   milliseconds) override
  {
    return host_app_->registerTimer (handler, milliseconds);
  }
  tresult PLUGIN_API unregisterTimer (Linux::ITimerHandler * handler) override
  {
    return host_app_->unregisterTimer (handler);
  }
#endif // SMTG_OS_LINUX

private:
  IPlugView *               view_;
  IPtr<Vst3HostApplication> host_app_;
  ResizeCallback            resize_cb_;
};

/**
 * @brief Claims the process-wide plugin context factory for the given host
 * application.
 *
 * The factory is a last-writer-wins singleton by SDK design; plugins
 * (VSTGUI) query the context during view lifecycle calls, so this must be
 * called immediately before createView()/attached()/removed().
 */
static void
claim_plugin_context (Vst::IHostApplication * host_app)
{
  Vst::PluginContextFactory::instance ().setPluginContext (host_app);
}

/**
 * @brief Clears the plugin context factory if it still references the given
 * host application.
 *
 * The factory is a process-wide singleton holding a raw pointer; leaving a
 * stale pointer behind would let a later getPluginContext() observe freed
 * memory.
 */
static void
clear_plugin_context_if_current (Vst::IHostApplication * host_app)
{
  if (Vst::PluginContextFactory::instance ().getPluginContext () == host_app)
    Vst::PluginContextFactory::instance ().setPluginContext (nullptr);
}

/**
 * Parameter routing state read on the audio thread.
 *
 * Mutated on the main thread (MIDI-CC table rebuilds on
 * kMidiCCAssignmentChanged, program-change info refresh, parameter
 * (re)creation on load); the audio thread holds one realtime ScopedAccess
 * per process block.
 */
struct Vst3RtParamMapping
{
  /**
   * MIDI CC -> ParamID translation table from the plugin's IMidiMapping
   * ([channel * kCountCtrlNumber + ctrlNumber], kNoParamId if unmapped).
   *
   * Params covered by it are not exposed as Zrythm parameters; MIDI CC
   * events are translated to parameter changes at process time instead.
   */
  std::array<Vst::ParamID, 16 * Vst::kCountCtrlNumber> midi_cc_param_ids_{};

  /** Param flagged kIsProgramChange (if any) and its step count, used to
   * translate MIDI program change messages to parameter changes. */
  std::optional<Vst::ParamID> program_change_param_id_;
  int32                       program_change_step_count_ = 0;

  std::unordered_map<dsp::ProcessorParameter *, Vst::ParamID> zrythm_to_vst3_;
};

static void
refresh_program_change_param (
  Vst::IEditController &controller,
  Vst3RtParamMapping   &rt_mapping);

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

  VST3::Hosting::Module::Ptr            module_;
  std::unique_ptr<Vst::PlugProvider>    plug_provider_;
  FUID                                  class_id_{};
  IPtr<Vst::IComponent>                 component_;
  IPtr<Vst::IEditController>            controller_;
  IPtr<Vst::IEditControllerHostEditing> controller_host_editing_;
  IPtr<Vst::IAudioProcessor>            processor_;
  IPtr<Vst3HostApplication>             host_app_;
  IPtr<Vst3ComponentHandler>            component_handler_;
  Vst::HostProcessData                  process_data_;
  Vst::EventList                        input_events_{ 0 };
  Vst::EventList                        output_events_{ 0 };
  Vst3ParameterChanges                  input_param_changes_{};
  Vst3ParameterChanges                  output_param_changes_{};

  /**
   * @brief Reports dropped items at a bounded rate (audio-thread safe).
   *
   * The drop is counted in @p counter and logged on the main thread at
   * power-of-two counts, so a flood of drops cannot flood the log or the
   * dispatcher.
   *
   * @param what Static string describing the dropped items, e.g.
   * "input event(s) (event list capacity exhausted)".
   */
  void
  note_drop (std::atomic<uint32_t> &counter, std::string_view what) noexcept
  {
    const auto drops = counter.fetch_add (1, std::memory_order_relaxed) + 1;
    if (drops == 1 || (drops & (drops - 1)) == 0)
      {
        owner_.post_main_thread_action ([this, drops, what] {
          z_warning (
            "VST3 plugin '{}': dropped {} {} so far", owner_.get_name (), drops,
            what);
        });
      }
  }

  /** Per-parameter queue capacity exhausted (addPoint failed). */
  void note_input_point_drop () noexcept
  {
    note_drop (
      input_point_drops_,
      "parameter change point(s) (per-parameter queue capacity exhausted)"sv);
  }

  std::atomic<uint32_t> input_point_drops_{ 0 };

  /** Event list capacity exhausted (addEvent failed). */
  void note_input_event_drop () noexcept
  {
    note_drop (
      input_event_drops_, "input event(s) (event list capacity exhausted)"sv);
  }

  std::atomic<uint32_t> input_event_drops_{ 0 };

  /** Events targeting an event input bus the plugin no longer has (stale
   * port topology after an IO change). */
  void note_stale_bus_event_drop () noexcept
  {
    note_drop (
      stale_bus_event_drops_,
      "input event(s) (no such event bus on the plugin)"sv);
  }

  std::atomic<uint32_t> stale_bus_event_drops_{ 0 };

  /**
   * @brief The plugin's current event input bus count.
   *
   * Updated on the main thread when the bus layout is (re)established
   * (load, IO change); read on the audio thread to drop events from stale
   * Zrythm ports that no longer have a corresponding plugin bus.
   */
  std::atomic<int32_t> event_in_bus_count_{ 0 };

  /** Queue table capacity exhausted (addParameterData failed). */
  void note_param_queue_drop () noexcept
  {
    note_drop (
      param_queue_drops_,
      "parameter change queue(s) (queue table capacity exhausted)"sv);
  }

  std::atomic<uint32_t> param_queue_drops_{ 0 };

  /** Main-thread dispatcher queue full (controller notification dropped). */
  void note_controller_notification_drop () noexcept
  {
    note_drop (
      controller_notification_drops_,
      "controller notification(s) (dispatcher queue full)"sv);
  }

  std::atomic<uint32_t> controller_notification_drops_{ 0 };

  /**
   * @brief Reports a dropped plugin output event (audio-thread safe).
   *
   * The event violated the output contract (see
   * validate_vst3_output_event); the drop is counted and logged on the
   * main thread at a bounded (power-of-two) rate.
   */
  void note_invalid_output_event_drop (std::string_view violation) noexcept
  {
    const auto drops =
      invalid_output_event_drops_.fetch_add (1, std::memory_order_relaxed) + 1;
    if (drops == 1 || (drops & (drops - 1)) == 0)
      {
        owner_.post_main_thread_action ([this, drops, violation] {
          z_warning (
            "VST3 plugin '{}': dropped {} invalid output event(s) so far "
            "(last: {})",
            owner_.get_name (), drops, violation);
        });
      }
  }

  std::atomic<uint32_t> invalid_output_event_drops_{ 0 };
  Vst::ProcessContext   process_context_{};
  std::unordered_map<Vst::ParamID, Vst3ParamAdapter> vst3_params_;

  /**
   * Realtime-published param routing state. farbot's destructor spins until
   * any realtime access is released, so the impl must outlive processing.
   */
  farbot::RealtimeObject<
    Vst3RtParamMapping,
    farbot::RealtimeObjectOptions::nonRealtimeMutatable>
    rt_mapping_{ Vst3RtParamMapping{} };

  /** Main-thread-only companion of Vst3RtParamMapping::midi_cc_param_ids_:
   * the set of CC-mapped param IDs, used to skip exposing them as Zrythm
   * parameters. */
  std::unordered_set<Vst::ParamID> midi_cc_param_id_set_;

  bool processing_active_ = false;

  /** Sample rate from the last setupProcessing, for ProcessContext. */
  double sample_rate_ = 0.0;

  /** Max block length from the last prepare, for kIoChanged re-prepare. */
  Steinberg::int32 max_block_samples_ = 0;

  /**
   * Scratch backing plugin bus channels with no corresponding Zrythm port
   * buffer (bus/channel count disagreement, e.g. after kIoChanged).
   * Allocated at prepare time alongside process_data_.
   */
  Vst3ScratchBuffers input_scratch_;
  Vst3ScratchBuffers output_scratch_;

  /**
   * Cached port buffer pointers, refreshed each block and passed to
   * map_vst3_channel_buffers(). Reserved at prepare time so the refresh
   * never allocates on the audio thread.
   */
  std::vector<juce::AudioBuffer<float> *> audio_in_buf_ptrs_;
  std::vector<juce::AudioBuffer<float> *> audio_out_buf_ptrs_;

  /** Playback latency; updated by the plugin via kLatencyChanged on any
   * thread and read on the audio thread. */
  std::atomic<units::sample_u32_t> latency_{ units::samples (0u) };

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

  utils::QObjectUniquePtr<plugins::PluginViewResizeCoordinator>
    resize_coordinator_;
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
  set_bypass_id (bypass_ref.id ());
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
  PluginConfiguration *,
  bool generateNewPluginPortsAndParams)
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
      claim_plugin_context (
        static_cast<Vst::IHostApplication *> (pimpl_->host_app_));
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
  // No native editor: leave uiVisible set so the generic UI is shown
  if (!hasNativeUi ())
    return;
  if (pimpl_->view_attached_)
    return;

  set_native_ui_unavailable (false);
  pimpl_->editor_window_ = pimpl_->host_window_factory_ (*this);
  if (pimpl_->editor_window_ == nullptr)
    {
      z_warning (
        "VST3: no host window available for plugin editor - showing "
        "generic UI");
      set_native_ui_unavailable (true);
      return;
    }

  // The host window failed to embed the plugin's view (already hidden by
  // the window itself): detach and fall back to the generic UI. Deferred
  // because the emission comes from within the window's own call stack
  connect (
    pimpl_->editor_window_.get (), &plugins::PluginHostWindow::embeddingFailed,
    this, [this] {
      QTimer::singleShot (std::chrono::milliseconds{ 0 }, this, [this] {
        detach_editor ();
        set_native_ui_unavailable (true);
      });
    });

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
  {
    const ScopedGlContextRelease gl_release;
    if (view->isPlatformTypeSupported (platform_type) != kResultTrue)
      {
        z_warning ("VST3: plugin editor does not support {}", platform_type);
        pimpl_->editor_window_.reset ();
        set_native_ui_unavailable (true);
        return;
      }
  }

  // The frame must be set before the scale factor and before attaching: the
  // plug-in may call IPlugFrame::resizeView() in response to either
  pimpl_->plug_frame_ = owned (
    new Vst3PlugFrame (view, pimpl_->host_app_, [this] (int width, int height) {
      if (pimpl_->editor_window_ == nullptr)
        return;
      if (width <= 0 || height <= 0)
        {
          z_warning (
            "VST3: plugin '{}' requested an invalid view size ({}x{}); "
            "keeping the current size",
            get_node_name (), width, height);
          return;
        }
      const auto [w, h] = plugin_view_size_to_host_window_size (
        width, height, pimpl_->editor_window_->contentScaleFactor ());
      pimpl_->editor_window_->setSize (w, h);
    }));
  const auto scale_factor = pimpl_->editor_window_->contentScaleFactor ();
  {
    const ScopedGlContextRelease gl_release;
    view->setFrame (pimpl_->plug_frame_);
    if (auto scale_support = U::cast<IPlugViewContentScaleSupport> (view))
      {
        scale_support->setContentScaleFactor (scale_factor);
      }
  }

  // Re-feed the scale and re-negotiate the size when the window's screen
  // scale changes
  connect (
    pimpl_->editor_window_.get (), &PluginHostWindow::contentScaleFactorChanged,
    this, [this, view] (float factor) {
      if (pimpl_->editor_window_ == nullptr)
        return;
      ViewRect size{};
      bool     have_size = false;
      {
        const ScopedGlContextRelease gl_release;
        if (auto scale_support = U::cast<IPlugViewContentScaleSupport> (view))
          {
            scale_support->setContentScaleFactor (factor);
          }
        // Fall back to re-querying for views that don't resizeView in
        // response to the scale change
        have_size =
          view->getSize (&size) == kResultOk && size.getWidth () > 0
          && size.getHeight () > 0;
      }
      if (have_size)
        {
          const auto [w, h] = plugin_view_size_to_host_window_size (
            size.getWidth (), size.getHeight (), factor);
          pimpl_->editor_window_->setSize (w, h);
          const ScopedGlContextRelease gl_release;
          view->onSize (&size);
        }
      else
        {
          z_warning (
            "VST3: plugin '{}' reported an invalid view size after a scale "
            "change; keeping the current size",
            get_node_name ());
        }
    });

  // Forward host-side embed area resizes (e.g., the user resizing the
  // window) to the view: plugins size their platform window in response to
  // onSize() (see the sizing rules in iplugview.h)
  pimpl_->resize_coordinator_ = utils::make_qobject_unique<
    plugins::PluginViewResizeCoordinator> (
    *pimpl_->editor_window_,
    plugins::PluginViewResizeCoordinator::Hooks{
      .gui_active = [this] { return pimpl_->view_attached_; },
      .can_resize =
        [view] {
          const ScopedGlContextRelease gl_release;
          return view->canResize () == kResultTrue;
        },
      .adjust_size =
        [view] (int &width, int &height) {
          const ScopedGlContextRelease gl_release;
          ViewRect                     rect{ 0, 0, width, height };
          view->checkSizeConstraint (&rect);
          width = rect.getWidth ();
          height = rect.getHeight ();
        },
      .apply_size =
        [view] (int width, int height) {
          const ScopedGlContextRelease gl_release;
          ViewRect                     rect{ 0, 0, width, height };
          view->onSize (&rect);
        },
    });

  ViewRect size{};
  bool     initial_size_ok = false;
  {
    const ScopedGlContextRelease gl_release;
    initial_size_ok =
      view->getSize (&size) == kResultOk && size.getWidth () > 0
      && size.getHeight () > 0;
  }
  if (!initial_size_ok)
    {
      z_warning ("VST3: plugin editor reported an invalid initial view size");
      {
        const ScopedGlContextRelease gl_release;
        view->setFrame (nullptr);
      }
      pimpl_->plug_frame_.reset ();
      pimpl_->editor_window_.reset ();
      set_native_ui_unavailable (true);
      return;
    }

  {
    const auto [w, h] = plugin_view_size_to_host_window_size (
      size.getWidth (), size.getHeight (), scale_factor);
    pimpl_->editor_window_->setSizeAndCenter (w, h);
  }

  bool resizable = false;
  {
    const ScopedGlContextRelease gl_release;
    resizable = view->canResize () == kResultTrue;
  }
  pimpl_->editor_window_->setResizable (resizable);

  const auto embed_id = pimpl_->editor_window_->getEmbedWindowId ();

  tresult attach_result;
  {
    const ScopedGlContextRelease gl_release;
    claim_plugin_context (
      static_cast<Vst::IHostApplication *> (pimpl_->host_app_));
    attach_result =
      view->attached (reinterpret_cast<void *> (embed_id), platform_type);
  }
  if (attach_result != kResultOk)
    {
      z_warning ("VST3: failed to attach plugin editor view");
      {
        const ScopedGlContextRelease gl_release;
        view->setFrame (nullptr);
      }
      pimpl_->plug_frame_.reset ();
      pimpl_->editor_window_.reset ();
      set_native_ui_unavailable (true);
      return;
    }

  pimpl_->view_attached_ = true;

  // Tell the view its size: plugins size their platform window in response
  // to onSize(). Re-query because the view may have issued resizeView()
  // during attached()
  {
    const ScopedGlContextRelease gl_release;
    if (
      view->getSize (&size) == kResultOk && size.getWidth () > 0
      && size.getHeight () > 0)
      {
        view->onSize (&size);
      }
  }

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

  {
    const ScopedGlContextRelease gl_release;
    // VSTGUI unregisters its run-loop watches via the plugin context here
    claim_plugin_context (
      static_cast<Vst::IHostApplication *> (pimpl_->host_app_));
    pimpl_->view_->removed ();
    pimpl_->view_->setFrame (nullptr);
  }
  pimpl_->plug_frame_.reset ();
  pimpl_->editor_window_.reset ();
  pimpl_->view_attached_ = false;
}

units::sample_u32_t
Vst3Plugin::get_single_playback_latency () const
{
  return pimpl_->latency_.load (std::memory_order_acquire);
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

  // Find the class matching the scanned unique ID (a 32-bit hash of the
  // TUID string), falling back to name matching for projects saved before
  // the native scanner existed
  const auto &class_infos = module->getFactory ().classInfos ();
  const auto  matches_hash = [plugin_unique_id] (const auto &ci) {
    return Vst3PluginFormat::get_hash_for_range (ci.ID ().toString ())
           == plugin_unique_id;
  };
  const auto matches_name = [this] (const auto &ci) {
    return ci.name () == get_name ().str ();
  };
  auto class_info_it = std::ranges::find_if (class_infos, matches_hash);
  if (
    class_info_it != class_infos.end ()
    && std::ranges::find_if (
         std::ranges::next (class_info_it), class_infos.end (), matches_hash)
         != class_infos.end ())
    {
      // Multiple classes in this module hash to the same unique ID:
      // disambiguate by class name
      const auto by_name =
        std::ranges::find_if (class_infos, [&] (const auto &ci) {
          return matches_hash (ci) && matches_name (ci);
        });
      if (by_name == class_infos.end ())
        {
          z_warning (
            "VST3: unique ID hash collision in module '{}' with no "
            "name match; refusing to load",
            path);
          return false;
        }
      z_warning (
        "VST3: unique ID hash collision in module '{}'; matched class by "
        "name",
        path);
      class_info_it = by_name;
    }
  if (class_info_it == class_infos.end ())
    {
      class_info_it = std::ranges::find_if (class_infos, matches_name);
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

  pimpl_->host_app_ = owned (new Vst3HostApplication);
  Vst::PluginContextFactory::instance ().setPluginContext (
    static_cast<Vst::IHostApplication *> (pimpl_->host_app_));
  pimpl_->plug_provider_ = std::make_unique<Vst::PlugProvider> (
    module->getFactory (), *class_info_it, true);
  if (!pimpl_->plug_provider_->initialize ())
    {
      z_warning ("VST3: PlugProvider initialization failed for '{}'", path);
      pimpl_->plug_provider_.reset ();
      clear_plugin_context_if_current (
        static_cast<Vst::IHostApplication *> (pimpl_->host_app_));
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
      // All references into the module must be dropped before `module`
      // goes out of scope and unloads the shared library
      pimpl_->processor_.reset ();
      pimpl_->controller_.reset ();
      pimpl_->component_.reset ();
      pimpl_->plug_provider_.reset ();
      clear_plugin_context_if_current (
        static_cast<Vst::IHostApplication *> (pimpl_->host_app_));
      pimpl_->host_app_ = nullptr;
      return false;
    }
  // Optional interface for host-initiated edit notifications
  pimpl_->controller_host_editing_ =
    U::cast<Vst::IEditControllerHostEditing> (pimpl_->controller_);
  pimpl_->class_id_ = FUID::fromTUID (class_info_it->ID ().data ());

  pimpl_->component_handler_ = owned (new Vst3ComponentHandler (
    [this] (Vst::ParamID id, Vst::ParamValue value) {
      const auto it = pimpl_->vst3_params_.find (id);
      if (it == pimpl_->vst3_params_.end ())
        return;
      const auto param_index = it->second.param_index;
      if (param_index >= param_sync_.entries.size ())
        return;
      const auto normalized = static_cast<float> (value);
      set_param_pending_from_plugin (param_index, normalized);
      param_sync_.entries[param_index].last_from_plugin = normalized;
    },
    [this] (int32 flags) {
      // restartComponent may arrive on the audio thread (kLatencyChanged
      // during processing) or the UI thread; the handler below touches
      // main-thread state, so it is marshalled over. It is also deferred:
      // the component may only be re-initialized or re-activated once the
      // plugin's own restartComponent() call has returned
      const auto handler = [this, flags] {
        // The plugin may have been unloaded (e.g. a reload) between
        // posting and handling this action
        if (pimpl_->processor_ == nullptr || pimpl_->controller_ == nullptr)
          return;
        if ((flags & Vst::RestartFlags::kReloadComponent) != 0)
          {
            // The plugin demands a full re-initialization: tear down and
            // re-create the component, carrying the state over. This
            // supersedes any other flags in the same request
            if (main_thread_callbacks_.with_paused_processing_)
              {
                main_thread_callbacks_.with_paused_processing_ ([this] {
                  reload_component ();
                });
              }
            else
              {
                z_warning (
                  "VST3: plugin '{}' requested a reload but the host cannot "
                  "pause processing; reload the plugin",
                  get_node_name ());
              }
            return;
          }
        if ((flags & Vst::RestartFlags::kLatencyChanged) != 0)
          {
            pimpl_->latency_.store (
              units::samples (pimpl_->processor_->getLatencySamples ()),
              std::memory_order_release);
            notify_latency_changed ();
          }
        if ((flags & Vst::RestartFlags::kParamValuesChanged) != 0)
          {
            // The plugin reports that its parameter values changed outside
            // of individual edits (e.g. a program/preset change): re-sync
            // all exposed parameters from the controller
            for (const auto &[param_id, adapter] : pimpl_->vst3_params_)
              {
                const auto param_index = adapter.param_index;
                if (param_index >= param_sync_.entries.size ())
                  continue;
                const auto normalized = static_cast<float> (
                  pimpl_->controller_->getParamNormalized (param_id));
                set_param_pending_from_plugin (param_index, normalized);
                param_sync_.entries[param_index].last_from_plugin = normalized;
              }
          }
        if ((flags & Vst::RestartFlags::kMidiCCAssignmentChanged) != 0)
          {
            // The plugin's MIDI-CC mapping (e.g. MIDI learn) or
            // program-change parameter info changed: rebuild both
            rebuild_midi_cc_mapping ();
            decltype (pimpl_->rt_mapping_)::ScopedAccess<
              farbot::ThreadType::nonRealtime>
              rt_mapping{ pimpl_->rt_mapping_ };
            refresh_program_change_param (*pimpl_->controller_, *rt_mapping);
          }
        if ((flags & Vst::RestartFlags::kIoChanged) != 0)
          {
            // The bus layout changed: re-activate buses and re-prepare the
            // process data (including scratch buffers) with processing
            // paused, and reconcile the Zrythm port topology with the live
            // bus layout (ports are reconfigured/detached/created, never
            // destroyed). The scratch channel mapping bridges any transient
            // disagreement until the graph is rebuilt
            bool       graph_changed = false;
            const auto reactivate = [this, &graph_changed] {
              if (pimpl_->processor_ == nullptr || pimpl_->component_ == nullptr)
                return;
              // Per the kIoChanged contract, the component is deactivated
              // before its buses are re-activated
              const auto was_processing = pimpl_->processing_active_;
              if (was_processing)
                release_resources_impl ();
              for (
                const auto media_type :
                { Vst::MediaTypes::kAudio, Vst::MediaTypes::kEvent })
                {
                  for (const auto dir : { Vst::kInput, Vst::kOutput })
                    {
                      const auto bus_count =
                        pimpl_->component_->getBusCount (media_type, dir);
                      for (const auto i : std::views::iota (0, bus_count))
                        {
                          pimpl_->component_->activateBus (
                            media_type, dir, i, true);
                        }
                    }
                }
              pimpl_->event_in_bus_count_.store (
                pimpl_->component_->getBusCount (
                  Vst::MediaTypes::kEvent, Vst::kInput),
                std::memory_order_release);
              for (
                const auto flow :
                { dsp::PortFlow::Input, dsp::PortFlow::Output })
                {
                  graph_changed |=
                    dsp::reconcile_audio_bus_configuration (
                      registry (), *this, flow, get_audio_bus_configs (flow))
                      .graph_changed;
                }
              if (was_processing)
                {
                  prepare_plugin_for_processing (
                    units::sample_rate (static_cast<int> (pimpl_->sample_rate_)),
                    units::samples (
                      static_cast<uint32_t> (pimpl_->max_block_samples_)));
                }
              if (graph_changed)
                {
                  if (main_thread_callbacks_.graph_recalc_ != nullptr)
                    {
                      // Reconciling drops the buffers of ports whose
                      // arrangement changed and creates new ports unprepared;
                      // the recalculation's node preparation reallocates
                      // them, and must happen before processing resumes
                      main_thread_callbacks_.graph_recalc_ ();
                    }
                  else
                    {
                      z_warning (
                        "VST3: ports of '{}' changed but the host cannot "
                        "recalculate the processing graph; reload the plugin",
                        get_node_name ());
                    }
                }
            };
            if (main_thread_callbacks_.with_paused_processing_)
              {
                main_thread_callbacks_.with_paused_processing_ (reactivate);
              }
            else
              {
                z_warning (
                  "VST3: plugin '{}' changed its IO configuration but the "
                  "host cannot reconfigure; reload the plugin",
                  get_node_name ());
              }
          }
        static constexpr auto kHandledFlags =
          Vst::RestartFlags::kReloadComponent
          | Vst::RestartFlags::kLatencyChanged
          | Vst::RestartFlags::kParamValuesChanged
          | Vst::RestartFlags::kMidiCCAssignmentChanged
          | Vst::RestartFlags::kIoChanged;
        if ((flags & ~kHandledFlags) != 0)
          {
            // Remaining flags only affect cached metadata (param/bus
            // titles, program lists, note expression types), which this
            // host does not cache
            z_debug (
              "VST3: ignoring restart flags {:#x} for '{}'",
              flags & ~kHandledFlags, get_node_name ());
          }
      };
      post_main_thread_action_deferred (handler);
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
  pimpl_->event_in_bus_count_.store (
    pimpl_->component_->getBusCount (Vst::MediaTypes::kEvent, Vst::kInput),
    std::memory_order_release);

  pimpl_->module_ = std::move (module);

  if (generate_new_ports && !create_ports_from_vst3_component ())
    {
      z_warning (
        "VST3: refusing to load '{}': invalid bus layout", get_node_name ());
      unload_current_plugin ();
      return false;
    }
  if (!generate_new_ports)
    {
      // Ports were restored from the project: negotiate the saved bus
      // topology into the (still inactive) component
      restore_saved_bus_arrangements ();
    }

  // Build the MIDI CC -> ParamID translation table from the plugin's
  // IMidiMapping (the spec's CC mechanism). Params covered by it are not
  // exposed as Zrythm parameters; MIDI CC events are translated to parameter
  // changes at process time instead.
  rebuild_midi_cc_mapping ();

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

  // The teardown below destroys the plug-in's run loop handlers and
  // closes its fds; drop the watches first so no notifier observes a dead
  // fd and no timer fires into dead handlers
  if (pimpl_->host_app_ != nullptr)
    {
      pimpl_->host_app_->unregister_all ();
    }

  if (pimpl_->controller_ != nullptr && pimpl_->component_handler_ != nullptr)
    {
      pimpl_->controller_->setComponentHandler (nullptr);
    }
  pimpl_->component_handler_.reset ();
  // The view's lifetime must not exceed the controller's
  pimpl_->view_.reset ();
  pimpl_->view_check_done_ = false;
  pimpl_->processor_.reset ();
  pimpl_->controller_host_editing_ = nullptr;
  pimpl_->controller_.reset ();
  pimpl_->component_.reset ();
  pimpl_->plug_provider_.reset ();
  clear_plugin_context_if_current (
    static_cast<Vst::IHostApplication *> (pimpl_->host_app_));
  pimpl_->host_app_ = nullptr;
  pimpl_->process_data_.unprepare ();
  pimpl_->module_.reset ();
}

void
Vst3Plugin::reload_component ()
{
  // Carry the current state over to the new instance
  const auto state = save_state_impl ();
  const auto was_processing = pimpl_->processing_active_;

  // load_plugin() unloads the current instance first
  const auto &descr = configuration ()->descriptor ();
  if (
    !load_plugin (
      std::get<std::filesystem::path> (descr->path_or_id_), descr->unique_id_,
      false))
    {
      z_warning (
        "VST3: failed to reload plugin '{}' after it requested a reload",
        get_node_name ());
      return;
    }

  if (!state.empty ())
    load_state_impl (state);

  if (was_processing)
    {
      prepare_plugin_for_processing (
        units::sample_rate (static_cast<int> (pimpl_->sample_rate_)),
        units::samples (static_cast<uint32_t> (pimpl_->max_block_samples_)));
    }

  // The editor was torn down with the old instance: re-show it if it was
  // visible
  if (uiVisible ())
    {
      QMetaObject::invokeMethod (
        this, [this] () { on_ui_visibility_changed (); }, Qt::QueuedConnection);
    }
}

bool
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

  const auto create_audio_ports = [&] (Vst::BusDirection dir) {
    const auto bus_count =
      pimpl_->component_->getBusCount (Vst::MediaTypes::kAudio, dir);
    const bool is_input = dir == Vst::kInput;
    for (const auto i : std::views::iota (0, bus_count))
      {
        Vst::BusInfo bus_info{};
        if (
          pimpl_->component_->getBusInfo (
            Vst::MediaTypes::kAudio, dir, i, bus_info)
          != kResultOk)
          {
            z_warning (
              "VST3: getBusInfo failed for audio {} bus {} of '{}'",
              is_input ? "input" : "output", i, get_node_name ());
            return false;
          }
        if (bus_info.channelCount < 1 || bus_info.channelCount > 255)
          {
            z_warning (
              "VST3: audio {} bus {} of '{}' has an unsupported channel "
              "count ({})",
              is_input ? "input" : "output", i, get_node_name (),
              bus_info.channelCount);
            return false;
          }
        Vst::SpeakerArrangement vst3_arrangement = 0;
        if (
          pimpl_->processor_->getBusArrangement (dir, i, vst3_arrangement)
          != kResultOk)
          {
            vst3_arrangement = 0;
          }
        const dsp::SpeakerArrangement arrangement = vst3_speaker_arrangement::
          to_dsp (vst3_arrangement, bus_info.channelCount);
        const auto name = utils::Utf8String::from_utf8_encoded_string (
          Steinberg::Vst::StringConvert::convert (bus_info.name));
        auto port_ref = utils::create_object<dsp::AudioPort> (
          registry (), name,
          is_input ? dsp::PortFlow::Input : dsp::PortFlow::Output, arrangement,
          bus_info.busType == Vst::BusTypes::kMain
            ? dsp::AudioPort::Purpose::Main
            : dsp::AudioPort::Purpose::Sidechain);
        if (is_input)
          add_input_port (port_ref);
        else
          add_output_port (port_ref);
      }
    return true;
  };
  create_midi_ports (Vst::kInput);
  create_midi_ports (Vst::kOutput);
  return create_audio_ports (Vst::kInput) && create_audio_ports (Vst::kOutput);
}

std::vector<dsp::AudioBusConfig>
Vst3Plugin::get_audio_bus_configs (dsp::PortFlow flow) const
{
  std::vector<dsp::AudioBusConfig> configs;
  if (pimpl_->processor_ == nullptr || pimpl_->component_ == nullptr)
    return configs;

  const auto dir = flow == dsp::PortFlow::Input ? Vst::kInput : Vst::kOutput;
  const bool is_input = dir == Vst::kInput;
  const auto bus_count =
    pimpl_->component_->getBusCount (Vst::MediaTypes::kAudio, dir);
  for (const auto i : std::views::iota (0, bus_count))
    {
      Vst::BusInfo bus_info{};
      if (
        pimpl_->component_->getBusInfo (Vst::MediaTypes::kAudio, dir, i, bus_info)
        != kResultOk)
        {
          z_warning (
            "VST3: getBusInfo failed for audio {} bus {} of '{}'",
            is_input ? "input" : "output", i, get_node_name ());
          continue;
        }
      Vst::SpeakerArrangement vst3_arrangement = 0;
      if (
        pimpl_->processor_->getBusArrangement (dir, i, vst3_arrangement)
        != kResultOk)
        {
          vst3_arrangement = 0;
        }
      configs.push_back (
        dsp::AudioBusConfig{
          .name = utils::Utf8String::from_utf8_encoded_string (
            Steinberg::Vst::StringConvert::convert (bus_info.name)),
          .arrangement = vst3_speaker_arrangement::to_dsp (
            vst3_arrangement, bus_info.channelCount),
          .purpose =
            bus_info.busType == Vst::BusTypes::kMain
              ? dsp::AudioPort::Purpose::Main
              : dsp::AudioPort::Purpose::Sidechain,
          .active = true,
          // VST3 buses have no stable ids; matching is positional
          .external_id = std::nullopt });
    }
  return configs;
}

bool
Vst3Plugin::set_bus_arrangements (
  std::span<const dsp::SpeakerArrangement> input_arrangements,
  std::span<const dsp::SpeakerArrangement> output_arrangements)
{
  if (pimpl_->processor_ == nullptr || pimpl_->component_ == nullptr)
    return false;

  // Build one request entry per live bus; arrangements with no VST3
  // representation keep the bus's current arrangement
  const auto build_request =
    [this] (
      Vst::BusDirection dir, std::span<const dsp::SpeakerArrangement> desired) {
      const auto bus_count =
        pimpl_->component_->getBusCount (Vst::MediaTypes::kAudio, dir);
      std::vector<Vst::SpeakerArrangement> request;
      request.reserve (static_cast<size_t> (bus_count));
      for (const auto i : std::views::iota (0, bus_count))
        {
          Vst::SpeakerArrangement current = 0;
          if (
            pimpl_->processor_->getBusArrangement (dir, i, current) != kResultOk)
            {
              current = 0;
            }
          auto requested =
            i < static_cast<Steinberg::int32> (desired.size ())
              ? vst3_speaker_arrangement::from_dsp (
                  desired[static_cast<size_t> (i)])
              : Vst::SpeakerArrangement{ 0 };
          request.push_back (requested != 0 ? requested : current);
        }
      return request;
    };
  auto input_request = build_request (Vst::kInput, input_arrangements);
  auto output_request = build_request (Vst::kOutput, output_arrangements);

  // setBusArrangements is only valid while the component is inactive
  const auto was_processing = pimpl_->processing_active_;
  if (was_processing)
    release_resources_impl ();

  const auto result = pimpl_->processor_->setBusArrangements (
    input_request.data (), static_cast<Steinberg::int32> (input_request.size ()),
    output_request.data (),
    static_cast<Steinberg::int32> (output_request.size ()));

  if (was_processing)
    {
      prepare_plugin_for_processing (
        units::sample_rate (static_cast<int> (pimpl_->sample_rate_)),
        units::samples (static_cast<uint32_t> (pimpl_->max_block_samples_)));
    }

  return result == kResultOk;
}

void
Vst3Plugin::restore_saved_bus_arrangements ()
{
  if (pimpl_->processor_ == nullptr || pimpl_->component_ == nullptr)
    return;

  const auto saved_arrangements = [this] (dsp::PortFlow flow) {
    const auto port_refs =
      flow == dsp::PortFlow::Input
        ? get_all_input_ports ()
        : get_all_output_ports ();
    return port_refs | std::views::transform (&dsp::PortUuidReference::get)
           | utils::views::qobject_cast_and_filter<dsp::AudioPort>
           | std::views::transform (&dsp::AudioPort::arrangement)
           | std::ranges::to<std::vector> ();
  };
  const auto saved_inputs = saved_arrangements (dsp::PortFlow::Input);
  const auto saved_outputs = saved_arrangements (dsp::PortFlow::Output);

  // Skip the negotiation when the live layout already matches the restored
  // topology
  const auto matches_live =
    [this] (
      dsp::PortFlow flow, const std::vector<dsp::SpeakerArrangement> &saved) {
      const auto live = get_audio_bus_configs (flow);
      return std::ranges::equal (
        saved, live | std::views::transform (&dsp::AudioBusConfig::arrangement));
    };
  if (
    matches_live (dsp::PortFlow::Input, saved_inputs)
    && matches_live (dsp::PortFlow::Output, saved_outputs))
    return;

  const auto accepted = set_bus_arrangements (saved_inputs, saved_outputs);
  if (!accepted)
    {
      z_warning (
        "VST3: plugin '{}' refused the saved bus configuration; adopting its "
        "current layout",
        get_node_name ());
    }

  // Sync the ports to the accepted (live) configuration. No graph recalc
  // request here: loading always recalculates the graph afterwards
  bool ports_changed = false;
  for (const auto flow : { dsp::PortFlow::Input, dsp::PortFlow::Output })
    {
      const auto result = dsp::reconcile_audio_bus_configuration (
        registry (), *this, flow, get_audio_bus_configs (flow));
      ports_changed |= result.graph_changed || result.metadata_changed;
    }
  if (ports_changed)
    {
      z_debug (
        "VST3: reconciled ports of '{}' with the accepted bus configuration",
        get_node_name ());
    }
}

void
Vst3Plugin::rebuild_midi_cc_mapping ()
{
  // Mutate the table through a nonRealtime farbot access: the audio thread
  // keeps reading the previously published snapshot until the updated table
  // is published on scope exit
  decltype (pimpl_->rt_mapping_)::ScopedAccess<farbot::ThreadType::nonRealtime>
    rt_mapping{ pimpl_->rt_mapping_ };

  rt_mapping->midi_cc_param_ids_.fill (Vst::kNoParamId);
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
                  rt_mapping->midi_cc_param_ids_[static_cast<size_t> (
                    ch * Vst::kCountCtrlNumber + ctrl)] = param_id;
                  pimpl_->midi_cc_param_id_set_.insert (param_id);
                }
            }
        }
    }
}

/**
 * @brief Re-reads the kIsProgramChange parameter (id and step count) from
 * the controller into the given realtime-published mapping.
 *
 * Called at load and on RestartFlags::kMidiCCAssignmentChanged, which also
 * covers program-change parameter info changes. The caller must hold the
 * nonRealtime farbot access for @p rt_mapping.
 */
static void
refresh_program_change_param (
  Vst::IEditController &controller,
  Vst3RtParamMapping   &rt_mapping)
{
  rt_mapping.program_change_param_id_.reset ();
  rt_mapping.program_change_step_count_ = 0;

  const auto param_count = controller.getParameterCount ();
  for (const auto i : std::views::iota (0, param_count))
    {
      Vst::ParameterInfo info{};
      if (controller.getParameterInfo (i, info) != kResultOk)
        continue;
      if ((info.flags & Vst::ParameterInfo::kIsProgramChange) != 0)
        {
          rt_mapping.program_change_param_id_ = info.id;
          rt_mapping.program_change_step_count_ = info.stepCount;
          break;
        }
    }
}

void
Vst3Plugin::create_parameters_from_vst3_controller ()
{
  // Single nonRealtime access for the whole rebuild (nested accesses on the
  // same thread would deadlock: farbot's nonRealtime lock is not recursive)
  decltype (pimpl_->rt_mapping_)::ScopedAccess<farbot::ThreadType::nonRealtime>
    rt_mapping{ pimpl_->rt_mapping_ };

  pimpl_->vst3_params_.clear ();
  rt_mapping->zrythm_to_vst3_.clear ();
  refresh_program_change_param (*pimpl_->controller_, *rt_mapping);

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
          // Handled via MIDI program change translation, not exposed as a
          // Zrythm parameter (consistent with MIDI-CC-mapped params). The id
          // and step count are tracked by refresh_program_change_param()
          continue;
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
      rt_mapping->zrythm_to_vst3_.emplace (zrythm_param, info.id);
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
  pimpl_->output_param_changes_.prepare (
    pimpl_->vst3_params_.size () + kMaxMidiCcQueuesPerBlock);

  // Pre-size event lists so addEvent() never allocates on the audio thread
  pimpl_->input_events_.setMaxSize (max_block * 4);
  pimpl_->output_events_.setMaxSize (max_block * 4);

  pimpl_->process_data_.unprepare ();
  // HostProcessData is kept non-owning (bufferSamples = 0): the process
  // loop points channels at Zrythm port buffers or our own scratch, and
  // unprepare() would otherwise delete[] pointers it doesn't own
  pimpl_->process_data_.prepare (
    *pimpl_->component_, 0, Vst::SymbolicSampleSizes::kSample32);

  pimpl_->input_scratch_ = make_vst3_scratch_buffers (
    pimpl_->process_data_.inputs, pimpl_->process_data_.numInputs, max_block);
  pimpl_->output_scratch_ = make_vst3_scratch_buffers (
    pimpl_->process_data_.outputs, pimpl_->process_data_.numOutputs, max_block);
  pimpl_->max_block_samples_ = max_block;
  pimpl_->audio_in_buf_ptrs_.reserve (audio_in_ports_.size ());
  pimpl_->audio_out_buf_ptrs_.reserve (audio_out_ports_.size ());

  if (
    pimpl_->processor_->canProcessSampleSize (Vst::SymbolicSampleSizes::kSample32)
    != kResultOk)
    {
      z_warning ("VST3: plugin does not support 32-bit sample processing");
      return;
    }

  // Per the VST3 lifecycle, setupProcessing must be called in the disabled
  // state, before setActive(true)
  Vst::ProcessSetup setup{
    Vst::ProcessModes::kRealtime, Vst::SymbolicSampleSizes::kSample32, max_block,
    static_cast<Steinberg::Vst::SampleRate> (sample_rate.in (units::sample_rate))
  };
  if (pimpl_->processor_->setupProcessing (setup) != kResultOk)
    {
      z_warning ("VST3: setupProcessing failed");
      return;
    }

  if (pimpl_->component_->setActive (true) != kResultOk)
    {
      z_warning ("VST3: setActive(true) failed");
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
  pimpl_->sample_rate_ =
    static_cast<double> (sample_rate.in (units::sample_rate));
  pimpl_->latency_.store (
    units::samples (pimpl_->processor_->getLatencySamples ()),
    std::memory_order_release);
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
Vst3Plugin::process_impl (
  dsp::graph::ProcessBlockInfo time_info,
  const dsp::ITransport       &transport,
  const dsp::TempoMap         &tempo_map) noexcept
{
  auto &impl = *pimpl_;

  if (!impl.processing_active_)
    return;

  const auto nframes = time_info.nframes_.in<int32_t> (units::samples);
  const auto local_offset =
    time_info.buffer_offset_.in<int32_t> (units::samples);

  // Realtime access to the published param routing snapshot for this block
  decltype (pimpl_->rt_mapping_)::ScopedAccess<farbot::ThreadType::realtime>
    rt_mapping{ pimpl_->rt_mapping_ };

  // Fill input events from the MIDI input ports (one per event bus). Note
  // on/off become VST note events; polyphonic aftertouch becomes a poly
  // pressure event; SysEx becomes a data event; MIDI CC / channel pressure /
  // pitch bend / program change are translated to parameter changes (via the
  // plugin's IMidiMapping table / kIsProgramChange param).
  impl.input_events_.clear ();
  impl.input_param_changes_.clear_queues ();
  impl.output_param_changes_.clear_queues ();
  for (
    const auto &[bus_idx, midi_in_port] :
    utils::views::enumerate (midi_in_ports_))
    {
      // The port buffer holds events for the whole cycle: only emit events
      // inside this chunk, re-based to chunk-relative times
      const auto buffer_offset = time_info.buffer_offset_;
      const auto chunk_end = buffer_offset + time_info.nframes_;
      auto       chunk_events =
        midi_in_port->buffer_
        | std::views::filter ([buffer_offset, chunk_end] (const auto &ev) {
            return ev.time () >= buffer_offset && ev.time () < chunk_end;
          });
      const auto bus_index = static_cast<int16> (bus_idx);
      // The Zrythm port topology is deliberately kept when the plugin
      // shrinks its event buses (kIoChanged): drop events targeting buses
      // the plugin no longer has
      if (bus_index >= impl.event_in_bus_count_.load (std::memory_order_acquire))
        {
          if (chunk_events.begin () != chunk_events.end ())
            impl.note_stale_bus_event_drop ();
          continue;
        }
      for (const auto &ev : chunk_events)
        {
          const auto ev_data = ev.data ();
          const auto sample_offset =
            (ev.time () - buffer_offset).in<int32_t> (units::samples);

          // SysEx -> data event (bytes must stay valid during process(); the
          // MIDI port buffer outlives the process call)
          if (!ev_data.empty () && ev_data[0] == 0xF0)
            {
              Vst::Event vst_ev{};
              vst_ev.busIndex = bus_index;
              vst_ev.sampleOffset = sample_offset;
              vst_ev.ppqPosition = 0;
              vst_ev.type = Vst::Event::kDataEvent;
              vst_ev.data.size = static_cast<uint32_t> (ev_data.size ());
              vst_ev.data.type = Vst::DataEvent::kMidiSysEx;
              vst_ev.data.bytes =
                reinterpret_cast<const uint8_t *> (ev_data.data ());
              if (impl.input_events_.addEvent (vst_ev) != kResultOk)
                pimpl_->note_input_event_drop ();
              continue;
            }

          if (ev_data.size () < 2)
            continue;

          const auto status = ev_data[0] & 0xF0;
          const auto channel = ev_data[0] & 0x0F;

          // Program change -> kIsProgramChange param. The translation table
          // only covers bus 0
          if (status == 0xC0)
            {
              if (
                bus_index == 0
                && rt_mapping->program_change_param_id_.has_value ()
                && rt_mapping->program_change_step_count_ > 0)
                {
                  int32  queue_index = 0;
                  auto * queue = impl.input_param_changes_.addParameterData (
                    *rt_mapping->program_change_param_id_, queue_index);
                  if (queue == nullptr)
                    {
                      pimpl_->note_param_queue_drop ();
                      continue;
                    }

                  // Clamp the program number to the step count so the
                  // normalized value stays in [0, 1] for plugins with
                  // fewer than 128 programs
                  const auto program = std::min<int32_t> (
                    ev_data[1], rt_mapping->program_change_step_count_);
                  int32 point_index = 0;
                  if (
                    queue->addPoint (
                      sample_offset,
                      static_cast<Vst::ParamValue> (program)
                        / rt_mapping->program_change_step_count_,
                      point_index)
                    != kResultTrue)
                    {
                      pimpl_->note_input_point_drop ();
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
              vst_ev.busIndex = bus_index;
              vst_ev.sampleOffset = sample_offset;
              vst_ev.ppqPosition = 0;
              vst_ev.type = Vst::Event::kPolyPressureEvent;
              vst_ev.polyPressure.channel = static_cast<int16_t> (channel);
              vst_ev.polyPressure.pitch = static_cast<int16_t> (ev_data[1]);
              vst_ev.polyPressure.pressure =
                static_cast<float> (ev_data[2]) / 127.f;
              vst_ev.polyPressure.noteId = -1;
              if (impl.input_events_.addEvent (vst_ev) != kResultOk)
                pimpl_->note_input_event_drop ();
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
              // The MIDI-CC translation table only covers bus 0
              if (bus_index != 0)
                continue;
              const auto param_id = rt_mapping->midi_cc_param_ids_[static_cast<
                size_t> (channel * Vst::kCountCtrlNumber + *ctrl_number)];
              if (param_id != Vst::kNoParamId)
                {
                  int32  queue_index = 0;
                  auto * queue = impl.input_param_changes_.addParameterData (
                    param_id, queue_index);
                  if (queue == nullptr)
                    {
                      pimpl_->note_param_queue_drop ();
                      continue;
                    }
                  int32 point_index = 0;
                  if (
                    queue->addPoint (sample_offset, ctrl_value, point_index)
                    != kResultTrue)
                    {
                      pimpl_->note_input_point_drop ();
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
          vst_ev.busIndex = bus_index;
          vst_ev.sampleOffset = sample_offset;
          vst_ev.ppqPosition = 0;
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
              vst_ev.noteOff.velocity =
                status == 0x80 ? static_cast<float> (ev_data[2]) / 127.f : 0.f;
              vst_ev.noteOff.tuning = 0.f;
              vst_ev.noteOff.noteId = -1;
            }
          if (impl.input_events_.addEvent (vst_ev) != kResultOk)
            pimpl_->note_input_event_drop ();
        }
    }

  // Point the bus channel buffers directly at the Zrythm port buffers
  // (zero-copy). Plugin bus channels with no corresponding port buffer
  // (layout disagreement, e.g. after kIoChanged) are routed to scratch so
  // every pointer handed to the plugin is valid
  impl.audio_in_buf_ptrs_.clear ();
  for (const auto * port : audio_in_ports_)
    impl.audio_in_buf_ptrs_.push_back (
      port->buffers () != nullptr ? port->buffers ().get () : nullptr);
  map_vst3_channel_buffers (
    impl.process_data_.inputs, impl.process_data_.numInputs,
    impl.audio_in_buf_ptrs_, impl.input_scratch_, local_offset, nframes, true);
  impl.audio_out_buf_ptrs_.clear ();
  for (const auto * port : audio_out_ports_)
    impl.audio_out_buf_ptrs_.push_back (
      port->buffers () != nullptr ? port->buffers ().get () : nullptr);
  map_vst3_channel_buffers (
    impl.process_data_.outputs, impl.process_data_.numOutputs,
    impl.audio_out_buf_ptrs_, impl.output_scratch_, local_offset, nframes,
    false);

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
          change.modulated_value,
          entry.last_from_plugin.load (std::memory_order_relaxed)))
        {
          entry.last_from_plugin = -1.f;
          continue;
        }
      const auto it = rt_mapping->zrythm_to_vst3_.find (change.param);
      if (it == rt_mapping->zrythm_to_vst3_.end ())
        continue;
      int32  queue_index = 0;
      auto * queue =
        impl.input_param_changes_.addParameterData (it->second, queue_index);
      if (queue == nullptr)
        {
          pimpl_->note_param_queue_drop ();
          continue;
        }
      int32 point_index = 0;
      if (
        queue->addPoint (0, change.modulated_value, point_index) != kResultTrue)
        {
          pimpl_->note_input_point_drop ();
        }

      // Also notify the edit controller on the main thread, or a
      // separate-component plugin (and its own UI) never learns about
      // host-initiated changes. Deferred so the controller call never runs
      // inline inside process_impl's realtime context when processing
      // happens to run on the main thread (e.g. in tests)
      const auto  param_id = it->second;
      const float value = change.modulated_value;
      if (!post_main_thread_action_deferred ([this, param_id, value] {
            if (pimpl_->controller_ == nullptr)
              return;
            if (pimpl_->controller_host_editing_ != nullptr)
              pimpl_->controller_host_editing_->beginEditFromHost (param_id);
            pimpl_->controller_->setParamNormalized (
              param_id, static_cast<Vst::ParamValue> (value));
            if (pimpl_->controller_host_editing_ != nullptr)
              pimpl_->controller_host_editing_->endEditFromHost (param_id);
          }))
        {
          pimpl_->note_controller_notification_drop ();
        }
    }

  impl.process_data_.numSamples = nframes;
  impl.process_data_.processMode = Vst::ProcessModes::kRealtime;
  impl.process_data_.inputParameterChanges = &impl.input_param_changes_;
  // Output queues are provided per the IParameterChanges contract; their
  // contents (plugin-reported parameter/latency outputs) are not consumed
  // yet
  impl.process_data_.outputParameterChanges = &impl.output_param_changes_;
  impl.process_data_.inputEvents = &impl.input_events_;
  impl.process_data_.outputEvents = &impl.output_events_;
  impl.process_context_ = {};
  impl.process_context_.sampleRate = impl.sample_rate_;
  impl.process_context_.projectTimeSamples =
    time_info.transport_position_.in<int64_t> (units::samples);

  const auto transport_context = build_plugin_transport_context (
    transport, tempo_map, time_info.transport_position_);
  impl.process_context_.state =
    Vst::ProcessContext::kProjectTimeMusicValid
    | Vst::ProcessContext::kBarPositionValid | Vst::ProcessContext::kTempoValid
    | Vst::ProcessContext::kTimeSigValid | Vst::ProcessContext::kCycleValid
    | (transport_context.playing_ ? Vst::ProcessContext::kPlaying : 0)
    | (transport_context.recording_ ? Vst::ProcessContext::kRecording : 0)
    | (transport_context.loop_enabled_ ? Vst::ProcessContext::kCycleActive : 0);
  impl.process_context_.projectTimeMusic =
    transport_context.position_.in (units::quarter_notes);
  impl.process_context_.barPositionMusic =
    transport_context.bar_start_.in (units::quarter_notes);
  impl.process_context_.cycleStartMusic =
    transport_context.loop_start_.in (units::quarter_notes);
  impl.process_context_.cycleEndMusic =
    transport_context.loop_end_.in (units::quarter_notes);
  impl.process_context_.tempo = transport_context.tempo_.in (units::bpm);
  impl.process_context_.timeSigNumerator = transport_context.time_sig_numerator_;
  impl.process_context_.timeSigDenominator =
    transport_context.time_sig_denominator_;

  impl.process_data_.processContext = &impl.process_context_;

  {
#if defined(__has_feature) && __has_feature(realtime_sanitizer)
    // Not our code, we don't care about RTSan violations here
    __rtsan::ScopedDisabler d;
#endif
    impl.processor_->process (impl.process_data_);
  }

  // Drain output events into the MIDI output ports (note on/off only),
  // routed by the event's bus index
  if (!midi_out_ports_.empty ())
    {
      const auto event_count = impl.output_events_.getEventCount ();
      for (const auto i : std::views::iota (0, event_count))
        {
          Vst::Event vst_ev{};
          if (impl.output_events_.getEvent (i, vst_ev) != kResultOk)
            continue;

          if (
            const auto violation = validate_vst3_output_event (vst_ev, nframes))
            {
              impl.note_invalid_output_event_drop (*violation);
              continue;
            }

          if (
            vst_ev.busIndex < 0
            || static_cast<size_t> (vst_ev.busIndex) >= midi_out_ports_.size ())
            {
              impl.note_invalid_output_event_drop ("bus index out of range"sv);
              continue;
            }
          auto * midi_out_port =
            midi_out_ports_[static_cast<size_t> (vst_ev.busIndex)];

          const auto time =
            units::samples (static_cast<uint64_t> (vst_ev.sampleOffset))
            + time_info.buffer_offset_;
          if (vst_ev.type == Vst::Event::kNoteOnEvent)
            {
              const auto midi_ev = dsp::midi_event::make_note_on (
                static_cast<midi_byte_t> (vst_ev.noteOn.channel),
                static_cast<midi_byte_t> (vst_ev.noteOn.pitch),
                static_cast<midi_byte_t> (
                  std::lround (vst_ev.noteOn.velocity * 127.f)),
                time);
              midi_out_port->buffer_.push_back (midi_ev.time_, midi_ev.data ());
            }
          else if (vst_ev.type == Vst::Event::kNoteOffEvent)
            {
              const auto midi_ev = dsp::midi_event::make_note_off (
                static_cast<midi_byte_t> (vst_ev.noteOff.channel),
                static_cast<midi_byte_t> (vst_ev.noteOff.pitch),
                static_cast<midi_byte_t> (
                  std::lround (vst_ev.noteOff.velocity * 127.f)),
                time);
              midi_out_port->buffer_.push_back (midi_ev.time_, midi_ev.data ());
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

bool
Vst3Plugin::load_state_impl (const std::string &base64_state)
{
  auto data = QByteArray::fromBase64 (QByteArray::fromStdString (base64_state));

  if (!pimpl_ || pimpl_->component_ == nullptr || pimpl_->controller_ == nullptr)
    {
      // Not instantiated yet - apply during instantiation
      state_to_apply_ = std::move (data);
      return true;
    }

  // IComponent::setState and IEditController::setComponentState/setState are
  // UI-thread calls allowed while processing, so the state can be applied
  // immediately
  return apply_state_from_byte_array (data);
}

bool
Vst3Plugin::apply_state_from_byte_array (const QByteArray &data)
{
  if (pimpl_->component_ == nullptr || pimpl_->controller_ == nullptr)
    return false;

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
      return false;
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
  return true;
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
