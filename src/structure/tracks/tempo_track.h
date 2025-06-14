// SPDX-FileCopyrightText: © 2019-2020, 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "structure/tracks/automatable_track.h"
#include "utils/types.h"

#define P_TEMPO_TRACK (TRACKLIST->getTempoTrack ())

namespace zrythm::structure::tracks
{
constexpr float TEMPO_TRACK_MAX_BPM = 420.f;
constexpr float TEMPO_TRACK_MIN_BPM = 40.f;
constexpr float TEMPO_TRACK_DEFAULT_BPM = 140.f;
constexpr int   TEMPO_TRACK_DEFAULT_BEATS_PER_BAR = 4;
constexpr int   TEMPO_TRACK_MIN_BEATS_PER_BAR = 1;
constexpr int   TEMPO_TRACK_MAX_BEATS_PER_BAR = 16;
constexpr auto  TEMPO_TRACK_DEFAULT_BEAT_UNIT = BeatUnit::Four;
constexpr auto  TEMPO_TRACK_MIN_BEAT_UNIT = BeatUnit::Two;
constexpr auto  TEMPO_TRACK_MAX_BEAT_UNIT = BeatUnit::Sixteen;

/**
 * @brief Represents a track that controls the tempo of the audio.
 *
 * The TempoTrack class is responsible for managing the tempo of the audio in a
 * project. It provides methods to set the BPM (beats per minute), beats per
 * bar, and beat unit. The tempo can be automated using the provided ports.
 */
class TempoTrack final
    : public QObject,
      public AutomatableTrack,
      public utils::InitializableObject<TempoTrack>
{
  Q_OBJECT
  QML_ELEMENT
  DEFINE_TRACK_QML_PROPERTIES (TempoTrack)
  DEFINE_AUTOMATABLE_TRACK_QML_PROPERTIES (TempoTrack)
  Q_PROPERTY (double bpm READ getBpm WRITE setBpm NOTIFY bpmChanged FINAL)

  friend class InitializableObject;

  DECLARE_FINAL_TRACK_CONSTRUCTORS (TempoTrack)

public:
  // ==================================================================
  // QML Interface
  // ==================================================================

  double          getBpm () const;
  void            setBpm (double bpm);
  Q_SIGNAL void   bpmChanged (double bpm);
  Q_INVOKABLE int getBeatUnit () const;
  Q_INVOKABLE int getBeatsPerBar () const;

  // ==================================================================

  /**
   * Removes all objects from the tempo track.
   *
   * Mainly used in testing.
   */
  void clear_objects () override;

  void
  init_loaded (PluginRegistry &plugin_registry, PortRegistry &port_registry)
    override;

  /**
   * Returns the BPM at the given pos.
   */
  bpm_t get_bpm_at_pos (Position pos);

  friend void init_from (
    TempoTrack            &obj,
    const TempoTrack      &other,
    utils::ObjectCloneType clone_type);

  /**
   * Returns the current BPM.
   */
  bpm_t get_current_bpm () const;

  std::string get_current_bpm_as_str () const;
  void        set_bpm_from_str (const std::string &str);

  /**
   * Sets the BPM.
   *
   * @param update_snap_points Whether to update the
   *   snap points.
   * @param stretch_audio_region Whether to stretch
   *   audio regions. This should only be true when
   *   the BPM change is final.
   * @param start_bpm The BPM at the start of the
   *   action, if not temporary.
   */
  void set_bpm (bpm_t bpm, bpm_t start_bpm, bool temporary, bool fire_events);

  static constexpr int beat_unit_enum_to_int (BeatUnit ebeat_unit)
  {
    switch (ebeat_unit)
      {
      case BeatUnit::Two:
        return 2;
      case BeatUnit::Four:
        return 4;
      case BeatUnit::Eight:
        return 8;
      case BeatUnit::Sixteen:
        return 16;
      default:
        z_return_val_if_reached (0);
      }
  }

  void set_beat_unit_from_enum (BeatUnit ebeat_unit);

  BeatUnit get_beat_unit_enum () const
  {
    return beat_unit_to_enum (get_beat_unit ());
  }

  static BeatUnit beat_unit_to_enum (int beat_unit);

  void set_beat_unit (int beat_unit);

  /**
   * Updates beat unit and anything depending on it.
   */
  void set_beats_per_bar (int beats_per_bar);

  int get_beats_per_bar () const;

  int get_beat_unit () const;

  void on_control_change_event (
    const PortUuid                          &port_uuid,
    const structure::tracks::PortIdentifier &id,
    float                                    value) override;

  void
  append_ports (std::vector<Port *> &ports, bool include_plugins) const final;

  ControlPort &get_bpm_port () const
  {
    return *std::get<ControlPort *> (bpm_port_->get_object ());
  }
  ControlPort &get_beats_per_bar_port () const
  {
    return *std::get<ControlPort *> (beats_per_bar_port_->get_object ());
  }
  ControlPort &get_beat_unit_port () const
  {
    return *std::get<ControlPort *> (beat_unit_port_->get_object ());
  }

private:
  static constexpr auto kBpmPortKey = "bpmPort"sv;
  static constexpr auto kBeatsPerBarPortKey = "beatsPerBarPort"sv;
  static constexpr auto kBeatUnitPortKey = "beatUnitPort"sv;
  friend void           to_json (nlohmann::json &j, const TempoTrack &track)
  {
    to_json (j, static_cast<const Track &> (track));
    to_json (j, static_cast<const AutomatableTrack &> (track));
    j[kBpmPortKey] = track.bpm_port_;
    j[kBeatsPerBarPortKey] = track.beats_per_bar_port_;
    j[kBeatUnitPortKey] = track.beat_unit_port_;
  }
  friend void from_json (const nlohmann::json &j, TempoTrack &track)
  {
    from_json (j, static_cast<Track &> (track));
    from_json (j, static_cast<AutomatableTrack &> (track));
    j.at (kBpmPortKey).get_to (track.bpm_port_);
    j.at (kBeatsPerBarPortKey).get_to (track.beats_per_bar_port_);
    j.at (kBeatUnitPortKey).get_to (track.beat_unit_port_);
  }

  bool initialize ();

private:
  PortRegistry &port_registry_;

  /** Automatable BPM control. */
  std::optional<PortUuidReference> bpm_port_;

  /** Automatable beats per bar port. */
  std::optional<PortUuidReference> beats_per_bar_port_;

  /** Automatable beat unit port. */
  std::optional<PortUuidReference> beat_unit_port_;
};
}
