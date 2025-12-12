// SPDX-FileCopyrightText: © 2018-2022, 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "dsp/modulator_macro_processor.h"
#include "structure/tracks/automation_tracklist.h"
#include "structure/tracks/channel.h"
#include "structure/tracks/piano_roll_track.h"
#include "structure/tracks/recordable_track.h"
#include "structure/tracks/track_fwd.h"
#include "structure/tracks/track_lane_list.h"
#include "structure/tracks/track_processor.h"
#include "utils/format.h"
#include "utils/playback_cache_scheduler.h"

#include <QColor>
#include <QtQmlIntegration/qqmlintegration.h>

namespace zrythm::dsp
{
class MidiPlaybackCache;
class TempoMapWrapper;
}

namespace zrythm::structure::tracks
{
using SoloedTracksExistGetter = GenericBoolGetter;

struct BaseTrackDependencies
{
  const dsp::TempoMapWrapper          &tempo_map_;
  dsp::FileAudioSourceRegistry        &file_audio_source_registry_;
  plugins::PluginRegistry             &plugin_registry_;
  dsp::PortRegistry                   &port_registry_;
  dsp::ProcessorParameterRegistry     &param_registry_;
  arrangement::ArrangerObjectRegistry &obj_registry_;
  const dsp::ITransport               &transport_;
  SoloedTracksExistGetter              soloed_tracks_exist_getter_;
};

/**
 * @brief Represents a track in the project.
 *
 * The `Track` class is the base class for all types of tracks in the
 * project. It provides common functionality and properties shared by
 * all track types, such as the track's position in the tracklist, its label,
 * and whether it is muted.
 *
 * Subclasses of `Track` represent specific types of tracks, such as
 * MIDI tracks, instrument tracks, and audio tracks.
 */
class Track : public QObject, public utils::UuidIdentifiableObject<Track>
{
  Q_OBJECT
  Q_PROPERTY (Type type READ type CONSTANT)
  Q_PROPERTY (
    zrythm::structure::tracks::AutomationTracklist * automationTracklist READ
      automationTracklist CONSTANT)
  Q_PROPERTY (QString name READ name WRITE setName NOTIFY nameChanged)
  Q_PROPERTY (QColor color READ color WRITE setColor NOTIFY colorChanged)
  Q_PROPERTY (
    QString comment READ comment WRITE setComment NOTIFY commentChanged)
  Q_PROPERTY (QString icon READ icon WRITE setIcon NOTIFY iconChanged)
  Q_PROPERTY (bool visible READ visible WRITE setVisible NOTIFY visibleChanged)
  Q_PROPERTY (bool enabled READ enabled WRITE setEnabled NOTIFY enabledChanged)
  Q_PROPERTY (double height READ height WRITE setHeight NOTIFY heightChanged)
  Q_PROPERTY (
    double fullVisibleHeight READ fullVisibleHeight NOTIFY
      fullVisibleHeightChanged)
  Q_PROPERTY (zrythm::structure::tracks::Channel * channel READ channel CONSTANT)
  Q_PROPERTY (zrythm::plugins::PluginGroup * modulators READ modulators CONSTANT)
  Q_PROPERTY (
    zrythm::structure::tracks::TrackLaneList * lanes READ lanes CONSTANT)
  Q_PROPERTY (
    zrythm::structure::tracks::RecordableTrackMixin * recordableTrackMixin READ
      recordableTrackMixin CONSTANT)
  Q_PROPERTY (
    zrythm::structure::tracks::PianoRollTrackMixin * pianoRollTrackMixin READ
      pianoRollTrackMixin CONSTANT)
  Q_PROPERTY (
    bool clipLauncherMode READ clipLauncherMode WRITE setClipLauncherMode NOTIFY
      clipLauncherModeChanged)
  QML_ELEMENT
  QML_UNCREATABLE ("")
public:
  using Plugin = plugins::Plugin;
  using PluginUuid = Plugin::Uuid;
  using PortType = dsp::PortType;
  using PluginRegistry = plugins::PluginRegistry;
  using PluginPtrVariant = PluginRegistry::VariantType;
  using PluginSlot = plugins::PluginSlot;
  using ArrangerObject = structure::arrangement::ArrangerObject;
  using ArrangerObjectPtrVariant =
    structure::arrangement::ArrangerObjectPtrVariant;
  using ArrangerObjectRegistry = structure::arrangement::ArrangerObjectRegistry;
  using Color = utils::Color;

