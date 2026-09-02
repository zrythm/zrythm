// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * @file Fixture exposing parameters in a nested IUnitInfo unit hierarchy.
 */

#include "pluginterfaces/vst/ivstunits.h"
#include "public.sdk/source/main/pluginfactory.h"
#include "public.sdk/source/vst/vstsinglecomponenteffect.h"

namespace zrythm_test_plugins
{

using namespace Steinberg;
using namespace Steinberg::Vst;

static const FUID
  TestParamGroupsUID (0x2C4D6E8F, 0x1A3B5C7D, 0x9E0F2A4B, 0x6D8E0C1A);

/**
 * Gain-less effect with a nested unit hierarchy (root -> "Osc" -> "Filter")
 * and one parameter per level: ungrouped (root), "Osc" and "Filter".
 */
class TestParamGroups : public SingleComponentEffect
{
public:
  static constexpr ParamID kMasterParamId = 0;
  static constexpr ParamID kResonanceParamId = 1;
  static constexpr ParamID kCutoffParamId = 2;

  static constexpr UnitID kOscUnitId = 1;
  static constexpr UnitID kFilterUnitId = 2;

  DELEGATE_REFCOUNT (SingleComponentEffect)

  tresult PLUGIN_API initialize (FUnknown * context) SMTG_OVERRIDE
  {
    const auto res = SingleComponentEffect::initialize (context);
    if (res != kResultOk)
      return res;

    addAudioInput (STR16 ("Input"), SpeakerArr::kStereo);
    addAudioOutput (STR16 ("Output"), SpeakerArr::kStereo);

    addUnit (new Unit (STR16 ("Root"), kRootUnitId, kNoParentUnitId));
    addUnit (new Unit (STR16 ("Osc"), kOscUnitId));
    addUnit (new Unit (STR16 ("Filter"), kFilterUnitId, kOscUnitId));

    // Ungrouped (root unit)
    parameters.addParameter (
      STR16 ("Master"), STR16 (""), 0, 1.0, ParameterInfo::kCanAutomate,
      kMasterParamId);
    // In the "Osc" unit
    parameters.addParameter (
      STR16 ("Resonance"), STR16 (""), 0, 0.5, ParameterInfo::kCanAutomate,
      kResonanceParamId, kOscUnitId);
    // In the "Filter" unit, nested under "Osc"
    parameters.addParameter (
      STR16 ("Cutoff"), STR16 (""), 0, 0.5, ParameterInfo::kCanAutomate,
      kCutoffParamId, kFilterUnitId);
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
          data.outputs[0].channelBuffers32[ch], data.numSamples, 0.0f);
        data.outputs[0].silenceFlags |= (uint64{ 1 } << ch);
      }
    return kResultOk;
  }

  static FUnknown * createInstance (void *)
  {
    return static_cast<IAudioProcessor *> (new TestParamGroups ());
  }
};

} // namespace zrythm_test_plugins

using zrythm_test_plugins::TestParamGroups;
using zrythm_test_plugins::TestParamGroupsUID;

BEGIN_FACTORY_DEF ("Zrythm", "https://zrythm.org", "mailto:contact@zrythm.org")
DEF_CLASS2 (
  INLINE_UID_FROM_FUID (TestParamGroupsUID),
  PClassInfo::kManyInstances,
  kVstAudioEffectClass,
  "Test Param Groups",
  0,
  Vst::PlugType::kFx,
  "1.0.0",
  kVstVersionString,
  TestParamGroups::createInstance)
END_FACTORY
