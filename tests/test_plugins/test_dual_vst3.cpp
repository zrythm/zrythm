// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

// Dual-component VST3 test fixture: the processor and the edit
// controller are separate classes, like JUCE 8-built VST3 plugins and
// the VST3 SDK's recommended architecture. The controller keeps its
// own state and only learns about processor-reported values when the
// host calls setParamNormalized on it.

#include <atomic>
#include <cmath>
#include <cstring>
#include <string>

#include "base/source/fstreamer.h"
#include "gain_dsp.h"
#include "pluginterfaces/vst/ivsteditcontroller.h"
#include "pluginterfaces/vst/ivstparameterchanges.h"
#include "public.sdk/source/main/pluginfactory.h"
#include "public.sdk/source/vst/vstaudioeffect.h"
#include "public.sdk/source/vst/vsteditcontroller.h"
#include <nlohmann/json.hpp>

namespace zrythm_test_plugins
{

using namespace Steinberg;
using namespace Steinberg::Vst;

static const FUID
  TestDualGainUID (0x5B7A1C29, 0x8E4F6D30, 0xA2C3B4D5, 0x7F8E9D0A);
static const FUID
  TestDualGainControllerUID (0x6C8B2D3A, 0x9F507E41, 0xB3D4C5E6, 0x8A9FAE1B);

// Module-level state shared by the processor and the controller (both
// live in the same shared library), exposed through the controller
// state chunk so hosts can verify controller-side behavior
static std::atomic<int>    dual_controller_sync_count{ 0 };
static std::atomic<double> dual_last_synced_level{ 1.0 };
static std::atomic<int>    dual_host_edit_wrap_count{ 0 };
static std::atomic<int>    dual_level_input_count{ 0 };

/** Processor side of the dual-component fixture. */
class TestDualGain : public AudioEffect
{
public:
  static constexpr ParamID kLevelParamId = 0;
  static constexpr ParamID kAutoReportParamId = 1;
  /** Fixed normalized Level value reported from process() while Auto
   * Report is on */
  static constexpr ParamValue kReportedLevel = 0.25;

  DELEGATE_REFCOUNT (AudioEffect)

  tresult PLUGIN_API initialize (FUnknown * context) SMTG_OVERRIDE
  {
    const auto res = AudioEffect::initialize (context);
    if (res != kResultOk)
      return res;

    addAudioInput (STR16 ("Input"), SpeakerArr::kStereo);
    addAudioOutput (STR16 ("Output"), SpeakerArr::kStereo);
    return kResultOk;
  }

