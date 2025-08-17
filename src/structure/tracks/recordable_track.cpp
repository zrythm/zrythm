// SPDX-FileCopyrightText: © 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <utility>

#include "structure/tracks/recordable_track.h"

namespace zrythm::structure::tracks
{
RecordableTrackMixin::RecordableTrackMixin (
  dsp::ProcessorBase::ProcessorBaseDependencies dependencies,
  NameProvider                                  name_provider,
  GenericBoolGetter                             autoarm_enabled_checker,
  QObject *                                     parent)
    : QObject (parent), dependencies_ (dependencies),
      autoarm_enabled_checker_ (std::move (autoarm_enabled_checker)),
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

  record_set_automatically_ = false;

  Q_EMIT recordingChanged (rec);
}

void
RecordableTrackMixin::onRecordableTrackSelectedChanged (bool selected)
{
  if (autoarm_enabled_checker_ ())
    {
      if (selected && !recording ())
        {
          get_recording_param ().setBaseValue (1.f);
          record_set_automatically_ = true;
          Q_EMIT recordingChanged (true);
        }
      else if (!selected && record_set_automatically_)
        {
          get_recording_param ().setBaseValue (0.f);
          Q_EMIT recordingChanged (false);
        }
    }
}
}
