// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <bitset>

#include "dsp/midi_activity_provider.h"
#include "dsp/midi_event.h"
#include "dsp/midi_port.h"
#include "dsp/port.h"
#include "dsp/port_observation_token.h"
#include "utils/midi.h"
#include "utils/qt.h"

#include <QTimer>

namespace zrythm::dsp
{

namespace
{

MidiActivityProvider::MidiEventKind
classify_midi_event (std::span<const midi_byte_t> data)
{
  using MidiEventKind = MidiActivityProvider::MidiEventKind;

  if (data.empty ())
    return MidiEventKind::Other;

  const int expected_length = utils::midi::midi_get_msg_length (data[0]);
  if (
    expected_length < 0 || data.size () < static_cast<size_t> (expected_length))
    return MidiEventKind::Other;

  if (utils::midi::midi_is_note_on (data))
    return MidiEventKind::NoteOn;
  if (utils::midi::midi_is_note_off (data))
    return MidiEventKind::NoteOff;
  if (utils::midi::midi_is_controller (data))
    {
      if (utils::midi::midi_is_all_notes_off (data))
        return MidiEventKind::AllNotesOff;
      return MidiEventKind::ControlChange;
    }
  if (utils::midi::midi_is_program_change (data))
    return MidiEventKind::ProgramChange;
  if (utils::midi::midi_is_channel_pressure (data))
    return MidiEventKind::ChannelPressure;
  if (utils::midi::midi_is_aftertouch (data))
    return MidiEventKind::PolyAftertouch;
  if (utils::midi::midi_is_pitch_wheel (data))
    return MidiEventKind::PitchBend;
  return MidiEventKind::Other;
}

} // namespace

struct MidiActivityProvider::Impl
{
  QPointer<dsp::Port>                   port_;
  QPointer<dsp::PortObservationManager> manager_;

  std::optional<dsp::ObservationToken> token_;

  std::array<uint16_t, 128> pitch_counts_{};
  std::bitset<128>          active_notes_;
  std::bitset<128>          prev_active_notes_;

  bool has_activity_ = false;
  bool prev_has_activity_ = false;

  utils::QObjectUniquePtr<QTimer> timer_;
};

MidiActivityProvider::MidiActivityProvider (QObject * parent)
    : QObject (parent), impl_ (std::make_unique<Impl> ())
{
}

MidiActivityProvider::~MidiActivityProvider () = default;

dsp::Port *
MidiActivityProvider::port () const
{
  return impl_->port_;
}

void
MidiActivityProvider::reset_state ()
{
  impl_->token_.reset ();
  impl_->timer_.reset ();
  impl_->active_notes_.reset ();
  impl_->prev_active_notes_.reset ();
  impl_->pitch_counts_.fill (0);

  const bool activity_changed = impl_->has_activity_;
  impl_->has_activity_ = false;
  impl_->prev_has_activity_ = false;

  Q_EMIT activeNotesChanged ();
  if (activity_changed)
    Q_EMIT activityChanged ();
}

void
MidiActivityProvider::setPort (dsp::Port * port)
{
  if (impl_->port_ == port)
    return;

  reset_state ();

  if (impl_->port_ != nullptr)
    {
      QObject::disconnect (impl_->port_, &QObject::destroyed, this, nullptr);
      impl_->port_.clear ();
    }

  impl_->port_ = port;

  if (impl_->port_ != nullptr)
    {
      QObject::connect (impl_->port_, &QObject::destroyed, this, [this] () {
        reset_state ();
        impl_->port_.clear ();
        Q_EMIT portChanged ();
      });
      try_create_token ();
    }

  Q_EMIT portChanged ();
}

dsp::PortObservationManager *
MidiActivityProvider::portObservationManager () const
{
  return impl_->manager_;
}

void
MidiActivityProvider::setPortObservationManager (
  dsp::PortObservationManager * manager)
{
  if (impl_->manager_ == manager)
    return;

  if (impl_->manager_ != nullptr)
    {
      QObject::disconnect (impl_->manager_, nullptr, this, nullptr);
    }

  reset_state ();
  impl_->manager_ = manager;
  if (impl_->manager_ != nullptr)
    {
      QObject::connect (impl_->manager_, &QObject::destroyed, this, [this] () {
        /* QPointer instances are cleared before destroyed() runs, so
         * reset_state() drops the token without touching the manager. */
        reset_state ();
        impl_->manager_.clear ();
        Q_EMIT portObservationManagerChanged ();
      });
      try_create_token ();
    }

  Q_EMIT portObservationManagerChanged ();
}

bool
MidiActivityProvider::hasActivity () const
{
  return impl_->has_activity_;
}

bool
MidiActivityProvider::isNoteActive (int note) const
{
  if (note < 0 || note >= 128)
    return false;
  return impl_->active_notes_.test (static_cast<size_t> (note));
}

void
MidiActivityProvider::update ()
{
  impl_->has_activity_ = false;
  const auto received_kinds = process_midi_events ();

  if (impl_->active_notes_ != impl_->prev_active_notes_)
    {
      impl_->prev_active_notes_ = impl_->active_notes_;
      Q_EMIT activeNotesChanged ();
    }

  if (impl_->has_activity_ != impl_->prev_has_activity_)
    {
      impl_->prev_has_activity_ = impl_->has_activity_;
      Q_EMIT activityChanged ();
    }

  if (received_kinds != MidiEventKinds{})
    {
      Q_EMIT midiEventsReceived (received_kinds);
    }
}

void
MidiActivityProvider::try_create_token ()
{
  if (impl_->port_ == nullptr || impl_->manager_ == nullptr)
    return;
  if (!impl_->port_->is_midi ())
    return;

  impl_->token_.emplace (*impl_->manager_, *impl_->port_);

  impl_->timer_ = utils::make_qobject_unique<QTimer> (this);
  impl_->timer_->setInterval (1000 / 60);
  connect (impl_->timer_.get (), &QTimer::timeout, this, [this] () {
    update ();
  });
  impl_->timer_->start ();
}

MidiActivityProvider::MidiEventKinds
MidiActivityProvider::process_midi_events ()
{
  if (!impl_->token_ || impl_->manager_ == nullptr)
    return {};

  auto &cache = impl_->token_->cache ();
  if (cache.midi.empty ())
    return {};

  impl_->has_activity_ = true;

  MidiEventKinds received_kinds;

  dsp::midi_event::sort_with_note_off_priority (cache.midi);
  for (const auto &event : cache.midi)
    {
      auto       data = event.data ();
      const auto kind = classify_midi_event (data);
      received_kinds |= kind;

      switch (kind)
        {
        case MidiEventKind::NoteOn:
          {
            auto pitch = utils::midi::midi_get_note_number (data);
            if (pitch < 128)
              {
                ++impl_->pitch_counts_[pitch];
                impl_->active_notes_.set (pitch);
              }
            break;
          }
        case MidiEventKind::NoteOff:
          {
            auto pitch = utils::midi::midi_get_note_number (data);
            if (pitch < 128 && impl_->pitch_counts_[pitch] > 0)
              {
                --impl_->pitch_counts_[pitch];
                if (impl_->pitch_counts_[pitch] == 0)
                  impl_->active_notes_.reset (pitch);
              }
            break;
          }
        case MidiEventKind::AllNotesOff:
          {
            impl_->pitch_counts_.fill (0);
            impl_->active_notes_.reset ();
            break;
          }
        default:
          break;
        }
    }

  cache.clear_midi ();
  return received_kinds;
}

}
