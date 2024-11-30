// SPDX-FileCopyrightText: © 2019-2022, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "gui/backend/backend/actions/transport_action.h"
#include "gui/backend/backend/project.h"
#include "gui/backend/backend/zrythm.h"
#include "gui/backend/ui.h"
#include "gui/dsp/automation_track.h"
#include "gui/dsp/port.h"
#include "gui/dsp/router.h"
#include "gui/dsp/tempo_track.h"
#include "gui/dsp/track.h"

#include "utils/flags.h"
#include "utils/math.h"

using namespace zrythm;

TempoTrack::TempoTrack (int track_pos)
    : Track (
        Track::Type::Tempo,
        QObject::tr ("Tempo").toStdString (),
        track_pos,
        PortType::Unknown,
        PortType::Unknown)
{
  main_height_ = TRACK_DEF_HEIGHT / 2;

  color_ = Color (QColor ("#2f6c52"));
  icon_name_ = "filename-bpm-amarok";

  /* set invisible */
  visible_ = false;

  automation_tracklist_->setParent (this);
}

void
TempoTrack::init_loaded ()
{
  AutomatableTrack::init_loaded ();
}

bool
TempoTrack::initialize ()
{
  /* create bpm port */
  bpm_port_ = std::make_unique<ControlPort> (QObject::tr ("BPM").toStdString ());
  bpm_port_->set_owner (this);
  bpm_port_->id_->sym_ = "bpm";
  bpm_port_->minf_ = 60.f;
  bpm_port_->maxf_ = 360.f;
  bpm_port_->deff_ = 140.f;
  bpm_port_->set_control_value (bpm_port_->deff_, false, false);
  bpm_port_->id_->flags_ |= dsp::PortIdentifier::Flags::Bpm;
  bpm_port_->id_->flags_ |= dsp::PortIdentifier::Flags::Automatable;

  /* create time sig ports */
  beats_per_bar_port_ = std::make_unique<ControlPort> (
    QObject::tr ("Beats per bar").toStdString ());
  beats_per_bar_port_->set_owner (this);
  beats_per_bar_port_->id_->sym_ = ("beats_per_bar");
  beats_per_bar_port_->minf_ = TEMPO_TRACK_MIN_BEATS_PER_BAR;
  beats_per_bar_port_->maxf_ = TEMPO_TRACK_MAX_BEATS_PER_BAR;
  beats_per_bar_port_->deff_ = TEMPO_TRACK_MIN_BEATS_PER_BAR;
  beats_per_bar_port_->set_control_value (
    TEMPO_TRACK_DEFAULT_BEATS_PER_BAR, false, false);
  beats_per_bar_port_->id_->flags2_ |= dsp::PortIdentifier::Flags2::BeatsPerBar;
  beats_per_bar_port_->id_->flags_ |= dsp::PortIdentifier::Flags::Automatable;
  beats_per_bar_port_->id_->flags_ |= dsp::PortIdentifier::Flags::Integer;

  beat_unit_port_ =
    std::make_unique<ControlPort> (QObject::tr ("Beat unit").toStdString ());
  beat_unit_port_->set_owner (this);
  beat_unit_port_->id_->sym_ = ("beat_unit");
  beat_unit_port_->minf_ = static_cast<float> (TEMPO_TRACK_MIN_BEAT_UNIT);
  beat_unit_port_->maxf_ = static_cast<float> (TEMPO_TRACK_MAX_BEAT_UNIT);
  beat_unit_port_->deff_ = static_cast<float> (TEMPO_TRACK_MIN_BEAT_UNIT);
  beat_unit_port_->set_control_value (
    static_cast<float> (TEMPO_TRACK_DEFAULT_BEAT_UNIT), false, false);
  beat_unit_port_->id_->flags2_ |= dsp::PortIdentifier::Flags2::BeatUnit;
  beat_unit_port_->id_->flags_ |= dsp::PortIdentifier::Flags::Automatable;
  beat_unit_port_->id_->flags_ |= dsp::PortIdentifier::Flags::Integer;

  generate_automation_tracks ();

  return true;
}

bool
TempoTrack::validate () const
{
  return Track::validate_base () && AutomatableTrack::validate_base ();
}

bpm_t
TempoTrack::get_bpm_at_pos (const Position pos)
{
  auto at = AutomationTrack::find_from_port_id (*bpm_port_->id_, false);
  return at->get_val_at_pos (pos, false, false, Z_F_NO_USE_SNAPSHOTS);
}

bpm_t
TempoTrack::get_current_bpm () const
{
  return bpm_port_->get_control_value (false);
}

double
TempoTrack::getBpm () const
{
  return get_current_bpm ();
}

int
TempoTrack::getBeatUnit () const
{
  return get_beat_unit ();
}

