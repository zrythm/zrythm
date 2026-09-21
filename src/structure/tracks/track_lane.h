// SPDX-FileCopyrightText: © 2019-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <utility>

#include "dsp/timebase.h"
#include "structure/arrangement/arranger_object_all.h"
#include "structure/arrangement/arranger_object_owner.h"

namespace zrythm::structure::tracks
{
class TrackLaneList;

/**
 * @brief A container of MIDI or Audio clips.
 *
 * Intended to be used as part of a TrackLaneList in a LanedTrackMixin as part
 * of a Track. In practice, only Instrument, Audio and MIDI tracks have lanes.
 *
 * For convenience, this handles all cases by including both MidiClip and
 * AudioClip objects. In practice, only one list will be used.
 */
class TrackLane
    : public utils::UuidIdentifiableObject<TrackLane>,
      public arrangement::ArrangerObjectOwner<arrangement::MidiClip>,
      public arrangement::ArrangerObjectOwner<arrangement::AudioClip>
{
  Q_OBJECT
  DEFINE_ARRANGER_OBJECT_OWNER_QML_PROPERTIES (
    TrackLane,
    midiClips,
    zrythm::structure::arrangement::MidiClip)
  DEFINE_ARRANGER_OBJECT_OWNER_QML_PROPERTIES (
    TrackLane,
    audioClips,
    zrythm::structure::arrangement::AudioClip)
  Q_PROPERTY (QString name READ name WRITE setName NOTIFY nameChanged)
  Q_PROPERTY (double height READ height WRITE setHeight NOTIFY heightChanged)
  Q_PROPERTY (bool muted READ muted WRITE setMuted NOTIFY muteChanged)
  Q_PROPERTY (bool soloed READ soloed WRITE setSoloed NOTIFY soloChanged)
  Q_PROPERTY (
    std::uint8_t midiChannel READ midiChannel WRITE setMidiChannel NOTIFY
      midiChannelChanged)
  QML_ELEMENT
  QML_UNCREATABLE ("")

  static constexpr double DEFAULT_HEIGHT = 48;

public:
  static constexpr auto default_format_str = QT_TR_NOOP_UTF8 ("Lane {}");

  struct TrackLaneDependencies
  {
    utils::IObjectRegistry &registry_;
    dsp::TimebaseProvider * timebase_provider_ = nullptr;
  };

  TrackLane (TrackLaneDependencies dependencies, QObject * parent = nullptr);
  Q_DISABLE_COPY_MOVE (TrackLane)
  ~TrackLane () override;

  /**
   * @brief Replaces the lane's list-derived dependencies.
   *
   * Lanes created for deserialization start without a timebase source;
   * the owning TrackLaneList calls this when it attaches the lane.
   */
  void rewire_dependencies (const TrackLaneDependencies &dependencies)
  {
    timebase_provider_ = dependencies.timebase_provider_;
  }

  /**
   * @brief The list this lane belongs to, or null for a detached lane.
   *
   * The list assigns this when it attaches the lane and clears it when
   * it detaches it. The reference is weak: it becomes null when the
   * list is destroyed.
   */
  void            set_owner_list (TrackLaneList * list);
  TrackLaneList * owner_list () const;

  // ========================================================================
  // QML Interface
  // ========================================================================

  QString name () const { return name_.to_qstring (); }
  void    setName (const QString &name)
  {
    const auto std_name = utils::Utf8String::from_qstring (name);
    if (name_ == std_name)
      return;

    name_ = std_name;
    Q_EMIT nameChanged (name);
  }
  Q_SIGNAL void nameChanged (const QString &name);

  double height () const { return height_; }
  void   setHeight (const double height)
  {
    if (qFuzzyCompare (height_, height))
      return;

    height_ = height;
    Q_EMIT heightChanged (height);
  }
  Q_SIGNAL void heightChanged (double height);

  bool soloed () const { return solo_; }
  void setSoloed (bool solo)
  {
    if (solo_ == solo)
      return;

    solo_ = solo;
    Q_EMIT soloChanged (solo);
  }
  Q_SIGNAL void soloChanged (bool solo);

  bool muted () const { return mute_; }
  void setMuted (bool mute)
  {
    if (mute_ == mute)
      return;

    mute_ = mute;
    Q_EMIT muteChanged (mute);
  }
  Q_SIGNAL void muteChanged (bool mute);

  /**
   * @brief Returns if the lane is effectively muted (explicitly or implicitly
   * muted).
   *
   * A lane not attached to a list is never implicitly muted: it has no
   * sibling lanes whose solo state could mute it.
   */
  Q_INVOKABLE bool effectivelyMuted () const;

  std::uint8_t midiChannel () const { return midi_ch_; }
  void         setMidiChannel (std::uint8_t midi_ch)
  {
    if (midi_ch_ == midi_ch)
      return;

    midi_ch_ = midi_ch;
    Q_EMIT midiChannelChanged (midi_ch);
  }
  Q_SIGNAL void midiChannelChanged (std::uint8_t midi_ch);

  // ========================================================================

  /**
   * @brief Generates a default name for the lane at the given index.
   *
   * @param index
   */
  void generate_name (size_t index);

  /**
   * @brief Generate a snapshot for playback.
   *
   * TODO
   */
  // std::unique_ptr<TrackLaneT> gen_snapshot () const;

  std::string get_field_name_for_serialization (
    const arrangement::MidiClip * _) const override
  {
    return "midiClips";
  }
  std::string get_field_name_for_serialization (
    const arrangement::AudioClip * _) const override
  {
    return "audioClips";
  }

private:
  static constexpr std::string_view kNameKey = "name";
  static constexpr std::string_view kHeightKey = "height";
  static constexpr std::string_view kMuteKey = "mute";
  static constexpr std::string_view kSoloKey = "solo";
  static constexpr std::string_view kMidiChannelKey = "midiChannel";
  friend void to_json (nlohmann::json &j, const TrackLane &lane);
  friend void from_json (const nlohmann::json &j, TrackLane &lane);

  friend void init_from (
    TrackLane             &obj,
    const TrackLane       &other,
    utils::ObjectCloneType clone_type);

private:
  QPointer<dsp::TimebaseProvider> timebase_provider_;

  QPointer<TrackLaneList> owner_list_;

  /** Name of lane, e.g. "Lane 1". */
  utils::Utf8String name_;

  /** Position of handle. */
  double height_{ DEFAULT_HEIGHT };

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

  BOOST_DESCRIBE_CLASS (
    TrackLane,
    (utils::UuidIdentifiableObject<TrackLane>),
    (),
    (),
    (name_, height_, mute_, solo_, midi_ch_))
};

using TrackLaneUuidReference = utils::TypedUuidReference<TrackLane>;
}
