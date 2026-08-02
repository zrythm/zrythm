// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <atomic>
#include <cstring>

#include "gain_dsp.h"
#include "pluginterfaces/base/funknownimpl.h"
#include "pluginterfaces/gui/iplugview.h"
#include "pluginterfaces/vst/ivstparameterchanges.h"
#include "public.sdk/source/main/pluginfactory.h"
#include "public.sdk/source/vst/vstsinglecomponenteffect.h"

namespace zrythm_test_plugins
{

using namespace Steinberg;
using namespace Steinberg::Vst;

static const FUID TestGuiUID (0x3C4D5E6F, 0x7A8B9C0D, 0x1E2F3A4B, 0x5C6D7E8F);

/**
 * Stub editor view with a fixed initial size that supports all platform
 * types and requests a resize when attached (exercising the host's
 * IPlugFrame::resizeView path, as permitted by the IPlugView docs).
 */
class TestGuiView : public U::Implements<U::Directly<IPlugView>>
{
public:
  static constexpr int32 kInitialWidth = 320;
  static constexpr int32 kInitialHeight = 240;
  static constexpr int32 kResizedWidth = 640;
  static constexpr int32 kResizedHeight = 480;

  TestGuiView () : size_ (0, 0, kInitialWidth, kInitialHeight) { }

  tresult PLUGIN_API isPlatformTypeSupported (FIDString type) SMTG_OVERRIDE
  {
    if (
      std::strcmp (type, kPlatformTypeX11EmbedWindowID) == 0
      || std::strcmp (type, kPlatformTypeNSView) == 0
      || std::strcmp (type, kPlatformTypeHWND) == 0)
      return kResultTrue;
    return kResultFalse;
  }

  tresult PLUGIN_API attached (void * parent, FIDString type) SMTG_OVERRIDE
  {
    if (frame_ != nullptr)
      {
        ViewRect new_size (0, 0, kResizedWidth, kResizedHeight);
        frame_->resizeView (this, &new_size);
      }
    return kResultOk;
  }

  tresult PLUGIN_API removed () SMTG_OVERRIDE { return kResultOk; }
  tresult PLUGIN_API onWheel (float distance) SMTG_OVERRIDE
  {
    return kResultFalse;
  }
  tresult PLUGIN_API
  onKeyDown (char16 key, int16 keyCode, int16 modifiers) SMTG_OVERRIDE
  {
    return kResultFalse;
  }
  tresult PLUGIN_API
  onKeyUp (char16 key, int16 keyCode, int16 modifiers) SMTG_OVERRIDE
  {
    return kResultFalse;
  }

  tresult PLUGIN_API getSize (ViewRect * size) SMTG_OVERRIDE
  {
    *size = size_;
    return kResultOk;
  }

  tresult PLUGIN_API onSize (ViewRect * newSize) SMTG_OVERRIDE
  {
    size_ = *newSize;
    return kResultOk;
  }

  tresult PLUGIN_API onFocus (TBool state) SMTG_OVERRIDE { return kResultOk; }

  tresult PLUGIN_API setFrame (IPlugFrame * frame) SMTG_OVERRIDE
  {
    frame_ = frame;
    return kResultOk;
  }

  tresult PLUGIN_API canResize () SMTG_OVERRIDE { return kResultFalse; }

  tresult PLUGIN_API checkSizeConstraint (ViewRect * rect) SMTG_OVERRIDE
  {
    return kResultOk;
  }

private:
  IPlugFrame * frame_ = nullptr;
  ViewRect     size_;
};

/**
 * Gain plugin with an editor view (TestGuiView).
 */
class TestGui : public SingleComponentEffect
{
public:
  static constexpr ParamID kLevelParamId = 0;

  tresult PLUGIN_API initialize (FUnknown * context) SMTG_OVERRIDE
  {
    const auto res = SingleComponentEffect::initialize (context);
    if (res != kResultOk)
      return res;

    addAudioInput (STR16 ("Input"), SpeakerArr::kStereo);
    addAudioOutput (STR16 ("Output"), SpeakerArr::kStereo);
    parameters.addParameter (
      STR16 ("Level"), STR16 (""), 0, 1.0, ParameterInfo::kCanAutomate,
      kLevelParamId);
    return kResultOk;
  }

  IPlugView * PLUGIN_API createView (FIDString name) SMTG_OVERRIDE
  {
    if (std::strcmp (name, ViewType::kEditor) == 0)
      return new TestGuiView ();
    return nullptr;
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
            if (queue == nullptr || queue->getParameterId () != kLevelParamId)
              continue;
            const auto num_points = queue->getPointCount ();
            if (num_points > 0)
              {
                int32      offset = 0;
                ParamValue value = 0.0;
                if (queue->getPoint (num_points - 1, offset, value) == kResultOk)
                  level_.store (value);
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
          in, out, static_cast<uint32_t> (data.numSamples), level_.load ());
      }
    return kResultOk;
  }

  static FUnknown * createInstance (void *)
  {
    return static_cast<IAudioProcessor *> (new TestGui ());
  }

private:
  std::atomic<double> level_{ 1.0 };
};

} // namespace zrythm_test_plugins

using zrythm_test_plugins::TestGui;
using zrythm_test_plugins::TestGuiUID;

BEGIN_FACTORY_DEF ("Zrythm", "https://zrythm.org", "mailto:contact@zrythm.org")
DEF_CLASS2 (
  INLINE_UID_FROM_FUID (TestGuiUID),
  PClassInfo::kManyInstances,
  kVstAudioEffectClass,
  "Test GUI",
  0,
  Vst::PlugType::kFx,
  "1.0.0",
  kVstVersionString,
  TestGui::createInstance)
END_FACTORY
