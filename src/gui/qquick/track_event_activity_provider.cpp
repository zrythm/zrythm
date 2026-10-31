// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "gui/qquick/track_event_activity_provider.h"
#include "structure/tracks/channel.h"

namespace zrythm::gui::qquick
{

TrackEventActivityProvider::TrackEventActivityProvider (QObject * parent)
    : QObject (parent),
      note_provider_ (utils::make_qobject_unique<dsp::MidiActivityProvider> (this))
{
  QObject::connect (
    note_provider_.get (), &dsp::MidiActivityProvider::midiEventsReceived, this,
    [this] (dsp::MidiActivityProvider::MidiEventKinds kinds) {
      if (
        kinds
        & (dsp::MidiActivityProvider::MidiEventKind::NoteOn | dsp::MidiActivityProvider::MidiEventKind::NoteOff | dsp::MidiActivityProvider::MidiEventKind::AllNotesOff))
        {
          Q_EMIT eventReceived ();
        }
    });
}

TrackEventActivityProvider::~TrackEventActivityProvider () = default;

structure::tracks::Track *
TrackEventActivityProvider::track () const
{
  return track_;
}

void
TrackEventActivityProvider::setTrack (structure::tracks::Track * track)
{
  if (track_ == track)
    return;

  if (track_ != nullptr)
    {
      QObject::disconnect (track_, nullptr, this, nullptr);
    }

  track_ = track;

  if (track_ != nullptr)
    {
      QObject::connect (track_, &QObject::destroyed, this, [this] () {
        track_ = nullptr;
        rebind_sources ();
        Q_EMIT trackChanged ();
      });
    }

  rebind_sources ();
  Q_EMIT trackChanged ();
}

dsp::PortObservationManager *
TrackEventActivityProvider::portObservationManager () const
{
  return manager_;
}

void
TrackEventActivityProvider::setPortObservationManager (
  dsp::PortObservationManager * manager)
{
  if (manager_ == manager)
    return;

  if (manager_ != nullptr)
    {
      QObject::disconnect (manager_, nullptr, this, nullptr);
    }

  manager_ = manager;
  if (manager_ != nullptr)
    {
      QObject::connect (manager_, &QObject::destroyed, this, [this] () {
        manager_.clear ();
        rebind_sources ();
        Q_EMIT portObservationManagerChanged ();
      });
    }

  rebind_sources ();
  Q_EMIT portObservationManagerChanged ();
}

bool
TrackEventActivityProvider::hasEventSource () const
{
  return has_event_source_;
}

dsp::MidiActivityProvider *
TrackEventActivityProvider::midiActivityProvider () const
{
  return note_provider_.get ();
}

void
TrackEventActivityProvider::rebind_sources ()
{
  dsp::MidiPort * note_port = nullptr;
  if (track_ != nullptr && track_->channel () != nullptr)
    {
      note_port = track_->channel ()->postMidiFxPort ();
    }

  note_provider_->setPort (note_port);
  note_provider_->setPortObservationManager (manager_);

  const bool has_source = note_port != nullptr;
  if (has_source != has_event_source_)
    {
      has_event_source_ = has_source;
      Q_EMIT hasEventSourceChanged ();
    }
}

} // namespace zrythm::gui::qquick
