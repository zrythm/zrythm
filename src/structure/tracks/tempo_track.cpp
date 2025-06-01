// SPDX-FileCopyrightText: © 2019-2022, 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "engine/session/router.h"
#include "gui/backend/backend/actions/transport_action.h"
#include "gui/backend/backend/project.h"
#include "gui/backend/backend/zrythm.h"
#include "gui/backend/ui.h"
#include "gui/dsp/port.h"
#include "structure/tracks/automation_track.h"
#include "structure/tracks/tempo_track.h"
#include "structure/tracks/track.h"
#include "utils/math.h"

namespace zrythm::structure::tracks
{

TempoTrack::TempoTrack (
  TrackRegistry          &track_registry,
  PluginRegistry         &plugin_registry,
  PortRegistry           &port_registry,
  ArrangerObjectRegistry &obj_registry,
  bool                    new_identity)
    : Track (
        Track::Type::Tempo,
        PortType::Unknown,
        PortType::Unknown,
        plugin_registry,
        port_registry,
        obj_registry),
      AutomatableTrack (port_registry, new_identity),
      port_registry_ (port_registry)
{
  if (new_identity)
    {
      main_height_ = DEF_HEIGHT / 2;

      color_ = Color (QColor ("#2f6c52"));
      icon_name_ = u8"filename-bpm-amarok";

      /* set invisible */
      visible_ = false;
    }

  automation_tracklist_->setParent (this);
}

void
TempoTrack::init_loaded (
  PluginRegistry &plugin_registry,
  PortRegistry   &port_registry)
{
  AutomatableTrack::init_loaded (plugin_registry, port_registry);
}

bool
TempoTrack::initialize ()
{
  {
    /* create bpm port */
    bpm_port_ = port_registry_.create_object<ControlPort> (
      utils::Utf8String::from_qstring (QObject::tr ("BPM")));
    auto &bpm_port = get_bpm_port ();
    bpm_port.set_owner (*this);
    bpm_port.id_->sym_ = u8"bpm";
    bpm_port.range_ = { 60.f, 360.f };
    bpm_port.deff_ = 140.f;
    bpm_port.set_control_value (bpm_port.deff_, false, false);
    bpm_port.id_->flags_ |= dsp::PortIdentifier::Flags::Bpm;
    bpm_port.id_->flags_ |= dsp::PortIdentifier::Flags::Automatable;
  }

  {
    /* create time sig ports */
    beats_per_bar_port_ = port_registry_.create_object<ControlPort> (
      utils::Utf8String::from_qstring (QObject::tr ("Beats per bar")));
    auto &beats_per_bar_port = get_beats_per_bar_port ();
    beats_per_bar_port.set_owner (*this);
    beats_per_bar_port.id_->sym_ = u8"beats_per_bar";
    beats_per_bar_port.range_ = {
      TEMPO_TRACK_MIN_BEATS_PER_BAR, TEMPO_TRACK_MAX_BEATS_PER_BAR
    };
    beats_per_bar_port.deff_ = TEMPO_TRACK_MIN_BEATS_PER_BAR;
    beats_per_bar_port.set_control_value (
      TEMPO_TRACK_DEFAULT_BEATS_PER_BAR, false, false);
    beats_per_bar_port.id_->flags2_ |= dsp::PortIdentifier::Flags2::BeatsPerBar;
    beats_per_bar_port.id_->flags_ |= dsp::PortIdentifier::Flags::Automatable;
    beats_per_bar_port.id_->flags_ |= dsp::PortIdentifier::Flags::Integer;
  }

  {
    beat_unit_port_ = port_registry_.create_object<ControlPort> (
      utils::Utf8String::from_qstring (QObject::tr ("Beat unit")));
    auto &beat_unit_port = get_beat_unit_port ();
    beat_unit_port.set_owner (*this);
    beat_unit_port.id_->sym_ = u8"beat_unit";
    beat_unit_port.range_ = {
      static_cast<float> (TEMPO_TRACK_MIN_BEAT_UNIT),
      static_cast<float> (TEMPO_TRACK_MAX_BEAT_UNIT)
    };
    beat_unit_port.deff_ = static_cast<float> (TEMPO_TRACK_MIN_BEAT_UNIT);
    beat_unit_port.set_control_value (
      static_cast<float> (TEMPO_TRACK_DEFAULT_BEAT_UNIT), false, false);
    beat_unit_port.id_->flags2_ |= dsp::PortIdentifier::Flags2::BeatUnit;
    beat_unit_port.id_->flags_ |= dsp::PortIdentifier::Flags::Automatable;
    beat_unit_port.id_->flags_ |= dsp::PortIdentifier::Flags::Integer;
  }

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
  auto at = AutomationTrack::find_from_port_id (bpm_port_->id (), false);
  return at->get_val_at_pos (pos, false, false, false);
}

bpm_t
TempoTrack::get_current_bpm () const
{
  return get_bpm_port ().get_control_value (false);
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
TempoTrack::get_current_bpm_as_str () const
{
  auto bpm = get_current_bpm ();
  return fmt::format ("{:.2f}", bpm);
}

void
TempoTrack::on_control_change_event (
  const PortUuid            &port_uuid,
  const dsp::PortIdentifier &id,
  float                      value)
{
  using PortIdentifier = dsp::PortIdentifier;

  /* if bpm, update engine */
  if (ENUM_BITSET_TEST (id.flags_, PortIdentifier::Flags::Bpm))
    {
      /* this must only be called during processing kickoff or while the
       * engine is stopped */
      z_return_if_fail (
        !AUDIO_ENGINE->run_.load () || ROUTER->is_processing_kickoff_thread ());

      int beats_per_bar = get_beats_per_bar ();
      AUDIO_ENGINE->update_frames_per_tick (
        beats_per_bar, value, AUDIO_ENGINE->sample_rate_, false, true, true);
      // EVENTS_PUSH (EventType::ET_BPM_CHANGED, nullptr);
    }

  /* if time sig value, update transport caches */
  else if (
    ENUM_BITSET_TEST (id.flags2_, PortIdentifier::Flags2::BeatsPerBar)
    || ENUM_BITSET_TEST (id.flags2_, PortIdentifier::Flags2::BeatUnit))
    {
      /* this must only be called during processing kickoff or while the
       * engine is stopped */
      z_return_if_fail (
        !AUDIO_ENGINE->run_.load () || ROUTER->is_processing_kickoff_thread ());

      int   beats_per_bar = get_beats_per_bar ();
      int   beat_unit = get_beat_unit ();
      bpm_t bpm = get_current_bpm ();
      TRANSPORT->update_caches (beats_per_bar, beat_unit);
      bool update_from_ticks =
        ENUM_BITSET_TEST (id.flags2_, PortIdentifier::Flags2::BeatsPerBar);
      AUDIO_ENGINE->update_frames_per_tick (
        beats_per_bar, bpm, AUDIO_ENGINE->sample_rate_, false,
        update_from_ticks, false);
      // EVENTS_PUSH (EventType::ET_TIME_SIGNATURE_CHANGED, nullptr);
    }
}

void
TempoTrack::set_bpm (bpm_t bpm, bpm_t start_bpm, bool temporary, bool fire_events)
{
  if (
    AUDIO_ENGINE->transport_type_
    == engine::device_io::AudioEngine::JackTransportType::NoJackTransport)
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
      get_bpm_port ().set_control_value (bpm, false, false);
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
  if (utils::math::floats_near (bpm, (double) get_current_bpm (), 0.001))
    {
      return;
    }

  set_bpm (static_cast<float> (bpm), get_current_bpm (), false, true);
}

void
TempoTrack::set_bpm_from_str (const std::string &str)
{
  auto bpm = (float) atof (str.c_str ());
  if (utils::math::floats_equal (bpm, 0.f))
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

  get_beat_unit_port ().set_control_value (
    static_cast<float> (ebeat_unit), false, true);
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

  get_beats_per_bar_port ().set_control_value (beats_per_bar, false, true);
  // EVENTS_PUSH (EventType::ET_TIME_SIGNATURE_CHANGED, nullptr);
}

int
TempoTrack::get_beats_per_bar () const
{
  return (int) utils::math::round_to_signed_32 (
    get_beats_per_bar_port ().control_);
}

int
TempoTrack::get_beat_unit () const
{
  auto ebu =
    (BeatUnit) utils::math::round_to_signed_32 (get_beat_unit_port ().control_);
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
  ports.push_back (&get_bpm_port ());
  ports.push_back (&get_beats_per_bar_port ());
  ports.push_back (&get_beat_unit_port ());
}

void
TempoTrack::init_after_cloning (
  const TempoTrack &other,
  ObjectCloneType   clone_type)
{
  bpm_port_ = other.bpm_port_;
  beats_per_bar_port_ = other.beats_per_bar_port_;
  beat_unit_port_ = other.beat_unit_port_;
  Track::copy_members_from (other, clone_type);
  AutomatableTrack::copy_members_from (other, clone_type);
}
}