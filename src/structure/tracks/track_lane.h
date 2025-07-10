// SPDX-FileCopyrightText: © 2019-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "dsp/midi_event.h"
#include "dsp/position.h"
#include "structure/arrangement/arranger_object_all.h"
#include "structure/arrangement/arranger_object_owner.h"

using MIDI_FILE = void;

#define DEFINE_TRACK_LANE_QML_PROPERTIES(ClassType, RegionType) \
  DEFINE_ARRANGER_OBJECT_OWNER_QML_PROPERTIES (ClassType, regions, RegionType) \
public: \
  /* ================================================================ */ \
  /* name */ \
  /* ================================================================ */ \
  Q_PROPERTY (QString name READ getName WRITE setName NOTIFY nameChanged) \
  QString getName () const \
  { \
    return name_.to_qstring (); \
  } \
  void setName (const QString &name) \
  { \
    const auto std_name = utils::Utf8String::from_qstring (name); \
    if (name_ == std_name) \
      return; \
\
    name_ = std_name; \
    Q_EMIT nameChanged (name); \
  } \
\
  Q_SIGNAL void nameChanged (const QString &name); \
  /* ================================================================ */ \
  /* height */ \
  /* ================================================================ */ \
  Q_PROPERTY ( \
    double height READ getHeight WRITE setHeight NOTIFY heightChanged) \
  double getHeight () const \
  { \
    return height_; \
  } \
  void setHeight (const double height) \
  { \
    if (utils::math::floats_equal (height_, height)) \
      return; \
\
    height_ = height; \
    Q_EMIT heightChanged (height); \
  } \
\
  Q_SIGNAL void heightChanged (double height);

namespace zrythm::structure::tracks
{
class MidiLane;
class AudioLane;
class Tracklist;

template <typename TrackLaneT> class LanedTrackImpl;

/**
 * A TrackLane belongs to a Track (can have many TrackLanes in a Track) and
 * contains Regions.
 *
 * Only Tracks that have Regions can have TrackLanes, such as InstrumentTrack
 * and AudioTrack.
 */
class TrackLane
{
public:
  static constexpr double DEF_HEIGHT = 48;

public:
  Q_DISABLE_COPY_MOVE (TrackLane)

  auto get_name () const { return this->name_; }

  void generate_name (int pos);

  bool get_soloed () const { return solo_; }

  /**
   * @brief Returns if the lane is explicitly marked as muted.
   *
   * @note Doesn't check soloed state or track state.
   */
  bool get_muted () const { return this->mute_; }

protected:
  TrackLane () = default;

public:
  /** Position in the Track. */
  // int pos_ = 0;

  /** Name of lane, e.g. "Lane 1". */
  utils::Utf8String name_;

  /** Position of handle. */
  double height_{ DEF_HEIGHT };

  /** Muted or not. */
  bool mute_{};

  /** Soloed or not. */
  bool solo_{};

