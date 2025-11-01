// SPDX-FileCopyrightText: © 2018-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <algorithm>
#include <utility>

#include "engine/session/transport.h"
#include "gui/backend/backend/project.h"
#include "gui/backend/backend/settings_manager.h"
#include "structure/arrangement/marker.h"
#include "structure/tracks/marker_track.h"
#include "structure/tracks/tracklist.h"

namespace zrythm::engine::session
{
/**
 * A buffer of n bars after the end of the last object.
 */
// constexpr int BARS_END_BUFFER = 4;

/** Millisec to allow moving further backward when very close to the calculated
 * backward position. */
constexpr auto REPEATED_BACKWARD_MS = au::milli (units::seconds) (240);

Transport::Transport (
  dsp::ProcessorBase::ProcessorBaseDependencies dependencies,
  const dsp::TempoMap                          &tempo_map,
  ConfigProvider                                config_provider,
  QObject *                                     parent)
    : QObject (parent), playhead_ (tempo_map),
      playhead_adapter_ (new dsp::PlayheadQmlWrapper (playhead_, this)),
      time_conversion_funcs_ (
        dsp::AtomicPosition::TimeConversionFunctions::from_tempo_map (
          playhead_.get_tempo_map ())),
      cue_position_ (*time_conversion_funcs_),
      cue_position_adapter_ (
        utils::make_qobject_unique<
          dsp::AtomicPositionQmlAdapter> (cue_position_, std::nullopt, this)),
      loop_start_position_ (*time_conversion_funcs_),
      loop_start_position_adapter_ (
        utils::make_qobject_unique<
          dsp::AtomicPositionQmlAdapter> (loop_start_position_, std::nullopt, this)),
      loop_end_position_ (*time_conversion_funcs_),
      loop_end_position_adapter_ (
        utils::make_qobject_unique<
          dsp::AtomicPositionQmlAdapter> (loop_end_position_, std::nullopt, this)),
      punch_in_position_ (*time_conversion_funcs_),
      punch_in_position_adapter_ (
        utils::make_qobject_unique<
          dsp::AtomicPositionQmlAdapter> (punch_in_position_, std::nullopt, this)),
      punch_out_position_ (*time_conversion_funcs_),
      punch_out_position_adapter_ (
        utils::make_qobject_unique<
          dsp::AtomicPositionQmlAdapter> (punch_out_position_, std::nullopt, this)),
      property_notification_timer_ (new QTimer (this)),
      config_provider_ (std::move (config_provider))
{
  z_debug ("Creating transport...");

  property_notification_timer_->setInterval (16);
  property_notification_timer_->callOnTimeout (this, [this] () {
    if (needs_property_notification_.exchange (false))
      {
        Q_EMIT playStateChanged (play_state_);
      }
  });
  property_notification_timer_->start ();

  if (parent == nullptr)
    {
      return;
    }

  /* set initial total number of beats this is applied to the ruler */
  total_bars_ = TRANSPORT_DEFAULT_TOTAL_BARS;

  loop_end_position_.set_ticks (tempo_map.musical_position_to_tick (
    { .bar = 5, .beat = 1, .sixteenth = 1, .tick = 0 }));
  punch_in_position_.set_ticks (tempo_map.musical_position_to_tick (
    { .bar = 3, .beat = 1, .sixteenth = 1, .tick = 0 }));
  punch_out_position_.set_ticks (tempo_map.musical_position_to_tick (
    { .bar = 5, .beat = 1, .sixteenth = 1, .tick = 0 }));

  /* create ports */
  roll_ = std::make_unique<dsp::MidiPort> (u8"Roll", dsp::PortFlow::Input);
  roll_->set_symbol (u8"roll");
  roll_->set_full_designation_provider (this);
  // roll_->id_->flags_ |= PortIdentifier::Flags::Trigger;

  stop_ = std::make_unique<dsp::MidiPort> (u8"Stop", dsp::PortFlow::Input);
  stop_->set_symbol (u8"stop");
  stop_->set_full_designation_provider (this);
  // stop_->id_->flags_ |= PortIdentifier::Flags::Trigger;

  backward_ =
    std::make_unique<dsp::MidiPort> (u8"Backward", dsp::PortFlow::Input);
  backward_->set_symbol (u8"backward");
  backward_->set_full_designation_provider (this);
  // backward_->id_->flags_ |= PortIdentifier::Flags::Trigger;

  forward_ = std::make_unique<dsp::MidiPort> (u8"Forward", dsp::PortFlow::Input);
  forward_->set_symbol (u8"forward");
  forward_->set_full_designation_provider (this);
  // forward_->id_->flags_ |= PortIdentifier::Flags::Trigger;

  loop_toggle_ =
    std::make_unique<dsp::MidiPort> (u8"Loop toggle", dsp::PortFlow::Input);
  loop_toggle_->set_symbol (u8"loop_toggle");
  loop_toggle_->set_full_designation_provider (this);
  // loop_toggle_->id_->flags_ |= PortIdentifier::Flags::Toggle;

  rec_toggle_ =
    std::make_unique<dsp::MidiPort> (u8"Rec toggle", dsp::PortFlow::Input);
  rec_toggle_->set_symbol (u8"rec_toggle");
  rec_toggle_->set_full_designation_provider (this);
  // rec_toggle_->id_->flags_ |= PortIdentifier::Flags::Toggle;

  const auto load_metronome_sample = [] (QFile f) {
    if (!f.open (QFile::ReadOnly | QFile::Text))
      {
        throw std::runtime_error (
          fmt::format ("Failed to open file at {}", f.fileName ()));
      }
    const QByteArray                         wavBytes = f.readAll ();
    std::unique_ptr<juce::AudioFormatReader> reader;
    {
      auto stream = std::make_unique<juce::MemoryInputStream> (
        wavBytes.constData (), static_cast<size_t> (wavBytes.size ()), false);
      juce::WavAudioFormat wavFormat;
      reader.reset (wavFormat.createReaderFor (stream.release (), false));
    }
    if (!reader)
      throw std::runtime_error ("Not a valid WAV");

    const int numChannels = static_cast<int> (reader->numChannels);
    const int numSamples = static_cast<int> (reader->lengthInSamples);

    juce::AudioBuffer<float> buffer;
    buffer.setSize (numChannels, numSamples);

    reader->read (&buffer, 0, numSamples, 0, true, numChannels > 1);
    return buffer;
  };
  metronome_ = utils::make_qobject_unique<dsp::Metronome> (
    dependencies, *this, tempo_map,
    load_metronome_sample (QFile (u":/qt/qml/Zrythm/wav/square_emphasis.wav"_s)),
    load_metronome_sample (QFile (u":/qt/qml/Zrythm/wav/square_normal.wav"_s)),
    zrythm::gui::SettingsManager::get_instance ()->get_metronomeEnabled (),
    zrythm::gui::SettingsManager::get_instance ()->get_metronomeVolume (), this);
  QObject::connect (
    metronome_.get (), &dsp::Metronome::volumeChanged,
    zrythm::gui::SettingsManager::get_instance (), [] (float val) {
      zrythm::gui::SettingsManager::get_instance ()->set_metronomeVolume (val);
    });
  QObject::connect (
    zrythm::gui::SettingsManager::get_instance (),
    &zrythm::gui::SettingsManager::metronomeVolume_changed, metronome_.get (),
    [this] (float val) { metronome_->setVolume (val); });
  QObject::connect (
    metronome_.get (), &dsp::Metronome::enabledChanged,
    zrythm::gui::SettingsManager::get_instance (), [] (bool val) {
      zrythm::gui::SettingsManager::get_instance ()->set_metronomeEnabled (val);
    });
  QObject::connect (
    zrythm::gui::SettingsManager::get_instance (),
    &zrythm::gui::SettingsManager::metronomeEnabled_changed, metronome_.get (),
    [this] (bool val) { metronome_->setEnabled (val); });
}

void
Transport::setLoopEnabled (bool enabled)
{
  if (loop_ == enabled)
    {
      return;
    }

  loop_ = enabled;
  Q_EMIT (loopEnabledChanged (loop_));
}

void
Transport::setRecordEnabled (bool enabled)
{
  if (recording_ == enabled)
    {
      return;
    }

  recording_ = enabled;
  Q_EMIT recordEnabledChanged (recording_);
}

void
Transport::setPunchEnabled (bool enabled)
{
  if (punch_mode_ == enabled)
    return;

  punch_mode_ = enabled;
  Q_EMIT punchEnabledChanged (enabled);
}

Transport::PlayState
Transport::getPlayState () const
{
  return play_state_;
}

void
Transport::setPlayState (PlayState state)
{
  if (play_state_ == state)
    {
      return;
    }

  play_state_ = state;
  Q_EMIT playStateChanged (play_state_);
}

void
init_from (
  Transport             &obj,
  const Transport       &other,
  utils::ObjectCloneType clone_type)
{
  obj.total_bars_ = other.total_bars_;

  obj.loop_start_position_.set_ticks (other.loop_start_position_.get_ticks ());
  obj.playhead_.set_position_ticks (other.playhead_.position_ticks ());
  obj.loop_end_position_.set_ticks (other.loop_end_position_.get_ticks ());
  obj.cue_position_.set_ticks (other.cue_position_.get_ticks ());
  obj.punch_in_position_.set_ticks (other.punch_in_position_.get_ticks ());
  obj.punch_out_position_.set_ticks (other.punch_out_position_.get_ticks ());

  // TODO
#if 0
  // Clone ports if they exit using a lambda
  auto clone_port = [] (const auto &port) {
    return port ? utils::clone_unique (*port) : nullptr;
  };

  obj.roll_ = clone_port (other.roll_);
  obj.stop_ = clone_port (other.stop_);
  obj.backward_ = clone_port (other.backward_);
  obj.forward_ = clone_port (other.forward_);
  obj.loop_toggle_ = clone_port (other.loop_toggle_);
  obj.rec_toggle_ = clone_port (other.rec_toggle_);
#endif
}

void
Transport::set_play_state_rt_safe (PlayState state)
{
  if (play_state_ == state)
    {
      return;
    }

  play_state_ = state;
  needs_property_notification_.store (true);
}

void
Transport::prepare_audio_regions_for_stretch (
  std::optional<structure::arrangement::ArrangerObjectSpan> sel_var)
{
// TODO
#if 0
  if (sel_var)
    {
      for (
        auto * obj :
        sel_var->template get_elements_by_type<
          structure::arrangement::AudioRegion> ())
        {
          obj->before_length_ = obj->get_length_in_ticks ();
        }
    }
  else
    {
      for (
        auto * track :
        TRACKLIST->get_track_span ()
          .get_elements_by_type<structure::tracks::AudioTrack> ())
        {
          for (auto &lane_var : track->lanes_)
            {
              auto * lane = std::get<structure::tracks::AudioLane *> (lane_var);
              for (auto * region : lane->get_children_view ())
                {
                  region->before_length_ = region->get_length_in_ticks ();
                }
            }
        }
    }
#endif
}

void
Transport::stretch_regions (
  std::optional<structure::arrangement::ArrangerObjectSpan> sel_var,
  bool                                                      with_fixed_ratio,
  double                                                    time_ratio,
  bool                                                      force)
{
// TODO
#if 0
  if (sel_var)
    {
      const auto &sel = *sel_var;
      for (
        auto * region :
        sel.template get_elements_derived_from<structure::arrangement::Region> ())
        {
          auto r_variant = convert_to_variant<
            structure::arrangement::RegionPtrVariant> (region);
          std::visit (
            [&] (auto &&r) {
              if constexpr (
                std::is_same_v<
                  base_type<decltype (r)>, structure::arrangement::AudioRegion>)
                {
                  /* don't stretch audio regions with musical mode off */
                  if (!r->get_musical_mode () && !force)
                    return;
                }

              double ratio =
                with_fixed_ratio
                  ? time_ratio
                  : r->get_length_in_ticks () / r->before_length_;
              r->stretch (ratio);
            },
            r_variant);
        }
    }
  else
    {
      for (
        auto * track :
        TRACKLIST->get_track_span ()
          .get_elements_by_type<structure::tracks::AudioTrack> ())
        {
          for (auto &lane_var : track->lanes_)
            {
              auto * lane = std::get<structure::tracks::AudioLane *> (lane_var);
              for (auto * region : lane->get_children_view ())
                {
                  /* don't stretch regions with musical mode off */
                  if (!region->get_musical_mode ())
                    continue;

                  double ratio =
                    with_fixed_ratio
                      ? time_ratio
                      : region->get_length_in_ticks () / region->before_length_;
                  region->stretch (ratio);
                }
            }
        }
    }
#endif
}

void
Transport::requestPause ()
{
  set_play_state_rt_safe (PlayState::PauseRequested);

  playhead_before_pause_ = playhead_.position_ticks ();
  if (config_provider_.return_to_cue_on_pause_ ())
    {
      move_playhead (cue_position_.get_ticks (), false);
    }
}

void
Transport::requestRoll ()
{
  {
    /* handle countin */
    // int num_bars = config_provider_.metronome_countin_bars_ ();
// TODO: convert num bars to countin frames
#if 0
      countin_frames_remaining_ =
        units::samples ((long) ((double) num_bars * frames_per_bar));
#endif

    if (recording_)
      {
        /* handle preroll */
        int        num_bars = config_provider_.recording_preroll_bars_ ();
        auto       pos_tick = playhead_.position_ticks ();
        const auto pos_musical =
          playhead_.get_tempo_map ().tick_to_musical_position (
            au::round_as<int64_t> (units::ticks, pos_tick));
        auto new_pos_musical = pos_musical;
        new_pos_musical.bar = std::max (new_pos_musical.bar - num_bars, 1);
        pos_tick =
          playhead_.get_tempo_map ().musical_position_to_tick (new_pos_musical);
        auto pos_frame =
          playhead_.get_tempo_map ().tick_to_samples_rounded (pos_tick);
        recording_preroll_frames_remaining_ =
          playhead_.get_tempo_map ().tick_to_samples_rounded (
            playhead_.position_ticks ())
          - pos_frame;
        playhead_adapter_->setTicks (pos_tick.in (units::ticks));
      }
  }

  setPlayState (PlayState::RollRequested);
}

void
Transport::add_to_playhead_in_audio_thread (const units::sample_t nframes)
{
  const auto cur_pos = get_playhead_position_in_audio_thread ();
  auto new_pos = get_playhead_position_after_adding_frames_in_audio_thread (
    get_playhead_position_in_audio_thread (), nframes);
  auto diff = new_pos - cur_pos;
  playhead_.advance_processing (diff);
}

bool
Transport::can_user_move_playhead () const
{
  return !recording_ || play_state_ != PlayState::Rolling;
}

void
Transport::move_playhead (units::precise_tick_t target_ticks, bool set_cue_point)
{
  /* if currently recording, do nothing */
  if (!can_user_move_playhead ())
    {
      z_info ("currently recording - refusing to move playhead manually");
      return;
    }

  /* send MIDI note off on currently playing timeline objects */
  for (
    auto * track :
    TRACKLIST->collection ()->get_track_span ()
      | std::views::transform ([] (const auto &track_var) {
          return structure::tracks::from_variant (track_var);
        }))
    {
      if (track->lanes () == nullptr)
        continue;

      for (const auto &lane : track->lanes ()->lanes_view ())
        {
          for (
            auto * region :
            lane->structure::arrangement::ArrangerObjectOwner<
              structure::arrangement::MidiRegion>::get_children_view ())
            {
              const auto playhead_pos_ticks = playhead_.position_ticks ();
              const auto playhead_pos_samples =
                playhead_.get_tempo_map ().tick_to_samples_rounded (
                  playhead_pos_ticks);
              if (!region->bounds ()->is_hit (playhead_pos_samples, true))
                continue;

              for (auto * midi_note : region->get_children_view ())
                {
                  if (midi_note->bounds ()->is_hit (playhead_pos_samples))
                    {
                      track->get_track_processor ()
                        ->get_piano_roll_port ()
                        .midi_events_.queued_events_
                        .add_note_off (1, midi_note->pitch (), 0);
                    }
                }
            }
        }
    }

  /* move to new pos */
  playhead_adapter_->setTicks (target_ticks.in (units::ticks));

  if (set_cue_point)
    {
      /* move cue point */
      cue_position_adapter_->setTicks (target_ticks.in (units::ticks));
    }
}

#if 0
void
Transport::foreach_arranger_handle_playhead_auto_scroll (
  ArrangerWidget * arranger)
{
  arranger_widget_handle_playhead_auto_scroll (arranger, true);
}
#endif

void
Transport::move_to_marker_or_pos_and_fire_events (
  const structure::arrangement::Marker * marker,
  std::optional<units::precise_tick_t>   pos_ticks)
{
  move_playhead (
    (marker != nullptr) ? units::ticks (marker->position ()->ticks ()) : *pos_ticks,
    true);

  {
    // arranger_widget_foreach (foreach_arranger_handle_playhead_auto_scroll);
  }
}

/**
 * Moves the playhead to the start Marker.
 */
void
Transport::goto_start_marker ()
{
  auto start_marker = P_MARKER_TRACK->get_start_marker ();
  z_return_if_fail (start_marker);
  move_to_marker_or_pos_and_fire_events (start_marker, std::nullopt);
}

/**
 * Moves the playhead to the end Marker.
 */
void
Transport::goto_end_marker ()
{
  auto end_marker = P_MARKER_TRACK->get_end_marker ();
  z_return_if_fail (end_marker);
  move_to_marker_or_pos_and_fire_events (end_marker, std::nullopt);
}

/**
 * Moves the playhead to the prev Marker.
 */
void
Transport::goto_prev_marker ()
{
  /* gather all markers */
  std::vector<units::precise_tick_t> marker_ticks;
  for (auto * marker : P_MARKER_TRACK->get_children_view ())
    {
      marker_ticks.push_back (units::ticks (marker->position ()->ticks ()));
    }
  marker_ticks.emplace_back (cue_position_.get_ticks ());
  marker_ticks.emplace_back (loop_start_position_.get_ticks ());
  marker_ticks.emplace_back (loop_end_position_.get_ticks ());
  marker_ticks.emplace_back ();
  std::ranges::sort (marker_ticks);

  for (
    const auto &[index, marker_tick] :
    marker_ticks | std::views::enumerate | std::views::reverse)
    {
      if (marker_tick >= playhead_.position_ticks ())
        continue;

      if (
        isRolling () && index > 0
        && (playhead_.get_tempo_map ().tick_to_seconds (
              playhead_.position_ticks ())
            - playhead_.get_tempo_map ().tick_to_seconds (marker_tick))
             < REPEATED_BACKWARD_MS)
        {
          continue;
        }

      move_to_marker_or_pos_and_fire_events (nullptr, marker_tick);
      break;
    }
}

/**
 * Moves the playhead to the next Marker.
 */
void
Transport::goto_next_marker ()
{
  /* gather all markers */
  std::vector<units::precise_tick_t> marker_ticks;
  for (auto * marker : P_MARKER_TRACK->get_children_view ())
    {
      marker_ticks.push_back (units::ticks (marker->position ()->ticks ()));
    }
  marker_ticks.emplace_back (cue_position_.get_ticks ());
  marker_ticks.emplace_back (loop_start_position_.get_ticks ());
  marker_ticks.emplace_back (loop_end_position_.get_ticks ());
  marker_ticks.emplace_back ();
  std::ranges::sort (marker_ticks);

  for (auto &marker : marker_ticks)
    {
      if (marker > playhead_.position_ticks ())
        {
          move_to_marker_or_pos_and_fire_events (nullptr, marker);
          break;
        }
    }
}

void
Transport::set_loop_range (
  bool                  range1,
  units::precise_tick_t start_pos,
  units::precise_tick_t pos,
  bool                  snap)
{
  auto * pos_to_set = range1 ? &loop_start_position_ : &loop_end_position_;

  if (pos < units::ticks (0))
    {
      pos_to_set->set_ticks (units::ticks (0));
    }
  else
    {
      pos_to_set->set_ticks (pos);
    }

  if (snap)
    {
      // TODO
      // *static_cast<Position *> (pos_to_set) = SNAP_GRID_TIMELINE->snap (
      //   *pos_to_set, start_pos);
    }
}

bool
Transport::position_is_inside_punch_range (const units::sample_t pos)
{
  return pos >= punch_in_position_.get_samples ()
         && pos < punch_out_position_.get_samples ();
}

void
Transport::recalculate_total_bars (
  std::optional<structure::arrangement::ArrangerObjectSpan> sel_var)
{
// TODO
#if 0

  int total_bars = total_bars_;
  if (sel_var)
    {
      const auto &sel = *sel_var;
      for (const auto &obj_var : sel)
        {
          std::visit (
            [&] (auto &&obj) {
              using ObjT = base_type<decltype (obj)>;
              Position pos;
              if constexpr (structure::arrangement::BoundedObject<ObjT>)
                {
                  pos =
                    structure::arrangement::ArrangerObjectSpan::bounds_projection (
                      obj)
                      ->get_end_position ();
                }
              else
                {
                  pos = obj->get_position ();
                }
              int pos_bars = pos.get_total_bars (
                true, ticks_per_bar_, project_->audio_engine_->frames_per_tick_);
              if (pos_bars > total_bars - 3)
                {
                  total_bars = pos_bars + BARS_END_BUFFER;
                }
            },
            obj_var);
        }
    }
  /* else no selections, calculate total bars for
   * every object */
  else
    {
      total_bars = TRACKLIST->get_track_span ().get_total_bars (
        *this, TRANSPORT_DEFAULT_TOTAL_BARS);

      total_bars += BARS_END_BUFFER;
    }

  update_total_bars (total_bars, true);
#endif
}

utils::Utf8String
Transport::get_full_designation_for_port (const dsp::Port &port) const
{
  return utils::Utf8String::from_utf8_encoded_string (
    fmt::format ("Transport/{}", port.get_label ()));
}

void
Transport::update_total_bars (int total_bars, bool fire_events)
{
  z_return_if_fail (total_bars >= TRANSPORT_DEFAULT_TOTAL_BARS);

  if (total_bars_ == total_bars)
    return;

  total_bars_ = total_bars;

  if (fire_events)
    {
      // EVENTS_PUSH (EventType::ET_TRANSPORT_TOTAL_BARS_CHANGED, nullptr);
    }
}

void
Transport::moveBackward ()
{
  const auto &tempo_map = playhead_.get_tempo_map ();
  auto        pos_ticks = units::ticks (SNAP_GRID_TIMELINE->prevSnapPoint (
    playhead_.position_ticks ().in (units::ticks)));
  const auto  pos_frames = tempo_map.tick_to_samples_rounded (pos_ticks);
  /* if prev snap point is exactly at the playhead or very close it, go back
   * more */
  const auto playhead_ticks = playhead_.position_ticks ();
  const auto playhead_frames =
    tempo_map.tick_to_samples_rounded (playhead_ticks);
  if (
    pos_frames > units::samples(0)
    && (pos_frames == playhead_frames || (isRolling () && (tempo_map.tick_to_seconds(playhead_ticks) - tempo_map.tick_to_seconds(pos_ticks) < REPEATED_BACKWARD_MS))))
    {
      pos_ticks = units::ticks (SNAP_GRID_TIMELINE->prevSnapPoint (
        (pos_ticks - units::ticks (1.0)).in (units::ticks)));
    }
  move_playhead (pos_ticks, true);
}

void
Transport::moveForward ()
{
  double pos_ticks = SNAP_GRID_TIMELINE->nextSnapPoint (
    playhead_.position_ticks ().in (units::ticks));
  move_playhead (units::ticks (pos_ticks), true);
}

void
to_json (nlohmann::json &j, const Transport &transport)
{
  j = nlohmann::json{
    { Transport::kTotalBarsKey,    transport.total_bars_          },
    { Transport::kPlayheadKey,     transport.playhead_            },
    { Transport::kCuePosKey,       transport.cue_position_        },
    { Transport::kLoopStartPosKey, transport.loop_start_position_ },
    { Transport::kLoopEndPosKey,   transport.loop_end_position_   },
    { Transport::kPunchInPosKey,   transport.punch_in_position_   },
    { Transport::kPunchOutPosKey,  transport.punch_out_position_  },
    { Transport::kRollKey,         transport.roll_                },
    { Transport::kStopKey,         transport.stop_                },
    { Transport::kBackwardKey,     transport.backward_            },
    { Transport::kForwardKey,      transport.forward_             },
    { Transport::kLoopToggleKey,   transport.loop_toggle_         },
    { Transport::kRecToggleKey,    transport.rec_toggle_          },
  };
}
void
from_json (const nlohmann::json &j, Transport &transport)
{
  j.at (Transport::kTotalBarsKey).get_to (transport.total_bars_);
  j.at (Transport::kPlayheadKey).get_to (transport.playhead_);
  j.at (Transport::kCuePosKey).get_to (transport.cue_position_);
  j.at (Transport::kLoopStartPosKey).get_to (transport.loop_start_position_);
  j.at (Transport::kLoopEndPosKey).get_to (transport.loop_end_position_);
  j.at (Transport::kPunchInPosKey).get_to (transport.punch_in_position_);
  j.at (Transport::kPunchOutPosKey).get_to (transport.punch_out_position_);
  j.at (Transport::kRollKey).get_to (*transport.roll_);
  j.at (Transport::kStopKey).get_to (*transport.stop_);
  j.at (Transport::kBackwardKey).get_to (*transport.backward_);
  j.at (Transport::kForwardKey).get_to (*transport.forward_);
  j.at (Transport::kLoopToggleKey).get_to (*transport.loop_toggle_);
  j.at (Transport::kRecToggleKey).get_to (*transport.rec_toggle_);
}
}