  enum class Type : basic_enum_base_type_t
  {
    /**
     * Instrument tracks must have an Instrument plugin at the first slot and
     * they produce audio output.
     */
    Instrument,

    /**
     * Audio tracks can record and contain audio clips. Other than that their
     * channel strips are similar to buses.
     */
    Audio,

    /**
     * The master track is a special type of group track.
     */
    Master,

    /**
     * The chord track contains chords that can be used to modify midi in real
     * time or to color the piano roll.
     */
    Chord,

    /**
     * Marker Track's contain named markers at specific Position's in the song.
     */
    Marker,

    /**
     * Special track to contain global Modulator's.
     */
    Modulator,

    /**
     * Buses are channels that receive audio input and have effects on their
     * channel strip. They are similar to Group Tracks, except that they
     * cannot be routed to directly. Buses are used for send effects.
     */
    AudioBus,

    /**
     * Group Tracks are used for grouping audio signals, for example routing
     * multiple drum tracks to a "Drums" group track. Like buses, they only
     * contain effects but unlike buses they can be routed to.
     */
    AudioGroup,

    /**
     * Midi tracks can only have MIDI effects in the strip and produce MIDI
     * output that can be routed to instrument channels or hardware.
     */
    Midi,

    /** Same with audio bus but for MIDI signals. */
    MidiBus,

    /** Same with audio group but for MIDI signals. */
    MidiGroup,

    /** Foldable track used for visual grouping. */
    Folder,
  };
  Q_ENUM (Type)

  static constexpr int MIN_HEIGHT = 26;
  static constexpr int DEF_HEIGHT = 52;

  static bool
  type_can_have_region_type (Type type, ArrangerObject::Type region_type)
  {
    switch (region_type)
      {
      case ArrangerObject::Type::AudioRegion:
        return type == Type::Audio;
      case ArrangerObject::Type::MidiRegion:
        return type == Type::Midi || type == Type::Instrument;
      case ArrangerObject::Type::ChordRegion:
        return type == Type::Chord;
      case ArrangerObject::Type::AutomationRegion:
        return true;
      default:
        throw std::runtime_error ("Invalid region type");
      }
  }

  static constexpr bool type_is_foldable (Type type)
  {
    return type == Type::Folder || type == Type::AudioGroup
           || type == Type::MidiGroup;
  }

  static constexpr bool type_is_copyable (Type type)
  {
    return type != Type::Master && type != Type::Chord
           && type != Type::Modulator && type != Type::Marker;
  }

  /**
   * Returns whether a track of the given type should be deletable by the user.
   */
  static constexpr bool type_is_deletable (Type type)
  {
    return type_is_copyable (type);
  }

  static Type type_get_from_plugin_descriptor (
    const zrythm::plugins::PluginDescriptor &descr);

  static consteval bool type_has_mono_compat_switch (const Type tt)
  {
    return tt == Type::AudioGroup || tt == Type::Master;
  }

  /**
   * Returns if regions in tracks from @p type1 can be moved to @p type2.
   */
  static constexpr bool
  type_is_compatible_for_moving (const Type type1, const Type type2)
  {
    return type1 == type2 || (type1 == Type::Midi && type2 == Type::Instrument)
           || (type1 == Type::Instrument && type2 == Type::Midi);
  }

  /**
   * Returns if the Track should have a piano roll.
   */
  [[gnu::const]]
  static constexpr bool type_has_piano_roll (const Type type)
  {
    return type == Type::Midi || type == Type::Instrument;
  }

  /**
   * Returns if the Track should have an inputs selector.
   */
  static constexpr bool type_has_inputs (const Type type)
  {
    return type == Type::Midi || type == Type::Instrument || type == Type::Audio;
  }

