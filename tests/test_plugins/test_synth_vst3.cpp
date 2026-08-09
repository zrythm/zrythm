// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <atomic>

#include "base/source/fstreamer.h"
#include "pluginterfaces/vst/ivstevents.h"
#include "pluginterfaces/vst/ivstparameterchanges.h"
#include "public.sdk/source/main/pluginfactory.h"
#include "public.sdk/source/vst/vstsinglecomponenteffect.h"
#include "sine_synth.h"

namespace zrythm_test_plugins
{

using namespace Steinberg;
using namespace Steinberg::Vst;

static const FUID TestSynthUID (0x1B2C3D4E, 0x5F6A7B8C, 0x9D0E1F2A, 0x3B4C5D6E);

class TestSynth : public SingleComponentEffect
{
public:
  static constexpr ParamID kLevelParamId = 0;

  tresult PLUGIN_API initialize (FUnknown * context) SMTG_OVERRIDE
  {
    const auto res = SingleComponentEffect::initialize (context);
    if (res != kResultOk)
      return res;

    addEventInput (STR16 ("Event Input"), 1);
    addEventOutput (STR16 ("Event Output"), 1);
    addAudioOutput (STR16 ("Output"), SpeakerArr::kStereo);
    parameters.addParameter (
      STR16 ("Level"), STR16 (""), 0, 1.0, ParameterInfo::kCanAutomate,
      kLevelParamId);
    return kResultOk;
  }

  tresult PLUGIN_API setBusArrangements (
    SpeakerArrangement * inputs,
    int32                numIns,
    SpeakerArrangement * outputs,
    int32                numOuts) SMTG_OVERRIDE
  {
    if (numIns == 0 && numOuts == 1 && outputs[0] == SpeakerArr::kStereo)
      return kResultOk;
    return kResultFalse;
  }

  tresult PLUGIN_API
  canProcessSampleSize (int32 symbolicSampleSize) SMTG_OVERRIDE
  {
    return symbolicSampleSize == kSample32 ? kResultTrue : kResultFalse;
  }

  tresult PLUGIN_API setupProcessing (ProcessSetup &setup) SMTG_OVERRIDE
  {
    synth_.set_sample_rate (setup.sampleRate);
    return SingleComponentEffect::setupProcessing (setup);
  }

  tresult PLUGIN_API
  setParamNormalized (ParamID tag, ParamValue value) SMTG_OVERRIDE
  {
    const auto res = EditControllerEx1::setParamNormalized (tag, value);
    if (res == kResultOk && tag == kLevelParamId)
      gain_.store (value);
    return res;
  }

  tresult PLUGIN_API setEditorState (IBStream * state) SMTG_OVERRIDE
  {
    IBStreamer streamer (state, kLittleEndian);
    double     saved_gain = 0.0;
    if (!streamer.readDouble (saved_gain))
      return kResultFalse;
    setParamNormalized (kLevelParamId, saved_gain);
    return kResultOk;
  }

  tresult PLUGIN_API getEditorState (IBStream * state) SMTG_OVERRIDE
  {
    IBStreamer streamer (state, kLittleEndian);
    streamer.writeDouble (gain_.load ());
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
            if (queue == nullptr || queue->getParameterId () != kLevelParamId)
              continue;
            const auto num_points = queue->getPointCount ();
            if (num_points > 0)
              {
                int32      offset = 0;
                ParamValue value = 0.0;
                if (queue->getPoint (num_points - 1, offset, value) == kResultOk)
                  gain_.store (value);
              }
          }
      }

    if (data.inputEvents != nullptr)
      {
        Event      event{};
        const auto num_events = data.inputEvents->getEventCount ();
        for (int32 i = 0; i < num_events; ++i)
          {
            if (data.inputEvents->getEvent (i, event) != kResultOk)
              continue;
            if (event.type == Event::kNoteOnEvent)
              {
                note_off_level_.store (0.0);
                synth_.note_on (event.noteOn.pitch, event.noteOn.velocity);
              }
            else if (event.type == Event::kNoteOffEvent)
              {
                synth_.note_off (event.noteOff.pitch);
                // Expose the note-off velocity as a DC offset so hosts can
                // verify it is delivered verbatim
                note_off_level_.store (event.noteOff.velocity);
              }
            // Echo note events so hosts can verify note-output forwarding
            // and event-time handling
            if (
              data.outputEvents != nullptr
              && (event.type == Event::kNoteOnEvent || event.type == Event::kNoteOffEvent))
              {
                Event echoed = event;
                echoed.busIndex = 0;
                data.outputEvents->addEvent (echoed);
              }
          }
      }

    if (data.numSamples <= 0 || data.numOutputs < 1)
      return kResultOk;

    auto &output = data.outputs[0];
    if (output.numChannels < 2)
      return kResultOk;

    auto * left = output.channelBuffers32[0];
    auto * right = output.channelBuffers32[1];
    std::fill_n (left, data.numSamples, 0.0f);
    std::fill_n (right, data.numSamples, 0.0f);
    synth_.process (
      left, right, static_cast<uint32_t> (data.numSamples), gain_.load ());
    const auto dc = static_cast<float> (note_off_level_.load ());
    if (dc != 0.0f)
      {
        for (int32 i = 0; i < data.numSamples; ++i)
          {
            left[i] += dc;
            right[i] += dc;
          }
      }
    return kResultOk;
  }

  static FUnknown * createInstance (void *)
  {
    return static_cast<IAudioProcessor *> (new TestSynth ());
  }

private:
  SineSynth           synth_;
  std::atomic<double> gain_{ 1.0 };
  std::atomic<double> note_off_level_{ 0.0 };
};

} // namespace zrythm_test_plugins

using zrythm_test_plugins::TestSynth;
using zrythm_test_plugins::TestSynthUID;

BEGIN_FACTORY_DEF ("Zrythm", "https://zrythm.org", "mailto:contact@zrythm.org")
DEF_CLASS2 (
  INLINE_UID_FROM_FUID (TestSynthUID),
  PClassInfo::kManyInstances,
  kVstAudioEffectClass,
  "Test Synth",
  0,
  Vst::PlugType::kInstrumentSynth,
  "1.0.0",
  kVstVersionString,
  TestSynth::createInstance)
END_FACTORY
