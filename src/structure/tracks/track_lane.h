// SPDX-FileCopyrightText: © 2019-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <utility>

#include "structure/arrangement/arranger_object_all.h"
#include "structure/arrangement/arranger_object_owner.h"

namespace zrythm::structure::tracks
{

/**
 * @brief A container of MIDI or Audio regions.
 *
 * Intended to be used as part of a TrackLaneList in a LanedTrackMixin as part
 * of a Track. In practice, only Instrument, Audio and MIDI tracks have lanes.
 *
 * For convenience, this handles all cases by including both MidiRegion and
 * AudioRegion objects. In practice, only one list will be used.
 */
class TrackLane
    : public QObject,
      public utils::UuidIdentifiableObject<TrackLane>,
      public arrangement::ArrangerObjectOwner<arrangement::MidiRegion>,
      public arrangement::ArrangerObjectOwner<arrangement::AudioRegion>
{
  Q_OBJECT
  DEFINE_ARRANGER_OBJECT_OWNER_QML_PROPERTIES (
    TrackLane,
    midiRegions,
    zrythm::structure::arrangement::MidiRegion)
  DEFINE_ARRANGER_OBJECT_OWNER_QML_PROPERTIES (
    TrackLane,
    audioRegions,
    zrythm::structure::arrangement::AudioRegion)
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
  /**
   * @brief Function to check if other soloed lanes exist in the owner.
   */
  using SoloedLanesExistFunc = GenericBoolGetter;

  static constexpr auto default_format_str = QT_TR_NOOP_UTF8 ("Lane {}");

  struct TrackLaneDependencies
  {
    structure::arrangement::ArrangerObjectRegistry &obj_registry_;
    dsp::FileAudioSourceRegistry                   &file_audio_source_registry_;
    SoloedLanesExistFunc                            soloed_lanes_exist_func_;
  };

  TrackLane (TrackLaneDependencies dependencies, QObject * parent = nullptr)
      : QObject (parent),
        arrangement::ArrangerObjectOwner<arrangement::MidiRegion> (
          dependencies.obj_registry_,
          dependencies.file_audio_source_registry_,
          *this),
        arrangement::ArrangerObjectOwner<arrangement::AudioRegion> (
          dependencies.obj_registry_,
          dependencies.file_audio_source_registry_,
          *this),
        soloed_lanes_exist_func_ (
          std::move (dependencies.soloed_lanes_exist_func_))
  {
  }
  Z_DISABLE_COPY_MOVE (TrackLane)
  ~TrackLane () override = default;

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
    if (utils::math::floats_equal (height_, height))
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
   */
  Q_INVOKABLE bool effectivelyMuted () const
  {
    if (muted ())
      return true;

    /* if lane is non-soloed while other soloed lanes exist, this should
     * be muted */
    if (soloed_lanes_exist_func_ () && !soloed ())
      return true;

    return false;
  }

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
    const arrangement::MidiRegion * _) const override
  {
    return "midiRegions";
  }
  std::string get_field_name_for_serialization (
    const arrangement::AudioRegion * _) const override
  {
    return "audioRegions";
  }

private:
  static constexpr std::string_view kNameKey = "name";
  static constexpr std::string_view kHeightKey = "height";
  static constexpr std::string_view kMuteKey = "mute";
  static constexpr std::string_view kSoloKey = "solo";
  static constexpr std::string_view kMidiChannelKey = "midiChannel";
  friend void to_json (nlohmann::json &j, const TrackLane &lane)
  {
    to_json (j, static_cast<const UuidIdentifiableObject &> (lane));
    to_json (
      j,
      static_cast<const arrangement::ArrangerObjectOwner<
        arrangement::MidiRegion> &> (lane));
    to_json (
      j,
      static_cast<const arrangement::ArrangerObjectOwner<
        arrangement::AudioRegion> &> (lane));
    j[kNameKey] = lane.name_;
    j[kHeightKey] = lane.height_;
    j[kMuteKey] = lane.mute_;
    j[kSoloKey] = lane.solo_;
    j[kMidiChannelKey] = lane.midi_ch_;
  }
  friend void from_json (const nlohmann::json &j, TrackLane &lane)
  {
    from_json (j, static_cast<UuidIdentifiableObject &> (lane));
    from_json (
      j,
      static_cast<arrangement::ArrangerObjectOwner<arrangement::MidiRegion> &> (
        lane));
    from_json (
      j,
      static_cast<arrangement::ArrangerObjectOwner<arrangement::AudioRegion> &> (
        lane));
    j.at (kNameKey).get_to (lane.name_);
    j.at (kHeightKey).get_to (lane.height_);
    j.at (kMuteKey).get_to (lane.mute_);
    j.at (kSoloKey).get_to (lane.solo_);
    j.at (kMidiChannelKey).get_to (lane.midi_ch_);
  }

  friend void init_from (
    TrackLane             &obj,
    const TrackLane       &other,
    utils::ObjectCloneType clone_type)
  {
    obj.name_ = other.name_;
    obj.height_ = other.height_;
    obj.mute_ = other.mute_;
    obj.solo_ = other.solo_;
    obj.midi_ch_ = other.midi_ch_;
  }

private:
  SoloedLanesExistFunc soloed_lanes_exist_func_;

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
}