  /**
   * @brief Returns if the Track can be a direct route target.
   *
   * @param type
   * @return true
   * @return false
   */
  static constexpr bool type_can_be_group_target (const Type type)
  {
    return type == Type::AudioGroup || type == Type::MidiGroup
           || type == Type::Instrument || type == Type::Master;
  }

  static constexpr bool type_can_have_automation (const Type type)
  {
    return type == Type::Instrument || type == Type::Audio
           || type == Type::Master || type == Type::Chord
           || type == Type::Modulator || type == Type::AudioBus
           || type == Type::AudioGroup || type == Type::Midi
           || type == Type::MidiBus || type == Type::MidiGroup;
  }

  static constexpr bool type_can_have_lanes (const Type type)
  {
    return type == Type::Instrument || type == Type::Audio || type == Type::Midi;
  }

  template <typename T> static consteval Type get_type_for_class ()
  {
    if constexpr (std::is_same_v<T, MidiTrack>)
      return Type::Midi;
    else if constexpr (std::is_same_v<T, AudioTrack>)
      return Type::Audio;
    else if constexpr (std::is_same_v<T, ChordTrack>)
      return Type::Chord;
    else if constexpr (std::is_same_v<T, InstrumentTrack>)
      return Type::Instrument;
    else if constexpr (std::is_same_v<T, AudioBusTrack>)
      return Type::AudioBus;
    else if constexpr (std::is_same_v<T, MidiBusTrack>)
      return Type::MidiBus;
    else if constexpr (std::is_same_v<T, MasterTrack>)
      return Type::Master;
    else if constexpr (std::is_same_v<T, ModulatorTrack>)
      return Type::Modulator;
    else if constexpr (std::is_same_v<T, MarkerTrack>)
      return Type::Marker;
    else if constexpr (std::is_same_v<T, FolderTrack>)
      return Type::Folder;
    else if constexpr (std::is_same_v<T, AudioGroupTrack>)
      return Type::AudioGroup;
    else if constexpr (std::is_same_v<T, MidiGroupTrack>)
      return Type::MidiGroup;
    else
      {
        static_assert (dependent_false_v<T>, "Unknown track type");
      }
  }

public:
  ~Track () override;
  Z_DISABLE_COPY_MOVE (Track)

protected:
  enum class TrackFeatures : std::uint8_t
  {
    Modulators = 1 << 0,
    Automation = 1 << 1,
    Lanes = 1 << 2,
    Recording = 1 << 3,
    PianoRoll = 1 << 4,
  };

  /**
   * Constructor to be used by subclasses.
   */
  Track (
    Type                  type,
    PortType              in_signal_type,
    PortType              out_signal_type,
    TrackFeatures         enabled_features,
    BaseTrackDependencies dependencies);

public:
  bool has_piano_roll () const { return type_has_piano_roll (type_); }

  bool is_folder () const { return type_ == Type::Folder; }
  bool is_audio_group () const { return type_ == Type::AudioGroup; }
  bool is_midi_group () const { return type_ == Type::MidiGroup; }
  bool is_audio_bus () const { return type_ == Type::AudioBus; }
  bool is_midi_bus () const { return type_ == Type::MidiBus; }
  bool is_modulator () const { return type_ == Type::Modulator; }
  bool is_chord () const { return type_ == Type::Chord; }
  bool is_marker () const { return type_ == Type::Marker; }
  bool is_audio () const { return type_ == Type::Audio; }
  bool is_instrument () const { return type_ == Type::Instrument; }
  bool is_midi () const { return type_ == Type::Midi; }
  bool is_master () const { return type_ == Type::Master; }

  // ========================================================================
  // QML Interface
  // ========================================================================

  Type type () const { return type_; }

  AutomationTracklist * automationTracklist () const
  {
    return automation_tracklist_.get ();
  }

  QString name () const { return name_.to_qstring (); }
  void    setName (const QString &name)
  {
    const auto name_str = utils::Utf8String::from_qstring (name);
    if (name_ == name_str)
      return;

    name_ = name_str;
    Q_EMIT nameChanged (name);
  }
  Q_SIGNAL void nameChanged (const QString &name);

  QColor color () const { return color_.to_qcolor (); }
  void   setColor (const QColor &color)
  {
    if (color_.to_qcolor () == color)
      return;

    color_ = color;
    Q_EMIT colorChanged (color);
  }
  Q_SIGNAL void colorChanged (const QColor &color);