  tresult PLUGIN_API getControllerClassId (TUID classID) SMTG_OVERRIDE
  {
    std::memcpy (classID, TestDualGainControllerUID, sizeof (TUID));
    return kResultTrue;
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

  tresult PLUGIN_API setState (IBStream * state) SMTG_OVERRIDE
  {
    // The state holds the Level value; parse errors leave the current
    // value (an empty state is valid)
    IBStreamer streamer (state, kLittleEndian);
    int32      length = 0;
    if (!streamer.readInt32 (length) || length <= 0)
      return kResultOk;
    std::string json_text (static_cast<size_t> (length), '\0');
    if (streamer.readRaw (json_text.data (), length) != length)
      return kResultOk;
    const auto j = nlohmann::json::parse (json_text, nullptr, false);
    if (!j.is_discarded () && j.contains ("level"))
      level_.store (j["level"].get<double> ());
    return kResultOk;
  }

  tresult PLUGIN_API getState (IBStream * state) SMTG_OVERRIDE
  {
    const nlohmann::json j{
      { "level", level_.load () }
    };
    const auto json_text = j.dump ();
    IBStreamer streamer (state, kLittleEndian);
    streamer.writeInt32 (static_cast<int32> (json_text.size ()));
    streamer.writeRaw (
      json_text.data (), static_cast<int32> (json_text.size ()));
    return kResultOk;
  }

  tresult PLUGIN_API process (ProcessData &data) SMTG_OVERRIDE
  {
    if (data.inputParameterChanges != nullptr)
      {
        const auto num_changes =
          data.inputParameterChanges->getParameterCount ();
        for (int32 i = 0; i < num_changes; ++i)
          {
            auto * queue = data.inputParameterChanges->getParameterData (i);
            if (queue == nullptr)
              continue;
            const auto num_points = queue->getPointCount ();
            if (num_points <= 0)
              continue;
            int32      offset = 0;
            ParamValue value = 0.0;
            if (queue->getPoint (num_points - 1, offset, value) != kResultOk)
              continue;
            if (queue->getParameterId () == kLevelParamId)
              {
                level_.store (value);
                // Counted and exposed through the controller state so
                // hosts can verify that applied plugin reports are not
                // echoed back as host-initiated changes
                dual_level_input_count.fetch_add (1);
              }
            else if (queue->getParameterId () == kAutoReportParamId)
              {
                auto_report_.store (value > 0.5);
              }
          }
      }

    // While Auto Report is on, report a fixed Level change from inside
    // processing, like a plugin morphing a parameter on the audio thread
    if (auto_report_.load () && data.outputParameterChanges != nullptr)
      {
        int32  queue_index = 0;
        auto * queue = data.outputParameterChanges->addParameterData (
          kLevelParamId, queue_index);
        if (queue != nullptr)
          {
            int32 point_index = 0;
            queue->addPoint (
              data.numSamples > 0 ? data.numSamples - 1 : 0, kReportedLevel,
              point_index);
          }
      }

    if (data.numSamples <= 0 || data.numInputs < 1 || data.numOutputs < 1)
      return kResultOk;

    const auto num_channels =
      std::min (data.inputs[0].numChannels, data.outputs[0].numChannels);
    for (int32 ch = 0; ch < num_channels; ++ch)
      {
        const auto * in = data.inputs[0].channelBuffers32[ch];
        auto *       out = data.outputs[0].channelBuffers32[ch];
        if ((data.inputs[0].silenceFlags & (uint64{ 1 } << ch)) != 0)
          {
            std::fill_n (out, data.numSamples, 0.0f);
            data.outputs[0].silenceFlags |= (uint64{ 1 } << ch);
            continue;
          }
        apply_gain (
          in, out, static_cast<uint32_t> (data.numSamples), level_.load ());
      }
    return kResultOk;
  }

  static FUnknown * createInstance (void *)
  {
    return static_cast<IAudioProcessor *> (new TestDualGain ());
  }

private:
  std::atomic<double> level_{ 1.0 };
  std::atomic<bool>   auto_report_{ false };
};

/** Edit controller side of the dual-component fixture. */
class TestDualGainController : public EditController, public IEditControllerHostEditing
{
public:
  static constexpr ParamID kLevelParamId = TestDualGain::kLevelParamId;
  static constexpr ParamID kAutoReportParamId = TestDualGain::kAutoReportParamId;

  DELEGATE_REFCOUNT (EditController)

  tresult PLUGIN_API initialize (FUnknown * context) SMTG_OVERRIDE
  {
    const auto res = EditController::initialize (context);
    if (res != kResultOk)
      return res;

    parameters.addParameter (
      STR16 ("Level"), STR16 (""), 0, 1.0, ParameterInfo::kCanAutomate,
      kLevelParamId);
    parameters.addParameter (
      STR16 ("Auto Report"), STR16 (""), 1, 0.0, ParameterInfo::kCanAutomate,
      kAutoReportParamId);
    return kResultOk;
  }

  tresult PLUGIN_API queryInterface (const TUID iid, void ** obj) SMTG_OVERRIDE
  {
    QUERY_INTERFACE (
      iid, obj, IEditControllerHostEditing::iid, IEditControllerHostEditing)
    return EditController::queryInterface (iid, obj);
  }

