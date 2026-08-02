// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <atomic>

#include "gain_dsp.h"
#include "pluginterfaces/vst/ivstmidicontrollers.h"
#include "pluginterfaces/vst/ivstparameterchanges.h"
#include "public.sdk/source/main/pluginfactory.h"
#include "public.sdk/source/vst/vstsinglecomponenteffect.h"

namespace zrythm_test_plugins
{

using namespace Steinberg;
using namespace Steinberg::Vst;

static const FUID TestMidiCcUID (0x7B2E4F01, 0x3D5A6C8E, 0x8F0A1B2C, 0x4D5E6F7A);

/**
 * Gain plugin that exposes a CC-mappable parameter via IMidiMapping.
 *
 * Hosts are expected to translate MIDI CC messages into parameter changes
 * using the IMidiMapping table instead of exposing CC-mapped parameters as
 * regular parameters.
 */
class TestMidiCc : public SingleComponentEffect, public IMidiMapping
{
public:
  static constexpr ParamID    kLevelParamId = 0;
  static constexpr ParamID    kCcLevelParamId = 1;
  static constexpr CtrlNumber kMappedCcNumber = 20;

  tresult PLUGIN_API initialize (FUnknown * context) SMTG_OVERRIDE
  {
    const auto res = SingleComponentEffect::initialize (context);
    if (res != kResultOk)
      return res;

    addAudioInput (STR16 ("Input"), SpeakerArr::kStereo);
    addAudioOutput (STR16 ("Output"), SpeakerArr::kStereo);
    addEventInput (STR16 ("Event Input"), 1);
    parameters.addParameter (
      STR16 ("Level"), STR16 (""), 0, 1.0, ParameterInfo::kCanAutomate,
      kLevelParamId);
    parameters.addParameter (
      STR16 ("CC Level"), STR16 (""), 0, 1.0, ParameterInfo::kCanAutomate,
      kCcLevelParamId);
    return kResultOk;
  }

  tresult PLUGIN_API getMidiControllerAssignment (
    int32      busIndex,
    int16      channel,
    CtrlNumber midiControllerNumber,
    ParamID   &id) SMTG_OVERRIDE
  {
    if (busIndex == 0 && midiControllerNumber == kMappedCcNumber)
      {
        id = kCcLevelParamId;
        return kResultOk;
      }
    return kResultFalse;
  }

  tresult PLUGIN_API queryInterface (const TUID iid, void ** obj) SMTG_OVERRIDE
  {
    QUERY_INTERFACE (iid, obj, IMidiMapping::iid, IMidiMapping)
    return SingleComponentEffect::queryInterface (iid, obj);
  }

  uint32 PLUGIN_API addRef () SMTG_OVERRIDE
  {
    return SingleComponentEffect::addRef ();
  }
  uint32 PLUGIN_API release () SMTG_OVERRIDE
  {
    return SingleComponentEffect::release ();
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

  tresult PLUGIN_API
  setParamNormalized (ParamID tag, ParamValue value) SMTG_OVERRIDE
  {
    const auto res = EditControllerEx1::setParamNormalized (tag, value);
    if (res == kResultOk && tag == kLevelParamId)
      level_.store (value);
    if (res == kResultOk && tag == kCcLevelParamId)
      cc_level_.store (value);
    return res;
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
            const auto param_id = queue->getParameterId ();
            if (param_id != kLevelParamId && param_id != kCcLevelParamId)
              continue;
            const auto num_points = queue->getPointCount ();
            if (num_points > 0)
              {
                int32      offset = 0;
                ParamValue value = 0.0;
                if (queue->getPoint (num_points - 1, offset, value) == kResultOk)
                  {
                    if (param_id == kLevelParamId)
                      level_.store (value);
                    else
                      cc_level_.store (value);
                  }
              }
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
          in, out, static_cast<uint32_t> (data.numSamples),
          level_.load () * cc_level_.load ());
      }
    return kResultOk;
  }

  static FUnknown * createInstance (void *)
  {
    return static_cast<IAudioProcessor *> (new TestMidiCc ());
  }

private:
  std::atomic<double> level_{ 1.0 };
  std::atomic<double> cc_level_{ 1.0 };
};

} // namespace zrythm_test_plugins

using zrythm_test_plugins::TestMidiCc;
using zrythm_test_plugins::TestMidiCcUID;

BEGIN_FACTORY_DEF ("Zrythm", "https://zrythm.org", "mailto:contact@zrythm.org")
DEF_CLASS2 (
  INLINE_UID_FROM_FUID (TestMidiCcUID),
  PClassInfo::kManyInstances,
  kVstAudioEffectClass,
  "Test MIDI CC",
  0,
  Vst::PlugType::kFx,
  "1.0.0",
  kVstVersionString,
  TestMidiCc::createInstance)
END_FACTORY