  QString comment () const { return comment_.to_qstring (); }
  void    setComment (const QString &comment)
  {
    const auto comment_str = utils::Utf8String::from_qstring (comment);
    if (comment_ == comment_str)
      return;

    comment_ = comment_str;
    Q_EMIT commentChanged (comment);
  }
  Q_SIGNAL void commentChanged (const QString &comment);

  bool visible () const { return visible_; }
  void setVisible (bool visible)
  {
    if (visible_ == visible)
      return;

    visible_ = visible;
    Q_EMIT visibleChanged (visible);
  }
  Q_SIGNAL void visibleChanged (bool visible);

  bool enabled () const { return enabled_; }
  void setEnabled (bool enabled)
  {
    if (enabled_ == enabled)
      return;
    enabled_ = enabled;
    Q_EMIT enabledChanged (enabled);
  }
  Q_SIGNAL void enabledChanged (bool enabled);

  double height () const { return main_height_; }
  void   setHeight (double height)
  {
    if (utils::values_equal_for_qproperty_type (height, main_height_))
      return;

    height = std::max (static_cast<double> (Track::MIN_HEIGHT), height);
    main_height_ = height;
    Q_EMIT heightChanged (height);
  }
  Q_SIGNAL void heightChanged (double height);

  double fullVisibleHeight () const { return get_full_visible_height (); }
  Q_SIGNAL void fullVisibleHeightChanged ();

  QString icon () const { return icon_name_.to_qstring (); }
  void    setIcon (const QString &icon)
  {
    const auto icon_str = utils::Utf8String::from_qstring (icon);
    if (icon_name_ == icon_str)
      {
        return;
      }
    icon_name_ = icon_str;
    Q_EMIT iconChanged (icon);
  }
  Q_SIGNAL void iconChanged (const QString &icon);

  Channel * channel () const { return channel_.get (); }

  plugins::PluginGroup * modulators () const { return modulators_.get (); }

  TrackLaneList * lanes () const { return lanes_.get (); }

  RecordableTrackMixin * recordableTrackMixin () const
  {
    return recordable_track_mixin_.get ();
  }

  PianoRollTrackMixin * pianoRollTrackMixin () const
  {
    return piano_roll_track_mixin_.get ();
  }

  bool          clipLauncherMode () const { return clip_launcher_mode_; }
  void          setClipLauncherMode (bool mode);
  Q_SIGNAL void clipLauncherModeChanged (bool mode);

  /**
   * @brief To be connected to to notify of any changes to the playable content,
   * like MIDI or audio events.
   *
   * This will request new caches to be generated for the track processor.
   */
  Q_INVOKABLE void
  regeneratePlaybackCaches (utils::ExpandableTickRange affectedRange);

  // ========================================================================

  // bool has_lanes () const { return type_has_lanes (type_); }

  bool is_deletable () const { return type_is_deletable (type_); }
  bool is_copyable () const { return type_is_copyable (type_); }
  bool has_automation () const { return automationTracklist () != nullptr; }

  /**
   * Returns the full visible height (main height + height of all visible
   * automation tracks + height of all visible lanes).
   */
  double get_full_visible_height () const;

  bool multiply_heights (double multiplier, bool visible_only, bool check_only);

  bool can_be_group_target () const { return type_can_be_group_target (type_); }

  template <arrangement::RegionObject RegionT>
  auto generate_name_for_region (
    const RegionT    &region,
    AutomationTrack * automation_track = nullptr)
  {
    auto ret = get_name ();
    if constexpr (std::is_same_v<RegionT, arrangement::AutomationRegion>)
      {
        assert (automation_track != nullptr);
        ret = utils::Utf8String::from_utf8_encoded_string (
          fmt::format (
            "{} - {}", get_name (), automation_track->parameter ()->label ()));
      }
    return ret;
  }

  /**
   * @brief Appends all the timeine objects in the track to @p objects.
   */
  void collect_timeline_objects (
    std::vector<ArrangerObjectPtrVariant> &objects) const;

  bool contains_uninstantiated_plugin () const;

