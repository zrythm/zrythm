// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <array>
#include <atomic>
#include <cmath>
#include <string>

#include "base/source/fstreamer.h"
#include "gain_dsp.h"
#include "pluginterfaces/base/ustring.h"
#include "pluginterfaces/vst/ivstparameterchanges.h"
#include "pluginterfaces/vst/ivstunits.h"
#include "public.sdk/source/main/pluginfactory.h"
#include "public.sdk/source/vst/vstsinglecomponenteffect.h"
#include <nlohmann/json.hpp>

namespace zrythm_test_plugins
{

using namespace Steinberg;
using namespace Steinberg::Vst;

static const FUID
  TestProgramsUID (0x3F9A1C52, 0x7E4B4D1A, 0x9C2E6F83, 0x1B5D8A47);

/**
 * Gain plugin with a factory program list (IUnitInfo) and a program-change
 * parameter.
 *
 * Selecting a program adjusts the level. Toggle params let tests drive
 * plugin-side actions: switching the program itself (notifying the host
 * with kParamValuesChanged), simulating a UI gesture on the level
 * parameter (beginEdit) and renaming a program (notifyProgramListChange).
 */
class TestPrograms : public SingleComponentEffect
{
public:
  static constexpr ParamID       kLevelParamId = 0;
  static constexpr ParamID       kProgramParamId = 1;
  static constexpr ParamID       kTriggerSelfProgramChangeParamId = 2;
  static constexpr ParamID       kTriggerGestureParamId = 3;
  static constexpr ParamID       kTriggerRenameProgramsParamId = 4;
  static constexpr ParamID       kTriggerQuietProgramChangeParamId = 5;
  static constexpr ParamID       kSuppressProgramReflectionParamId = 6;
  static constexpr ProgramListID kProgramListId = 100;

  static constexpr int32                           kProgramCount = 3;
  static constexpr std::array<const char16_t *, 3> kProgramNames = {
    u"Init", u"Bright", u"Warm"
  };
  static constexpr std::array<double, 3> kProgramLevels = { 0.25, 0.75, 1.0 };
  tresult PLUGIN_API initialize (FUnknown * context) SMTG_OVERRIDE
  {
    const auto res = SingleComponentEffect::initialize (context);
    if (res != kResultOk)
      return res;

    addAudioInput (STR16 ("Input"), SpeakerArr::kStereo);
    addAudioOutput (STR16 ("Output"), SpeakerArr::kStereo);
    addEventInput (STR16 ("Event Input"), 1);
    parameters.addParameter (
      STR16 ("Level"), STR16 (""), 0, kProgramLevels[0],
      ParameterInfo::kCanAutomate, kLevelParamId);
    parameters.addParameter (
      STR16 ("Program"), STR16 (""), kProgramCount - 1, 0.0,
      ParameterInfo::kIsProgramChange, kProgramParamId);
    parameters.addParameter (
      STR16 ("Trigger Self Program Change"), STR16 (""), 1, 0.0,
      ParameterInfo::kCanAutomate, kTriggerSelfProgramChangeParamId);
    parameters.addParameter (
      STR16 ("Trigger Gesture"), STR16 (""), 1, 0.0,
      ParameterInfo::kCanAutomate, kTriggerGestureParamId);
    parameters.addParameter (
      STR16 ("Trigger Rename Programs"), STR16 (""), 1, 0.0,
      ParameterInfo::kCanAutomate, kTriggerRenameProgramsParamId);
    parameters.addParameter (
      STR16 ("Trigger Quiet Program Change"), STR16 (""), 1, 0.0,
      ParameterInfo::kCanAutomate, kTriggerQuietProgramChangeParamId);
    parameters.addParameter (
      STR16 ("Suppress Program Change Reflection"), STR16 (""), 1, 0.0,
      ParameterInfo::kCanAutomate, kSuppressProgramReflectionParamId);
    return kResultOk;
  }

  tresult PLUGIN_API
  setComponentHandler (IComponentHandler * handler) SMTG_OVERRIDE
  {
    // The unit handler is used by the rename trigger to report program
    // list content changes
    unit_handler_ = nullptr;
    if (handler != nullptr)
      {
        IUnitHandler * unit_handler = nullptr;
        if (
          handler->queryInterface (
            IUnitHandler::iid, reinterpret_cast<void **> (&unit_handler))
          == kResultOk)
          {
            // queryInterface already added a reference; adopt it
            unit_handler_ = IPtr<IUnitHandler> (unit_handler, false);
          }
      }
    return EditControllerEx1::setComponentHandler (handler);
  }

  //--- IUnitInfo -------------
  int32 PLUGIN_API getUnitCount () SMTG_OVERRIDE { return 1; }

