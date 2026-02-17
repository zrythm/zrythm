// SPDX-FileCopyrightText: © 2024-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <utility>

#include "structure/tracks/recordable_track.h"

namespace zrythm::structure::tracks
{
RecordableTrackMixin::RecordableTrackMixin (
  dsp::ProcessorBase::ProcessorBaseDependencies dependencies,
  NameProvider                                  name_provider,
  QObject *                                     parent)
    : QObject (parent), dependencies_ (dependencies),
      name_provider_ (std::move (name_provider)),
      recording_id_ (dependencies.param_registry_)
{
  recording_id_ = dependencies.param_registry_.create_object<
    dsp::ProcessorParameter> (
    dependencies.port_registry_,
    dsp::ProcessorParameter::UniqueId (u8"track_record"),
    dsp::ParameterRange::make_toggle (false),
    utils::Utf8String::from_qstring (QObject::tr ("Track record")));
}

void
RecordableTrackMixin::setRecording (bool rec)
{
  if (recording () == rec)
    return;

  z_debug ("{}: setting recording {}", name_provider_ (), rec);
  get_recording_param ().setBaseValue (rec ? 1.f : 0.f);

  if (!rec)
    {
      // TODO: this should send all notes off, but it's not the concern of
      // this class
    }

  Q_EMIT recordingChanged (rec);
}

void
to_json (nlohmann::json &j, const RecordableTrackMixin &track)
{
  j = track.recording_id_;
}
void
from_json (const nlohmann::json &j, RecordableTrackMixin &track)
{
  track.recording_id_ = { track.dependencies_.param_registry_ };
  j.get_to (track.recording_id_);
}

void
init_from (
  RecordableTrackMixin       &obj,
  const RecordableTrackMixin &other,
  utils::ObjectCloneType      clone_type)
{
  obj.recording_id_ = other.recording_id_;
}
}
