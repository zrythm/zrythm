// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <atomic>
#include <string>

#include "base/source/fstreamer.h"
#include "public.sdk/source/main/pluginfactory.h"
#include "public.sdk/source/vst/vstsinglecomponenteffect.h"
#include <nlohmann/json.hpp>

namespace zrythm_test_plugins
{

using namespace Steinberg;
using namespace Steinberg::Vst;

static const FUID
  TestRestartUID (0x5C1D9E27, 0x3A8B4F60, 0xD2E6A194, 0xB7C05F38);

/**
 * @brief Effect that asks the host to restart it, for testing the host's
 * restartComponent handling.
 *
 * Exposes its results as a structured (JSON) controller state: whether a
 * bus was (re)activated while the component was active
 * ("busesActivatedWhileActive") and the total initialize() call count of
 * this class ("initializeCount").
 */
class TestRestart : public SingleComponentEffect
{
public:
  static constexpr ParamID kTriggerIoChangeParamId = 0;
  static constexpr ParamID kTriggerReloadParamId = 1;
  static constexpr ParamID kTriggerGrowOutputParamId = 2;
  static constexpr ParamID kTriggerShrinkOutputParamId = 3;

  /** initialize() calls across all instances of this class (the module
   * stays loaded in the host process), so hosts can verify that
   * kReloadComponent recreated the component. */
  static std::atomic<double> &initialize_count ()
  {
    static std::atomic<double> count{ 0.0 };
    return count;
  }

  tresult PLUGIN_API initialize (FUnknown * context) SMTG_OVERRIDE
  {
    const auto res = SingleComponentEffect::initialize (context);
    if (res != kResultOk)
      return res;

    initialize_count ().fetch_add (1.0);

    addAudioInput (STR16 ("Input"), SpeakerArr::kStereo);
    addAudioOutput (STR16 ("Output"), SpeakerArr::kStereo);
    // Toggles that make the fixture request a host restart; each
    // auto-resets so that restored state does not re-fire the request
    parameters.addParameter (
      STR16 ("Trigger IO Change"), STR16 (""), 1, 0.0,
      ParameterInfo::kCanAutomate, kTriggerIoChangeParamId);
    parameters.addParameter (
      STR16 ("Trigger Reload"), STR16 (""), 1, 0.0, ParameterInfo::kCanAutomate,
      kTriggerReloadParamId);
    // Toggles that add/remove an audio output bus before requesting the IO
    // restart, so hosts can verify runtime bus reconciliation
    parameters.addParameter (
      STR16 ("Grow Output"), STR16 (""), 1, 0.0, ParameterInfo::kCanAutomate,
      kTriggerGrowOutputParamId);
    parameters.addParameter (
      STR16 ("Shrink Output"), STR16 (""), 1, 0.0, ParameterInfo::kCanAutomate,
      kTriggerShrinkOutputParamId);
    return kResultOk;
  }

  tresult PLUGIN_API setActive (TBool state) SMTG_OVERRIDE
  {
    active_ = state;
    return SingleComponentEffect::setActive (state);
  }

  tresult PLUGIN_API
  activateBus (MediaType type, BusDirection dir, int32 index, TBool state)
    SMTG_OVERRIDE
  {
    // Per the VST3 lifecycle, buses are only (de)activated while the
    // component is deactivated
    if (state && active_)
      bus_activated_while_active_ = true;
    return SingleComponentEffect::activateBus (type, dir, index, state);
  }