  tresult PLUGIN_API getUnitInfo (int32 unitIndex, UnitInfo &info) SMTG_OVERRIDE
  {
    if (unitIndex != 0)
      return kResultFalse;
    info.id = kRootUnitId;
    info.parentUnitId = kNoParentUnitId;
    Steinberg::UString (info.name, 128).assign (u"Root");
    info.programListId = kProgramListId;
    return kResultTrue;
  }

  int32 PLUGIN_API getProgramListCount () SMTG_OVERRIDE { return 1; }

  tresult PLUGIN_API
  getProgramListInfo (int32 listIndex, ProgramListInfo &info) SMTG_OVERRIDE
  {
    if (listIndex != 0)
      return kResultFalse;
    info.id = kProgramListId;
    Steinberg::UString (info.name, 128).assign (u"Factory");
    info.programCount = kProgramCount;
    return kResultTrue;
  }

  tresult PLUGIN_API
  getProgramName (ProgramListID listId, int32 programIndex, String128 name)
    SMTG_OVERRIDE
  {
    if (
      listId != kProgramListId || programIndex < 0
      || programIndex >= kProgramCount)
      return kResultFalse;
    Steinberg::UString (name, 128).assign (
      program_names_[programIndex].c_str ());
    return kResultTrue;
  }

  tresult PLUGIN_API
  getProgramInfo (ProgramListID, int32, Vst::CString, String128) SMTG_OVERRIDE
  {
    return kResultFalse;
  }

  tresult PLUGIN_API hasProgramPitchNames (ProgramListID, int32) SMTG_OVERRIDE
  {
    return kResultFalse;
  }

  tresult PLUGIN_API
  getProgramPitchName (ProgramListID, int32, int16, String128) SMTG_OVERRIDE
  {
    return kResultFalse;
  }

  UnitID PLUGIN_API getSelectedUnit () SMTG_OVERRIDE { return kRootUnitId; }

  tresult PLUGIN_API selectUnit (UnitID unitId) SMTG_OVERRIDE
  {
    return unitId == kRootUnitId ? kResultTrue : kResultFalse;
  }

  tresult PLUGIN_API setUnitProgramData (int32, int32, IBStream *) SMTG_OVERRIDE
  {
    return kResultFalse;
  }

  tresult PLUGIN_API
  getUnitByBus (MediaType, BusDirection, int32, int32, UnitID & /*unitId*/)
    SMTG_OVERRIDE
  {
    return kResultFalse;
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
    if (res != kResultOk)
      return res;

    if (tag == kProgramParamId)
      {
        apply_program (
          static_cast<int32> (std::lround (value * (kProgramCount - 1))));
      }
    else if (tag == kSuppressProgramReflectionParamId)
      {
        suppress_program_reflection_.store (value > 0.5);
      }
    else if (value > 0.5 && componentHandler != nullptr)
      {
        if (tag == kTriggerSelfProgramChangeParamId && !program_change_fired_)
          {
            program_change_fired_ = true;
            // Simulate the plugin switching to the last program by itself
            // (e.g. from its own preset browser)
            EditControllerEx1::setParamNormalized (kProgramParamId, 1.0);
            apply_program (kProgramCount - 1);
            componentHandler->restartComponent (
              RestartFlags::kParamValuesChanged);
            // Reset the toggle so that restored state does not re-fire
            EditControllerEx1::setParamNormalized (tag, 0.0);
          }
        else if (tag == kTriggerGestureParamId && !gesture_fired_)
          {
            gesture_fired_ = true;
            // Simulate the user starting to adjust a parameter in the
            // plugin's own UI
            componentHandler->beginEdit (kLevelParamId);
            EditControllerEx1::setParamNormalized (tag, 0.0);
          }
        else if (
          tag == kTriggerRenameProgramsParamId && !rename_fired_
          && unit_handler_ != nullptr)
          {
            rename_fired_ = true;
            // Simulate the plugin renaming a program (e.g. user saved a
            // user program over a factory slot)
            program_names_[0] = u"Init (User)";
            unit_handler_->notifyProgramListChange (kProgramListId, 0);
            // Reset the toggle so that restored state does not re-fire
            EditControllerEx1::setParamNormalized (tag, 0.0);
          }
        else if (
          tag == kTriggerQuietProgramChangeParamId
          && !quiet_program_change_fired_)
          {
            quiet_program_change_fired_ = true;
            // Simulate the plugin switching to the last program from its
            // own UI, notifying only via a begin/perform/end gesture on
            // the program-change parameter (no restartComponent)
            componentHandler->beginEdit (kProgramParamId);
            EditControllerEx1::setParamNormalized (kProgramParamId, 1.0);
            apply_program (kProgramCount - 1);
            componentHandler->performEdit (kProgramParamId, 1.0);
            componentHandler->endEdit (kProgramParamId);
            // Reset the toggle so that restored state does not re-fire
            EditControllerEx1::setParamNormalized (tag, 0.0);
          }
      }
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
            const auto param_id = queue->getParameterId ();
            if (
              param_id != kLevelParamId && param_id != kProgramParamId
              && param_id != kSuppressProgramReflectionParamId)
              continue;
            const auto num_points = queue->getPointCount ();
            if (num_points > 0)
              {
                int32      offset = 0;
                ParamValue value = 0.0;
                if (queue->getPoint (num_points - 1, offset, value) == kResultOk)
                  {
                    if (param_id == kLevelParamId)
                      {
                        level_.store (value);
                      }
                    else if (param_id == kSuppressProgramReflectionParamId)
                      {
                        suppress_program_reflection_.store (value > 0.5);
                      }
                    else
                      {
                        apply_program (
                          static_cast<int32_t> (
                            std::lround (value * (kProgramCount - 1))));
                        // Reflect the program change on the controller side
                        // and notify the host, as plugins do for program
                        // changes arriving on realtime paths (e.g. MIDI).
                        // A plugin has no obligation to do this for
                        // host-initiated changes, so tests can suppress it
                        if (!suppress_program_reflection_.load ())
                          {
                            EditControllerEx1::setParamNormalized (
                              kProgramParamId, value);
                            if (componentHandler != nullptr)
                              {
                                componentHandler->restartComponent (
                                  RestartFlags::kParamValuesChanged);
                              }
                          }
                      }
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
          in, out, static_cast<uint32_t> (data.numSamples), level_.load ());
      }
    return kResultOk;
  }