  /**
   * Getter for the track name.
   */
  utils::Utf8String get_name () const { return name_; };

  auto get_input_signal_type () const { return in_signal_type_; }
  auto get_output_signal_type () const { return out_signal_type_; }

  /**
   * Returns the MIDI channel that this region should be played on, starting
   * from 1.
   */
  uint8_t get_midi_ch (const arrangement::MidiRegion &midi_region) const;

  /**
   * Fills in the given array with all plugins in the track.
   */
  void collect_plugins (std::vector<plugins::PluginPtrVariant> &plugins) const;

  /**
   * Returns if @p descr can be dropped at @p slot_type in a track of type @p
   * track_type.
   */
  static bool is_plugin_descriptor_valid_for_slot_type (
    const plugins::PluginDescriptor &descr,
    zrythm::plugins::PluginSlotType  slot_type,
    Track::Type                      track_type);

  /**
   * Set various caches (snapshots, track name hash, plugin input/output
   * ports, etc).
   */
  void set_caches (CacheType types);

  utils::Utf8String get_full_designation_for_port (const dsp::Port &port) const;

  /**
   * @brief Adds basic automation tracks.
   *
   * This only adds automation tracks for a few commonly used parameters. When
   * the user selects an automatable parameter to automate that does not have
   * automation tracks yet, automation tracks should be created lazily.
   */
  void generate_basic_automation_tracks ();

  auto &get_plugin_registry () const
  {
    return base_dependencies_.plugin_registry_;
  }
  auto &get_plugin_registry () { return base_dependencies_.plugin_registry_; }
  auto &get_port_registry () const { return base_dependencies_.port_registry_; }
  auto &get_port_registry () { return base_dependencies_.port_registry_; }
  auto &get_param_registry () const
  {
    return base_dependencies_.param_registry_;
  }
  auto &get_param_registry () { return base_dependencies_.param_registry_; }
  auto &get_object_registry () const
  {
    return base_dependencies_.obj_registry_;
  }
  auto &get_object_registry () { return base_dependencies_.obj_registry_; }

  TrackProcessor * get_track_processor () const { return processor_.get (); }

  auto get_icon_name () const { return icon_name_; }

protected:
  friend void
  init_from (Track &obj, const Track &other, utils::ObjectCloneType clone_type);

  /**
   * @brief Set the playback caches for a track.
   *
   * This is called by @ref set_caches().
   */
  virtual void set_playback_caches () { }

  void generate_automation_tracks_for_processor (
    std::vector<utils::QObjectUniquePtr<AutomationTrack>> &ats,
    const dsp::ProcessorBase                              &processor)
  {
    structure::tracks::generate_automation_tracks_for_processor (
      ats, processor, base_dependencies_.tempo_map_,
      base_dependencies_.file_audio_source_registry_,
      base_dependencies_.obj_registry_);
  }

  /**
   * @brief Implementations with a processor must call this in their constructor.
   */
  [[nodiscard]] utils::QObjectUniquePtr<TrackProcessor> make_track_processor (
    std::optional<TrackProcessor::FillEventsCallback> fill_events_cb =
      std::nullopt,
    std::optional<TrackProcessor::TransformMidiInputsFunc>
      transform_midi_inputs_func = std::nullopt,
    std::optional<TrackProcessor::AppendMidiInputsToOutputsFunc>
      append_midi_inputs_to_outputs_func = std::nullopt);

