// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <algorithm>

#include "public.sdk/source/main/pluginfactory.h"
#include "public.sdk/source/vst/vstsinglecomponenteffect.h"

namespace zrythm_test_plugins
{

using namespace Steinberg;
using namespace Steinberg::Vst;

static const FUID
  TestLatencyUID (0x7B3E9A41, 0x2F6D4C8E, 0xB5A1D3F7, 0x9E2C6B48);

class TestLatency : public SingleComponentEffect
{
public:
  static constexpr uint32 kLatencySamples = 256;

  tresult PLUGIN_API initialize (FUnknown * context) SMTG_OVERRIDE
  {
    const auto res = SingleComponentEffect::initialize (context);
    if (res != kResultOk)
      return res;

    addAudioInput (STR16 ("Input"), SpeakerArr::kStereo);
    addAudioOutput (STR16 ("Output"), SpeakerArr::kStereo);
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

  uint32 PLUGIN_API getLatencySamples () SMTG_OVERRIDE
  {
    return kLatencySamples;
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
    return static_cast<IAudioProcessor *> (new TestLatency ());
  }
};

} // namespace zrythm_test_plugins

using zrythm_test_plugins::TestLatency;
using zrythm_test_plugins::TestLatencyUID;

BEGIN_FACTORY_DEF ("Zrythm", "https://zrythm.org", "mailto:contact@zrythm.org")
DEF_CLASS2 (
  INLINE_UID_FROM_FUID (TestLatencyUID),
  PClassInfo::kManyInstances,
  kVstAudioEffectClass,
  "Test Latency",
  0,
  Vst::PlugType::kFx,
  "1.0.0",
  kVstVersionString,
  TestLatency::createInstance)
END_FACTORY