  tresult PLUGIN_API
  setParamNormalized (ParamID tag, ParamValue value) SMTG_OVERRIDE
  {
    const auto res = EditControllerEx1::setParamNormalized (tag, value);
    if (res != kResultOk)
      return res;

    if (value > 0.5 && componentHandler != nullptr)
      {
        const auto fire = [this, tag] (int32 flag, bool &fired) {
          if (!fired)
            {
              fired = true;
              componentHandler->restartComponent (flag);
              // Reset the toggle so that restored state does not re-fire
              // the request
              EditControllerEx1::setParamNormalized (tag, 0.0);
            }
        };
        if (tag == kTriggerIoChangeParamId)
          fire (RestartFlags::kIoChanged, io_change_fired_);
        else if (tag == kTriggerReloadParamId)
          fire (RestartFlags::kReloadComponent, reload_fired_);
        else if (tag == kTriggerGrowOutputParamId && audioOutputs.size () == 1)
          {
            addAudioOutput (STR16 ("Out 2"), SpeakerArr::kStereo);
            fire (RestartFlags::kIoChanged, grow_fired_);
          }
        else if (tag == kTriggerShrinkOutputParamId && audioOutputs.size () > 1)
          {
            audioOutputs.erase (audioOutputs.begin () + 1);
            fire (RestartFlags::kIoChanged, shrink_fired_);
          }
      }
    return kResultOk;
  }

  tresult PLUGIN_API setEditorState (IBStream * state) SMTG_OVERRIDE
  {
    // Informational state only - nothing to restore
    IBStreamer streamer (state, kLittleEndian);
    int32      length = 0;
    if (!streamer.readInt32 (length))
      return kResultFalse;
    if (length <= 0)
      return kResultFalse;
    std::string json_text (static_cast<size_t> (length), '\0');
    if (streamer.readRaw (json_text.data (), length) != length)
      return kResultFalse;
    return kResultOk;
  }

  tresult PLUGIN_API getEditorState (IBStream * state) SMTG_OVERRIDE
  {
    const nlohmann::json j{
      { "busesActivatedWhileActive", bus_activated_while_active_ },
      { "initializeCount",           initialize_count ().load () },
    };
    const auto json_text = j.dump ();
    IBStreamer streamer (state, kLittleEndian);
    streamer.writeInt32 (static_cast<int32> (json_text.size ()));
    streamer.writeRaw (
      json_text.data (), static_cast<int32> (json_text.size ()));
    return kResultOk;
  }

  tresult PLUGIN_API setBusArrangements (
    SpeakerArrangement * inputs,
    int32                numIns,
    SpeakerArrangement * outputs,
    int32                numOuts) SMTG_OVERRIDE
  {
    if (
      numIns == 1 && numOuts == 1 && inputs[0] == SpeakerArr::kStereo
      && outputs[0] == SpeakerArr::kStereo)
      return kResultOk;
    return kResultFalse;
  }

  tresult PLUGIN_API
  canProcessSampleSize (int32 symbolicSampleSize) SMTG_OVERRIDE
  {
    return symbolicSampleSize == kSample32 ? kResultTrue : kResultFalse;
  }

  tresult PLUGIN_API process (ProcessData &data) SMTG_OVERRIDE
  {
    if (data.numSamples <= 0 || data.numOutputs < 1)
      return kResultOk;
    for (int32 ch = 0; ch < data.outputs[0].numChannels; ++ch)
      {
        std::fill_n (
          data.outputs[0].channelBuffers32[ch],
          static_cast<uint32_t> (data.numSamples), 0.0f);
        data.outputs[0].silenceFlags |= (uint64{ 1 } << ch);
      }
    return kResultOk;
  }

  static FUnknown * createInstance (void *)
  {
    return static_cast<IAudioProcessor *> (new TestRestart ());
  }

private:
  bool active_ = false;
  bool bus_activated_while_active_ = false;
  bool io_change_fired_ = false;
  bool reload_fired_ = false;
  bool grow_fired_ = false;
  bool shrink_fired_ = false;
};

} // namespace zrythm_test_plugins

using zrythm_test_plugins::TestRestart;
using zrythm_test_plugins::TestRestartUID;

BEGIN_FACTORY_DEF ("Zrythm", "https://zrythm.org", "mailto:contact@zrythm.org")
DEF_CLASS2 (
  INLINE_UID_FROM_FUID (TestRestartUID),
  PClassInfo::kManyInstances,
  kVstAudioEffectClass,
  "Test Restart",
  0,
  Vst::PlugType::kFx,
  "1.0.0",
  kVstVersionString,
  TestRestart::createInstance)
END_FACTORY