  /**
   * MIDI channel, if MIDI lane, starting at 1.
   *
   * If this is set to 0, the value will be inherited from the Track.
   */
  uint8_t midi_ch_ = 0;
};

/**
 * A TrackLane belongs to a Track (can have many TrackLanes in a Track) and
 * contains Regions.
 *
 * Only Tracks that have Regions can have TrackLanes, such as InstrumentTrack
 * and AudioTrack.
 */
template <arrangement::RegionObject RegionT>
class TrackLaneImpl
    : public TrackLane,
      public arrangement::ArrangerObjectOwner<RegionT>
{
public:
  using MidiRegion = arrangement::MidiRegion;
  using AudioRegion = arrangement::AudioRegion;
  using TrackLaneT =
    std::conditional_t<std::is_same_v<RegionT, MidiRegion>, MidiLane, AudioLane>;
  using LanedTrackT = LanedTrackImpl<TrackLaneT>;
  using Position = dsp::Position;

public:
  // TrackLaneImpl () = default;
  ~TrackLaneImpl () override = default;
  Q_DISABLE_COPY_MOVE (TrackLaneImpl)

  TrackLaneT &get_derived_lane () { return *static_cast<TrackLaneT *> (this); }
  const TrackLaneT &get_derived_lane () const
  {
    return *static_cast<const TrackLaneT *> (this);
  }

  /**
   * Creates a new TrackLane at the given pos in the given Track.
   *
   * @param track The Track to create the TrackLane for.
   */
  TrackLaneImpl (
    structure::arrangement::ArrangerObjectRegistry &registry,
    dsp::FileAudioSourceRegistry                   &file_audio_source_registry,
    LanedTrackT *                                   track,
    QObject                                        &derived)
      : arrangement::ArrangerObjectOwner<
          RegionT> (registry, file_audio_source_registry, derived),
        track_ (track)
  {
  }

  bool is_auditioner () const;

  /**
   * Sets track lane soloed, updates UI and optionally
   * adds the action to the undo stack.
   *
   * @param trigger_undo Create and perform an
   *   undoable action.
   * @param fire_events Fire UI events.
   */
  void set_soloed (bool solo, bool trigger_undo, bool fire_events);

  /**
   * Sets track lane muted, updates UI and optionally
   * adds the action to the undo stack.
   *
   * @param trigger_undo Create and perform an
   *   undoable action.
   * @param fire_events Fire UI events.
   */
  void set_muted (bool mute, bool trigger_undo, bool fire_events);

  /**
   * @brief Returns if the lane is effectively muted (explicitly or implicitly
   * muted).
   */
  bool is_effectively_muted () const;

  /**
   * Rename the lane.
   *
   * @param with_action Whether to make this an
   *   undoable action.
   */
  void rename (const utils::Utf8String &new_name, bool with_action);

  /**
   * Wrapper over track_lane_rename().
   */
  void rename_with_action (const utils::Utf8String &new_name);

  /**
   * Unselects all arranger objects.
   */
  void unselect_all ();

  /**
   * Writes the lane to the given MIDI file.
   *
   * @param lanes_as_tracks Export lanes as separate MIDI tracks.
   * @param use_track_or_lane_pos Whether to use the track position (or lane
   * position if @ref lanes_as_tracks is true) in the MIDI data. The MIDI track
   * will be set to 1 if false.
   * @param events Track events, if not using lanes as tracks.
   * @param start Events before this position will be skipped.
   * @param end Events after this position will be skipped.
   */
  void write_to_midi_file (
    MIDI_FILE *            mf,
    dsp::MidiEventVector * events,
    const Position *       start,
    const Position *       end,
    bool                   lanes_as_tracks,
    bool                   use_track_or_lane_pos)
    requires std::derived_from<MidiRegion, RegionT>;

  Tracklist * get_tracklist () const;

  LanedTrackT * get_track () const
  {
    z_return_val_if_fail (track_, nullptr);
    return track_;
  }

  int get_index_in_track (this const auto &self)
  {
    return self.get_derived_lane ().get_track ()->get_lane_index (
      self.get_derived_lane ());
  }

  /**
   * Calculates a unique index for this lane.
   */
  int calculate_lane_idx_for_midi_serialization () const;

  /**
   * Generate a snapshot for playback.
   */
  std::unique_ptr<TrackLaneT> gen_snapshot () const;

  std::string get_field_name_for_serialization (const RegionT *) const override
  {
    return "regions";
  }

protected:
  friend void init_from (
    TrackLaneImpl         &obj,
    const TrackLaneImpl   &other,
    utils::ObjectCloneType clone_type)
  {
    obj.name_ = other.name_;
    // y_ = other.y_;
    obj.height_ = other.height_;
    obj.mute_ = other.mute_;
    obj.solo_ = other.solo_;
  }

private:
  static constexpr std::string_view kNameKey = "name";
  static constexpr std::string_view kHeightKey = "height";
  static constexpr std::string_view kMuteKey = "mute";
  static constexpr std::string_view kSoloKey = "solo";
  static constexpr std::string_view kMidiChannelKey = "midiChannel";
  friend void to_json (nlohmann::json &j, const TrackLaneImpl &lane)
  {
    to_json (
      j, static_cast<const arrangement::ArrangerObjectOwner<RegionT> &> (lane));
    j[kNameKey] = lane.name_;
    j[kHeightKey] = lane.height_;
    j[kMuteKey] = lane.mute_;
    j[kSoloKey] = lane.solo_;
    j[kMidiChannelKey] = lane.midi_ch_;
  }
  friend void from_json (const nlohmann::json &j, TrackLaneImpl &lane)
  {
    from_json (
      j, static_cast<arrangement::ArrangerObjectOwner<RegionT> &> (lane));
    j.at (kNameKey).get_to (lane.name_);
    j.at (kHeightKey).get_to (lane.height_);
    j.at (kMuteKey).get_to (lane.mute_);
    j.at (kSoloKey).get_to (lane.solo_);
    j.at (kMidiChannelKey).get_to (lane.midi_ch_);
  }

  // TODO make sure this gets called when regions are removed from
  // ArrangerObjectOwner
  void after_remove_region ();

public:
  /** Owner track. */
  LanedTrackT * track_ = nullptr;
};

using TrackLaneVariant = std::variant<MidiLane, AudioLane>;
using TrackLanePtrVariant = to_pointer_variant<TrackLaneVariant>;

template <typename T>
concept TrackLaneSubclass = std::derived_from<T, TrackLane>;

extern template class TrackLaneImpl<arrangement::MidiRegion>;
extern template class TrackLaneImpl<arrangement::AudioRegion>;
}