  tresult PLUGIN_API setEditorState (IBStream * state) SMTG_OVERRIDE
  {
    IBStreamer streamer (state, kLittleEndian);
    int32      length = 0;
    if (!streamer.readInt32 (length))
      return kResultFalse;
    if (length <= 0)
      return kResultFalse;
    std::string json_text (static_cast<size_t> (length), '\0');
    if (streamer.readRaw (json_text.data (), length) != length)
      return kResultFalse;
    const auto j = nlohmann::json::parse (json_text, nullptr, false);
    if (
      j.is_discarded () || !j.contains ("level") || !j.contains ("program")
      || !j["level"].is_number () || !j["program"].is_number ())
      return kResultFalse;
    // Restore the raw values through the base class: the setParamNormalized
    // override applies the program, which would clobber the restored level
    EditControllerEx1::setParamNormalized (
      kProgramParamId, j["program"].get<double> ());
    const auto level = j["level"].get<double> ();
    EditControllerEx1::setParamNormalized (kLevelParamId, level);
    level_.store (level);
    return kResultOk;
  }

  tresult PLUGIN_API getEditorState (IBStream * state) SMTG_OVERRIDE
  {
    const nlohmann::json j{
      { "level",   level_.load ()                       },
      { "program", getParamNormalized (kProgramParamId) },
    };
    const auto json_text = j.dump ();
    IBStreamer streamer (state, kLittleEndian);
    if (!streamer.writeInt32 (static_cast<int32> (json_text.size ())))
      return kResultFalse;
    if (
      streamer.writeRaw (
        json_text.data (), static_cast<int32> (json_text.size ()))
      != static_cast<int32> (json_text.size ()))
      return kResultFalse;
    return kResultOk;
  }

  static FUnknown * createInstance (void *)
  {
    return static_cast<IAudioProcessor *> (new TestPrograms ());
  }

private:
  void apply_program (int32 program_index)
  {
    // The index is derived from host-supplied parameter values
    if (program_index < 0 || program_index >= kProgramCount)
      return;
    level_.store (kProgramLevels[program_index]);
    EditControllerEx1::setParamNormalized (
      kLevelParamId, kProgramLevels[program_index]);
  }

  std::atomic<double> level_{ kProgramLevels[0] };
  IPtr<IUnitHandler>  unit_handler_;
  bool                program_change_fired_ = false;
  bool                gesture_fired_ = false;
  bool                rename_fired_ = false;
  bool                quiet_program_change_fired_ = false;
  std::atomic<bool>   suppress_program_reflection_{ false };

  std::array<std::u16string, 3> program_names_ = {
    kProgramNames[0], kProgramNames[1], kProgramNames[2]
  };
};

} // namespace zrythm_test_plugins

using zrythm_test_plugins::TestPrograms;
using zrythm_test_plugins::TestProgramsUID;

BEGIN_FACTORY_DEF ("Zrythm", "https://zrythm.org", "mailto:contact@zrythm.org")
DEF_CLASS2 (
  INLINE_UID_FROM_FUID (TestProgramsUID),
  PClassInfo::kManyInstances,
  kVstAudioEffectClass,
  "Test Programs",
  0,
  Vst::PlugType::kFx,
  "1.0.0",
  kVstVersionString,
  TestPrograms::createInstance)
END_FACTORY