  tresult PLUGIN_API
  setParamNormalized (ParamID tag, ParamValue value) SMTG_OVERRIDE
  {
    const auto res = EditController::setParamNormalized (tag, value);
    if (res == kResultOk && tag == kLevelParamId)
      {
        // Counted and exposed through the state chunk so hosts can
        // verify that processor-reported values reach the controller
        dual_controller_sync_count.fetch_add (1);
        dual_last_synced_level.store (value);
      }
    return res;
  }

  tresult PLUGIN_API setComponentState (IBStream * state) SMTG_OVERRIDE
  {
    // The processor state holds the Level value; applying it through
    // the parameter object directly (not the counting
    // setParamNormalized override, which is reserved for host sync
    // notifications)
    IBStreamer streamer (state, kLittleEndian);
    int32      length = 0;
    if (streamer.readInt32 (length) && length > 0)
      {
        std::string json_text (static_cast<size_t> (length), '\0');
        if (streamer.readRaw (json_text.data (), length) == length)
          {
            const auto j = nlohmann::json::parse (json_text, nullptr, false);
            if (!j.is_discarded () && j.contains ("level"))
              {
                if (auto * param = parameters.getParameter (kLevelParamId))
                  param->setNormalized (j["level"].get<double> ());
              }
          }
      }
    return kResultOk;
  }

  tresult PLUGIN_API setState (IBStream * state) SMTG_OVERRIDE
  {
    // The controller state is host-observable counters only; nothing
    // to restore
    (void) state;
    return kResultOk;
  }

  tresult PLUGIN_API getState (IBStream * state) SMTG_OVERRIDE
  {
    auto *               level_param = parameters.getParameter (kLevelParamId);
    const nlohmann::json j{
      { "level",                     level_param != nullptr ? level_param->getNormalized () : 1.0 },
      { "controllerSyncCount",       dual_controller_sync_count.load ()                           },
      { "controllerLastSyncedLevel", dual_last_synced_level.load ()                               },
      { "hostEditWrapCount",         dual_host_edit_wrap_count.load ()                            },
      { "levelInputCount",           dual_level_input_count.load ()                               },
    };
    const auto json_text = j.dump ();
    IBStreamer streamer (state, kLittleEndian);
    streamer.writeInt32 (static_cast<int32> (json_text.size ()));
    streamer.writeRaw (
      json_text.data (), static_cast<int32> (json_text.size ()));
    return kResultOk;
  }

  tresult PLUGIN_API beginEditFromHost (ParamID id) SMTG_OVERRIDE
  {
    if (id == kLevelParamId)
      dual_host_edit_wrap_count.fetch_add (1);
    return kResultOk;
  }

  tresult PLUGIN_API endEditFromHost (ParamID id) SMTG_OVERRIDE
  {
    // beginEditFromHost already counted the wrapped edit
    (void) id;
    return kResultOk;
  }

  static FUnknown * createInstance (void *)
  {
    return static_cast<IEditController *> (new TestDualGainController ());
  }
};

} // namespace zrythm_test_plugins

using zrythm_test_plugins::TestDualGain;
using zrythm_test_plugins::TestDualGainController;
using zrythm_test_plugins::TestDualGainControllerUID;
using zrythm_test_plugins::TestDualGainUID;

BEGIN_FACTORY_DEF ("Zrythm", "https://zrythm.org", "mailto:contact@zrythm.org")
DEF_CLASS2 (
  INLINE_UID_FROM_FUID (TestDualGainUID),
  PClassInfo::kManyInstances,
  kVstAudioEffectClass,
  "Test Dual Gain",
  0,
  Vst::PlugType::kFx,
  "1.0.0",
  kVstVersionString,
  TestDualGain::createInstance)
DEF_CLASS2 (
  INLINE_UID_FROM_FUID (TestDualGainControllerUID),
  PClassInfo::kManyInstances,
  kVstComponentControllerClass,
  "Test Dual Gain Controller",
  0,
  "",
  "1.0.0",
  kVstVersionString,
  TestDualGainController::createInstance)
END_FACTORY
