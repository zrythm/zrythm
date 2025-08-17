// SPDX-FileCopyrightText: © 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "dsp/parameter.h"
#include "dsp/processor_base.h"

#include <QtQmlIntegration>

namespace zrythm::structure::tracks
{
/**
 * @brief Mixin class for a track that can record events (excluding automation).
 */
class RecordableTrackMixin : public QObject
{
  Q_OBJECT
  Q_PROPERTY (
    bool recording READ recording WRITE setRecording NOTIFY recordingChanged)
  QML_ELEMENT
  QML_UNCREATABLE ("")

public:
  using NameProvider = std::function<utils::Utf8String ()>;
  using AutoarmEnabledGetter = GenericBoolGetter;

  RecordableTrackMixin (
    dsp::ProcessorBase::ProcessorBaseDependencies dependencies,
    NameProvider                                  name_provider,
    AutoarmEnabledGetter                          autoarm_enabled_checker,
    QObject *                                     parent = nullptr);
  ~RecordableTrackMixin () override = default;
  Z_DISABLE_COPY_MOVE (RecordableTrackMixin)

  // ========================================================================
  // QML Interface
  // ========================================================================

  [[gnu::hot]] bool recording () const
  {
    const auto &recording_param = get_recording_param ();
    return recording_param.range ().is_toggled (recording_param.baseValue ());
  }
  void          setRecording (bool recording);
  Q_SIGNAL void recordingChanged (bool recording);

  /**
   * @brief To be fired when the track's selection status changes.
   *
   * @param selected Whether the track is now selected.
   */
  Q_SLOT void onRecordableTrackSelectedChanged (bool selected);

  // ========================================================================

  dsp::ProcessorParameter &get_recording_param () const
  {
    return *recording_id_.get_object_as<dsp::ProcessorParameter> ();
  }

private:
  static constexpr auto kRecordingIdKey = "recordingId"sv;
  static constexpr auto kRecordSetAutomaticallyKey = "recordSetAutomatically"sv;
  friend void to_json (nlohmann::json &j, const RecordableTrackMixin &track)
  {
    j[kRecordingIdKey] = track.recording_id_;
    j[kRecordSetAutomaticallyKey] = track.record_set_automatically_;
  }
  friend void from_json (const nlohmann::json &j, RecordableTrackMixin &track)
  {
    track.recording_id_ = { track.dependencies_.param_registry_ };
    j.at (kRecordingIdKey).get_to (track.recording_id_);
    j.at (kRecordSetAutomaticallyKey).get_to (track.record_set_automatically_);
  }

  friend void init_from (
    RecordableTrackMixin       &obj,
    const RecordableTrackMixin &other,
    utils::ObjectCloneType      clone_type)
  {
    obj.recording_id_ = other.recording_id_;
    obj.record_set_automatically_ = other.record_set_automatically_;
  }

private:
  dsp::ProcessorBase::ProcessorBaseDependencies dependencies_;

  /**
   * @brief Returns whether autoarm is currently enabled.
   *
   * Used to auto-arm the track when selected.
   */
  GenericBoolGetter autoarm_enabled_checker_;

  // Used for debugging.
  NameProvider name_provider_;

  /** Recording or not. */
  dsp::ProcessorParameterUuidReference recording_id_;

  /**
   * Whether record was set automatically when the channel was selected.
   *
   * This is so that it can be unset when selecting another track. If we don't
   * do this all the tracks end up staying on record mode.
   */
  bool record_set_automatically_ = false;
};
}