  /**
   * @brief Called by @ref collect_timeline_objects to collect any additional
   * objects not handled by this class (such as markers and scales).
   */
  virtual void collect_additional_timeline_objects (
    std::vector<ArrangerObjectPtrVariant> &objects) const
  {
  }

private:
  static constexpr auto kTypeKey = "type"sv;
  static constexpr auto kNameKey = "name"sv;
  static constexpr auto kIconNameKey = "iconName"sv;
  static constexpr auto kVisibleKey = "visible"sv;
  static constexpr auto kMainHeightKey = "mainHeight"sv;
  static constexpr auto kEnabledKey = "enabled"sv;
  static constexpr auto kColorKey = "color"sv;
  static constexpr auto kInputSignalTypeKey = "inSignalType"sv;
  static constexpr auto kOutputSignalTypeKey = "outSignalType"sv;
  static constexpr auto kCommentKey = "comment"sv;
  static constexpr auto kFrozenClipIdKey = "frozenClipId"sv;
  static constexpr auto kProcessorKey = "processor"sv;
  static constexpr auto kAutomationTracklistKey = "automationTracklist"sv;
  static constexpr auto kChannelKey = "channel"sv;
  static constexpr auto kModulatorsKey = "modulators"sv;
  static constexpr auto kModulatorMacroProcessorsKey = "modulatorMacros"sv;
  static constexpr auto kTrackLanesKey = "lanes"sv;
  static constexpr auto kRecordableTrackMixinKey = "recordableTrackMixin"sv;
  static constexpr auto kPianoRollTrackMixinKey = "pianoRollTrackMixin"sv;
  friend void           to_json (nlohmann::json &j, const Track &track)
  {
    to_json (j, static_cast<const UuidIdentifiableObject &> (track));
    j[kTypeKey] = track.type_;
    j[kNameKey] = track.name_;
    j[kIconNameKey] = track.icon_name_;
    j[kVisibleKey] = track.visible_;
    j[kMainHeightKey] = track.main_height_;
    j[kEnabledKey] = track.enabled_;
    j[kColorKey] = track.color_;
    j[kInputSignalTypeKey] = track.in_signal_type_;
    j[kOutputSignalTypeKey] = track.out_signal_type_;
    j[kCommentKey] = track.comment_;
    j[kFrozenClipIdKey] = track.frozen_clip_id_;
    j[kProcessorKey] = track.processor_;
    j[kAutomationTracklistKey] = track.automation_tracklist_;
    j[kChannelKey] = track.channel_;
    j[kModulatorsKey] = track.modulators_;
    j[kModulatorMacroProcessorsKey] = track.modulator_macro_processors_;
    j[kTrackLanesKey] = track.lanes_;
    j[kRecordableTrackMixinKey] = track.recordable_track_mixin_;
    j[kPianoRollTrackMixinKey] = track.piano_roll_track_mixin_;
  }
  friend void from_json (const nlohmann::json &j, Track &track);

  [[nodiscard]] utils::QObjectUniquePtr<AutomationTracklist>
                                                 make_automation_tracklist ();
  [[nodiscard]] utils::QObjectUniquePtr<Channel> make_channel ();
  [[nodiscard]] utils::QObjectUniquePtr<plugins::PluginGroup>
                                                       make_modulators ();
  [[nodiscard]] utils::QObjectUniquePtr<TrackLaneList> make_lanes ();
  [[nodiscard]] utils::QObjectUniquePtr<RecordableTrackMixin>
  make_recordable_track_mixin ();
  [[nodiscard]] utils::QObjectUniquePtr<PianoRollTrackMixin>
  make_piano_roll_track_mixin ();

protected:
  BaseTrackDependencies base_dependencies_;

  /** The type of track this is. */
  Type type_{};

  TrackFeatures features_{};

  /** Track name, used in channel too. */
  utils::Utf8String name_;

  /** Icon name of the track. */
  utils::Utf8String icon_name_;

  /** Whole Track is visible or not. */
  bool visible_ = true;

  /** Height of the main part (without lanes). */
  double main_height_{ DEF_HEIGHT };

  /**
   * Active (enabled) or not.
   *
   * Disabled tracks should be ignored in routing.
   * Similar to Plugin.enabled (bypass).
   */
  bool enabled_ = true;

  /**
   * Track color.
   *
   * This is used in the channels as well.
   */
  Color color_;

  /**
   * The input signal type (eg audio bus tracks have audio input signals).
   */
  PortType in_signal_type_ = {};

  /**
   * The output signal type (eg midi tracks have MIDI output signals).
   */
  PortType out_signal_type_ = {};

  /** User comments. */
  utils::Utf8String comment_;

  /**
   * @brief Pool ID of the clip if track is frozen (unset if not frozen).
   *
   * Currently unused.
   */
  std::optional<dsp::FileAudioSourceUuidReference> frozen_clip_id_;