int
TempoTrack::getBeatsPerBar () const
{
  return get_beats_per_bar ();
}

std::string
TempoTrack::get_current_bpm_as_str ()
{
  auto bpm = get_current_bpm ();
  return fmt::format ("{:.2f}", bpm);
}

void
TempoTrack::set_bpm (bpm_t bpm, bpm_t start_bpm, bool temporary, bool fire_events)
{
  if (
    AUDIO_ENGINE->transport_type_
    == AudioEngine::JackTransportType::NoJackTransport)
    {
      z_debug ("bpm <{:f}>, temporary <{}>", bpm, temporary);
    }

  if (bpm < TEMPO_TRACK_MIN_BPM)
    {
      bpm = TEMPO_TRACK_MIN_BPM;
    }
  else if (bpm > TEMPO_TRACK_MAX_BPM)
    {
      bpm = TEMPO_TRACK_MAX_BPM;
    }

  if (temporary)
    {
      bpm_port_->set_control_value (bpm, false, false);
    }
  else
    {
      try
        {
          UNDO_MANAGER->perform (
            new gui::actions::TransportAction (start_bpm, bpm, false));
        }
      catch (const ZrythmException &e)
        {
          e.handle (QObject::tr ("Failed to change BPM"));
        }
    }

  if (fire_events)
    {
      Q_EMIT bpmChanged (bpm);
    }
}

void
TempoTrack::setBpm (double bpm)
{
  if (math_doubles_equal_epsilon (bpm, get_current_bpm (), 0.001))
    {
      return;
    }

  set_bpm (static_cast<float> (bpm), get_current_bpm (), false, true);
}

void
TempoTrack::set_bpm_from_str (const std::string &str)
{
  auto bpm = (float) atof (str.c_str ());
  if (math_floats_equal (bpm, 0))
    {
// TODO
#if 0
      ui_show_error_message (
        QObject::tr ("Invalid BPM Value"),
        QObject::tr ("Please enter a positive decimal number"));
#endif
      return;
    }

  setBpm (bpm);
}

void
TempoTrack::set_beat_unit_from_enum (BeatUnit ebeat_unit)
{
  z_return_if_fail (
    !AUDIO_ENGINE->run_.load ()
    || (ROUTER && ROUTER->is_processing_kickoff_thread ()));

  beat_unit_port_->set_control_value (
    static_cast<float> (ebeat_unit), F_NOT_NORMALIZED, true);
  // EVENTS_PUSH (EventType::ET_TIME_SIGNATURE_CHANGED, nullptr);
}

BeatUnit
TempoTrack::beat_unit_to_enum (int beat_unit)
{
  switch (beat_unit)
    {
    case 2:
      return BeatUnit::Two;
    case 4:
      return BeatUnit::Four;
    case 8:
      return BeatUnit::Eight;
    case 16:
      return BeatUnit::Sixteen;
      ;
    default:
      break;
    }
  z_return_val_if_reached (ENUM_INT_TO_VALUE (BeatUnit, 0));
}

void
TempoTrack::set_beat_unit (int beat_unit)
{
  set_beat_unit_from_enum (beat_unit_to_enum (beat_unit));
}

void
TempoTrack::set_beats_per_bar (int beats_per_bar)
{
  z_return_if_fail (
    !AUDIO_ENGINE->run_.load ()
    || (ROUTER && ROUTER->is_processing_kickoff_thread ()));

  beats_per_bar_port_->set_control_value (beats_per_bar, F_NOT_NORMALIZED, true);
  // EVENTS_PUSH (EventType::ET_TIME_SIGNATURE_CHANGED, nullptr);
}

int
TempoTrack::get_beats_per_bar () const
{
  return (int) math_round_float_to_signed_32 (beats_per_bar_port_->control_);
}

int
TempoTrack::get_beat_unit () const
{
  BeatUnit ebu =
    (BeatUnit) math_round_float_to_signed_32 (beat_unit_port_->control_);
  return beat_unit_enum_to_int (ebu);
}

void
TempoTrack::clear_objects ()
{
  /* TODO */
}

void
TempoTrack::append_ports (std::vector<Port *> &ports, bool include_plugins) const
{
  ports.push_back (bpm_port_.get ());
  ports.push_back (beats_per_bar_port_.get ());
  ports.push_back (beat_unit_port_.get ());
}

void
TempoTrack::init_after_cloning (const TempoTrack &other)
{
  bpm_port_ = other.bpm_port_->clone_unique ();
  beats_per_bar_port_ = other.beats_per_bar_port_->clone_unique ();
  beat_unit_port_ = other.beat_unit_port_->clone_unique ();
  Track::copy_members_from (other);
  AutomatableTrack::copy_members_from (other);
}
