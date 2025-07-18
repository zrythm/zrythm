// SPDX-FileCopyrightText: © 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "dsp/parameter.h"
#include "structure/tracks/processable_track.h"

#define DEFINE_RECORDABLE_TRACK_QML_PROPERTIES(ClassType) \
public: \
  /* ================================================================ */ \
  /* recording */ \
  /* ================================================================ */ \
  Q_PROPERTY ( \
    bool recording READ getRecording WRITE setRecording NOTIFY recordingChanged) \
  bool getRecording () const \
  { \
    return get_recording (); \
  } \
  void setRecording (bool recording) \
  { \
    set_recording (recording); \
  } \
\
  Q_SIGNAL void recordingChanged (bool recording);

namespace zrythm::structure::tracks
{
/**
 * Abstract class for a track that can be recorded.
 */
class RecordableTrack : virtual public ProcessableTrack
{
protected:
  RecordableTrack (ProcessableTrack::Dependencies dependencies);

public:
  Z_DISABLE_COPY_MOVE (RecordableTrack)

  ~RecordableTrack () override = default;

  void init_loaded (
    PluginRegistry                  &plugin_registry,
    dsp::PortRegistry               &port_registry,
    dsp::ProcessorParameterRegistry &param_registry) override;

  [[gnu::hot]] bool get_recording () const
  {
    const auto &recording_param = get_recording_param ();
    return recording_param.range ().is_toggled (recording_param.currentValue ());
  }

  /**
   * Sets recording and connects/disconnects the JACK ports.
   */
  void set_recording (this auto &&self, bool recording)
  {
    if (self.get_recording () == recording)
      return;

    z_debug ("{}: setting recording {}", self.name_, recording);
    self.get_recording_param ().setBaseValue (recording ? 1.f : 0.f);

    if (recording)
      {
        z_info ("enabled recording on {}", self.name_);
      }
    else
      {
        z_info ("disabled recording on {}", self.name_);

        /* send all notes off if can record MIDI */
        self.processor_->pending_midi_panic_ = true;
      }

    Q_EMIT self.recordingChanged (recording);
  }

  std::optional<ArrangerObjectPtrVariant> get_recording_region () const
  {
    if (!recording_region_)
      return std::nullopt;
    return object_registry_.find_by_id_or_throw (recording_region_.value ());
  }

  /**
   * @brief Initializes a recordable track.
   */
  template <typename DerivedT>
  void init_recordable_track (
    this DerivedT    &self,
    GenericBoolGetter autoarm_enabled_checker)
  {
    // self.get_recording_param ().set_full_designation_provider (&self);
    self.connect (
      &self, &DerivedT::selectedChanged, &self, [&self, autoarm_enabled_checker] {
        if (autoarm_enabled_checker ())
          {
            const auto selected = self.getSelected ();
            self.set_recording (selected);
            self.record_set_automatically_ = selected;
          }
      });
  }

protected:
  friend void init_from (
    RecordableTrack       &obj,
    const RecordableTrack &other,
    utils::ObjectCloneType clone_type)
  {
    obj.recording_id_ = other.recording_id_;
    obj.record_set_automatically_ = other.record_set_automatically_;
  }

  dsp::ProcessorParameter &get_recording_param () const
  {
    return *recording_id_.get_object_as<dsp::ProcessorParameter> ();
  }

private:
  static constexpr auto kRecordingIdKey = "recordingId"sv;
  static constexpr auto kRecordSetAutomaticallyKey = "recordSetAutomatically"sv;
  friend void to_json (nlohmann::json &j, const RecordableTrack &track)
  {
    j[kRecordingIdKey] = track.recording_id_;
    j[kRecordSetAutomaticallyKey] = track.record_set_automatically_;
  }
  friend void from_json (const nlohmann::json &j, RecordableTrack &track)
  {
    track.recording_id_ = { track.param_registry_ };
    j.at (kRecordingIdKey).get_to (track.recording_id_);
    j.at (kRecordSetAutomaticallyKey).get_to (track.record_set_automatically_);
  }

public:
  /** Recording or not. */
  dsp::ProcessorParameterUuidReference recording_id_;

  /**
   * Whether record was set automatically when the channel was selected.
   *
   * This is so that it can be unset when selecting another track. If we don't
   * do this all the tracks end up staying on record mode.
   */
  bool record_set_automatically_ = false;

  /**
   * Region currently recording on.
   *
   * This must only be set by the RecordingManager when processing an event
   * and should not be touched by anything else.
   */
  std::optional<arrangement::ArrangerObject::Uuid> recording_region_;

  /**
   * This is a flag to let the recording manager know that a START signal was
   * already sent for recording.
   *
   * This is because @ref recording_region_ takes a cycle or 2 to become
   * non-NULL.
   */
  bool recording_start_sent_ = false;

  /**
   * This is a flag to let the recording manager know that a STOP signal was
   * already sent for recording.i
   *
   * This is because @ref recording_region_ takes a cycle or 2 to become NULL.
   */
  bool recording_stop_sent_ = false;

  /**
   * This must only be set by the RecordingManager when temporarily pausing
   * recording, eg when looping or leaving the punch range.
   *
   * See @ref RECORDING_EVENT_TYPE_PAUSE_TRACK_RECORDING.
   */
  bool recording_paused_ = false;
};
}