  /**
   * @brief Automation tracks, if track is automatable.
   */
  utils::QObjectUniquePtr<AutomationTracklist> automation_tracklist_;

  /**
   * The TrackProcessor, used for processing.
   *
   * This is the starting point when processing a Track. Tracks that want to be
   * part of the DSP graph as signal producers must have this.
   *
   * @see Channel for additional DSP graph functionality.
   */
  utils::QObjectUniquePtr<TrackProcessor> processor_;

  /**
   * @brief Channel for this track, if any.
   */
  utils::QObjectUniquePtr<Channel> channel_;

  /** Modulators. */
  utils::QObjectUniquePtr<plugins::PluginGroup> modulators_;

  /** Modulator macros. */
  std::vector<utils::QObjectUniquePtr<dsp::ModulatorMacroProcessor>>
    modulator_macro_processors_;

  utils::QObjectUniquePtr<TrackLaneList> lanes_;

  utils::QObjectUniquePtr<RecordableTrackMixin> recordable_track_mixin_;

  utils::QObjectUniquePtr<PianoRollTrackMixin> piano_roll_track_mixin_;

  /**
   * @brief Debouncer/scheduler of audio/MIDI cache requests.
   */
  utils::QObjectUniquePtr<utils::PlaybackCacheScheduler>
    playable_content_cache_request_debouncer_;

  bool clip_launcher_mode_{};

  BOOST_DESCRIBE_CLASS (
    Track,
    (utils::UuidIdentifiableObject<Track>),
    (),
    (type_,
     name_,
     visible_,
     main_height_,
     enabled_,
     color_,
     in_signal_type_,
     out_signal_type_,
     comment_,
     automation_tracklist_,
     processor_,
     channel_,
     modulators_,
     modulator_macro_processors_,
     lanes_,
     recordable_track_mixin_,
     piano_roll_track_mixin_,
     clip_launcher_mode_),
    ())
};

using TrackRegistry = utils::OwningObjectRegistry<TrackPtrVariant, Track>;
using TrackRegistryRef = std::reference_wrapper<TrackRegistry>;
using TrackUuidReference = utils::UuidReference<TrackRegistry>;

struct FinalTrackDependencies : public BaseTrackDependencies
{
  FinalTrackDependencies (
    const dsp::TempoMapWrapper          &tempo_map,
    dsp::FileAudioSourceRegistry        &file_audio_source_registry,
    plugins::PluginRegistry             &plugin_registry,
    dsp::PortRegistry                   &port_registry,
    dsp::ProcessorParameterRegistry     &param_registry,
    arrangement::ArrangerObjectRegistry &obj_registry,
    TrackRegistry                       &track_registry,
    const dsp::ITransport               &transport,
    SoloedTracksExistGetter              soloed_tracks_exist_getter)
      : BaseTrackDependencies (
          tempo_map,
          file_audio_source_registry,
          plugin_registry,
          port_registry,
          param_registry,
          obj_registry,
          transport,
          std::move (soloed_tracks_exist_getter)),
        track_registry_ (track_registry)
  {
  }

  TrackRegistry &track_registry_;

  BaseTrackDependencies to_base_dependencies ()
  {
    return static_cast<BaseTrackDependencies> (*this); // NOLINT
  }
};

} // namespace zrythm::structure::tracks

DEFINE_ENUM_FORMATTER (
  zrythm::structure::tracks::Track::Type,
  Track_Type,
  QT_TR_NOOP_UTF8 ("Instrument"),
  QT_TR_NOOP_UTF8 ("Audio"),
  QT_TR_NOOP_UTF8 ("Master"),
  QT_TR_NOOP_UTF8 ("Chord"),
  QT_TR_NOOP_UTF8 ("Marker"),
  QT_TR_NOOP_UTF8 ("Modulator"),
  QT_TR_NOOP_UTF8 ("Audio FX"),
  QT_TR_NOOP_UTF8 ("Audio Group"),
  QT_TR_NOOP_UTF8 ("MIDI"),
  QT_TR_NOOP_UTF8 ("MIDI FX"),
  QT_TR_NOOP_UTF8 ("MIDI Group"),
  QT_TR_NOOP_UTF8 ("Folder"));
