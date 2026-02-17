// SPDX-FileCopyrightText: © 2024-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "dsp/parameter.h"
#include "dsp/processor_base.h"

#include <QtQmlIntegration/qqmlintegration.h>

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

  RecordableTrackMixin (
    dsp::ProcessorBase::ProcessorBaseDependencies dependencies,
    NameProvider                                  name_provider,
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

  // ========================================================================

  dsp::ProcessorParameter &get_recording_param () const
  {
    return *recording_id_.get_object_as<dsp::ProcessorParameter> ();
  }

private:
  friend void to_json (nlohmann::json &j, const RecordableTrackMixin &track);
  friend void from_json (const nlohmann::json &j, RecordableTrackMixin &track);

  friend void init_from (
    RecordableTrackMixin       &obj,
    const RecordableTrackMixin &other,
    utils::ObjectCloneType      clone_type);

private:
  dsp::ProcessorBase::ProcessorBaseDependencies dependencies_;

  // Used for debugging.
  NameProvider name_provider_;

  /** Recording or not. */
  dsp::ProcessorParameterUuidReference recording_id_;
};
}
