// SPDX-FileCopyrightText: © 2018-2022, 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "dsp/port_connections_manager.h"
#include "dsp/position.h"
#include "gui/dsp/plugin.h"
#include "structure/arrangement/arranger_object_all.h"
#include "structure/tracks/automation_track.h"
#include "structure/tracks/fader.h"
#include "structure/tracks/track_fwd.h"
#include "structure/tracks/track_lane.h"
#include "utils/format.h"

#include <QColor>
#include <QHash>
#include <QtQmlIntegration>

struct FileImportInfo;
class FileDescriptor;

#define DECLARE_FINAL_TRACK_CONSTRUCTORS(ClassType) \
  /* FIXME: make private */ \
public: \
  ClassType (FinalTrackDependencies dependencies);

#define DEFINE_TRACK_QML_PROPERTIES(ClassType) \
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
    const auto name_str = utils::Utf8String::from_qstring (name); \
    if (name_ == name_str) \
      return; \
\
    name_ = name_str; \
    Q_EMIT nameChanged (name); \
  } \
\
  Q_SIGNAL void nameChanged (const QString &name); \
\
  /* ================================================================ */ \
  /* color */ \
  /* ================================================================ */ \
  Q_PROPERTY (QColor color READ getColor WRITE setColor NOTIFY colorChanged) \
\
  QColor getColor () const \
  { \
    return color_.to_qcolor (); \
  } \
  void setColor (const QColor &color) \
  { \
    if (color_.to_qcolor () == color) \
      return; \
\
    color_ = color; \
    Q_EMIT colorChanged (color); \
  } \
\
  Q_SIGNAL void colorChanged (const QColor &color); \
\
  /* ================================================================ */ \
  /* comment */ \
  /* ================================================================ */ \
  Q_PROPERTY ( \
    QString comment READ getComment WRITE setComment NOTIFY commentChanged) \
\
  QString getComment () const \
  { \
    return comment_.to_qstring (); \
  } \
  void setComment (const QString &comment) \
  { \
    const auto comment_str = utils::Utf8String::from_qstring (comment); \
    if (comment_ == comment_str) \
      return; \
\
    comment_ = comment_str; \
    Q_EMIT commentChanged (comment); \
  } \
\
  Q_SIGNAL void commentChanged (const QString &comment); \
\
  /* ================================================================ */ \
  /* visible */ \
  /* ================================================================ */ \
  Q_PROPERTY ( \
    bool visible READ getVisible WRITE setVisible NOTIFY visibleChanged) \
  bool getVisible () const \
  { \
    return visible_; \
  } \
  void setVisible (bool visible) \
  { \
    if (visible_ == visible) \
      return; \
\
    visible_ = visible; \
    Q_EMIT visibleChanged (visible); \
  } \
\
  Q_SIGNAL void visibleChanged (bool visible); \
\
  /* ================================================================ */ \
  /* enabled */ \
  /* ================================================================ */ \
  Q_PROPERTY ( \
    bool enabled READ getEnabled WRITE setEnabled NOTIFY enabledChanged) \
  bool getEnabled () const \
  { \
    return enabled_; \
  } \
\
  void setEnabled (bool enabled) \
  { \
    set_enabled (enabled, true, true, true); \
  } \
  Q_SIGNAL void enabledChanged (bool enabled); \
\
  /* ================================================================ */ \
  /* selected */ \
  /* ================================================================ */ \
  Q_PROPERTY (bool selected READ getSelected NOTIFY selectedChanged) \
  bool getSelected () const \
  { \
    if (track_selection_status_getter_) \
      { \
        return (*track_selection_status_getter_) (get_uuid ()); \
      } \
    return false; \
  } \
  Q_SIGNAL void selectedChanged (bool selected); \
\
  /* ================================================================ */ \
  /* type */ \
  /* ================================================================ */ \
  Q_PROPERTY (int type READ getType CONSTANT) \
  int getType () const \
  { \
    return ENUM_VALUE_TO_INT (type_); \
  } \
  /* ================================================================ */ \
  /* isRecordable */ \
  /* ================================================================ */ \
  Q_PROPERTY (bool isRecordable READ getIsRecordable CONSTANT) \
  bool getIsRecordable () const \
  { \
    if constexpr (std::derived_from<ClassType, RecordableTrack>) \
      { \
        return true; \
      } \
    return false; \
  } \
\
  /* ================================================================ */ \
  /* hasLanes */ \
  /* ================================================================ */ \
  Q_PROPERTY (bool hasLanes READ getHasLanes CONSTANT) \
  bool getHasLanes () const \
  { \
    if constexpr (std::derived_from<ClassType, LanedTrack>) \
      { \
        return true; \
      } \
    return false; \
  } \
\
  /* ================================================================ */ \
  /* hasChannel */ \
  /* ================================================================ */ \
  Q_PROPERTY (bool hasChannel READ getHasChannel CONSTANT) \
  bool getHasChannel () const \
  { \
    if constexpr (std::derived_from<ClassType, ChannelTrack>) \
      { \
        return true; \
      } \
    return false; \
  } \
\
  /* ================================================================ */ \
  /* isAutomatable */ \
  /* ================================================================ */ \
  Q_PROPERTY (bool isAutomatable READ getIsAutomatable CONSTANT) \
  bool getIsAutomatable () const \
  { \
    if constexpr (AutomatableTrack<ClassType>) \
      { \
        return true; \
      } \
    return false; \
  } \
\
  /* ================================================================ */ \
  /* height */ \
  /* ================================================================ */ \
  Q_PROPERTY ( \
    double height READ getHeight WRITE setHeight NOTIFY heightChanged) \
  double getHeight () const \
  { \
    return main_height_; \
  } \
  void setHeight (double height) \
  { \
    if (utils::math::floats_equal (height, main_height_)) \
      { \
        return; \
      } \
    main_height_ = height; \
    Q_EMIT heightChanged (height); \
  } \
  Q_SIGNAL void heightChanged (double height); \
  Q_PROPERTY ( \
    double fullVisibleHeight READ getFullVisibleHeight NOTIFY \
      fullVisibleHeightChanged) \
  double getFullVisibleHeight () const \
  { \
    return get_full_visible_height (); \
  } \
  Q_SIGNAL void fullVisibleHeightChanged (); \
  /* ================================================================ */ \
  /* icon */ \
  /* ================================================================ */ \
  Q_PROPERTY (QString icon READ getIcon WRITE setIcon NOTIFY iconChanged) \
  QString getIcon () const \
  { \
    return icon_name_.to_qstring (); \
  } \
  void setIcon (const QString &icon) \
  { \
    const auto icon_str = utils::Utf8String::from_qstring (icon); \
    if (icon_name_ == icon_str) \
      { \
        return; \
      } \
    icon_name_ = icon_str; \
    Q_EMIT iconChanged (icon); \
  } \
  Q_SIGNAL void iconChanged (const QString &icon);

namespace zrythm::engine::session
{
class Transport;
}

// hack to make QML registration happy because it doesn't support Q_GADGET with
// pure virtual methods
class TrackTypeWrapper
{
  Q_GADGET

public:
  /**
   * The Track's type.
   */
  enum class TrackType : basic_enum_base_type_t
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
  Q_ENUM (TrackType)
};

namespace zrythm::structure::tracks
{
class FoldableTrack;
class ChannelTrack;
class GroupTargetTrack;
class TrackFactory;

/**
 * Called when track(s) are actually imported into the project.
 */
using TracksReadyCallback = void (*) (const FileImportInfo *);

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
class Track : public utils::UuidIdentifiableObject<Track>
{
public:
  using PluginUuid = gui::old_dsp::plugins::Plugin::Uuid;
  using PortType = dsp::PortType;
  using PluginRegistry = gui::old_dsp::plugins::PluginRegistry;
  using PluginPtrVariant = PluginRegistry::VariantType;
  using PluginSlot = plugins::PluginSlot;
  using ArrangerObject = structure::arrangement::ArrangerObject;
  using ArrangerObjectPtrVariant =
    structure::arrangement::ArrangerObjectPtrVariant;
  using ArrangerObjectRegistry = structure::arrangement::ArrangerObjectRegistry;
  using Type = ::TrackTypeWrapper::TrackType;
  using TrackSelectionStatusGetter = std::function<bool (const Track::Uuid &)>;

  static constexpr int MIN_HEIGHT = 26;
  static constexpr int DEF_HEIGHT = 52;

  friend class Tracklist;

  using Color = utils::Color;
  using Position = dsp::Position;

  /**
   * Returns the prefader type.
   */
  static Fader::Type type_get_fader_type (const Type type)
  {
    switch (type)
      {
      case Type::Midi:
      case Type::MidiBus:
      case Type::Chord:
      case Type::MidiGroup:
        return Fader::Type::MidiChannel;
      case Type::Instrument:
      case Type::Audio:
      case Type::AudioBus:
      case Type::Master:
      case Type::AudioGroup:
        return Fader::Type::AudioChannel;
      case Type::Marker:
      case Type::Folder:
        return Fader::Type::None;
      default:
        z_return_val_if_reached (Fader::Type::None);
      }
  }

  static constexpr bool type_can_have_direct_out (Type type)
  {
    return type != Type::Master;
  }

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
  static bool type_can_be_group_target (const Type type)
  {
    return type == Type::AudioGroup || type == Type::MidiGroup
           || type == Type::Instrument || type == Type::Master;
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
  /**
   * Constructor to be used by subclasses.
   *
   * @param pos Position in the Tracklist.
   */
  Track (
    Type                             type,
    PortType                         in_signal_type,
    PortType                         out_signal_type,
    PluginRegistry                  &plugin_registry,
    dsp::PortRegistry               &port_registry,
    dsp::ProcessorParameterRegistry &param_registry,
    ArrangerObjectRegistry          &obj_registry);

public:
  /**
   * @brief Adds additional metadata to track members after deserialization.
   *
   * @note Each implementor must chain up to its direct superclass.
   */
  virtual void init_loaded (
    PluginRegistry                  &plugin_registry,
    dsp::PortRegistry               &port_registry,
    dsp::ProcessorParameterRegistry &param_registry) = 0;

  Tracklist * get_tracklist () const;

  dsp::PortConnectionsManager * get_port_connections_manager () const;

  bool has_piano_roll () const { return type_has_piano_roll (type_); }

  /**
   * Returns whether the track should be visible.
   *
   * Takes into account Track.visible and whether any of the track's foldable
   * parents are folded.
   */
  bool should_be_visible () const;

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

  static Track * from_variant (const TrackPtrVariant &variant);

  // bool has_lanes () const { return type_has_lanes (type_); }

  bool is_deletable () const { return type_is_deletable (type_); }
  bool is_copyable () const { return type_is_copyable (type_); }
  // bool has_automation () const { return type_has_automation (type_); }

  /**
   * Returns the full visible height (main height + height of all visible
   * automation tracks + height of all visible lanes).
   */
  template <typename DerivedT>
  double get_full_visible_height (this DerivedT &&self)
    requires std::derived_from<base_type<DerivedT>, Track>
             && FinalClass<base_type<DerivedT>>
  {
    double height = self.main_height_;

    if constexpr (std::derived_from<DerivedT, LanedTrack>)
      {
        height += self.get_visible_lane_heights ();
      }
    if constexpr (AutomatableTrack<DerivedT>)
      {
        const auto * automatable_track = self.automatableTrackMixin ();
        if (automatable_track->automationVisible ())
          {
            const auto * atl = automatable_track->automationTracklist ();
            for (const auto &at_holder : atl->automation_track_holders ())
              {
                if (at_holder->visible ())
                  height += at_holder->height ();
              }
          }
      }
    return height;
  }

  template <typename DerivedT>
  bool multiply_heights (
    this DerivedT &&self,
    double          multiplier,
    bool            visible_only,
    bool            check_only)
    requires std::derived_from<base_type<DerivedT>, Track>
             && FinalClass<base_type<DerivedT>>
  {
    if (self.main_height_ * multiplier < MIN_HEIGHT)
      return false;

    if (!check_only)
      {
        self.main_height_ *= multiplier;
      }

    if constexpr (std::derived_from<DerivedT, LanedTrack>)
      {
        if (!visible_only || self.lanes_visible_)
          {
            for (auto &lane_var : self.lanes_)
              {
                using TrackLaneT = DerivedT::TrackLaneType;
                auto lane = std::get<TrackLaneT *> (lane_var);
                if (lane->height_ * multiplier < MIN_HEIGHT)
                  {
                    return false;
                  }

                if (!check_only)
                  {
                    lane->height_ *= multiplier;
                  }
              }
          }
      }
    if constexpr (AutomatableTrack<DerivedT>)
      {
        if (!visible_only || self.automation_visible_)
          {
            auto &atl = self.get_automation_tracklist ();
            for (auto &at : atl.ats_)
              {
                if (visible_only && !at->visible_)
                  continue;

                if (at->height_ * multiplier < MIN_HEIGHT)
                  {
                    return false;
                  }

                if (!check_only)
                  {
                    at->height_ *= multiplier;
                  }
              }
          }
      }

    return true;
  }

  /** Whether this track is part of the SampleProcessor auditioner tracklist. */
  bool is_auditioner () const;

  bool can_be_group_target () const { return type_can_be_group_target (type_); }

  bool is_frozen () const { return frozen_clip_id_.has_value (); }

  /**
   * Inserts a Region to the given lane or AutomationTrack of the track, at
   * the given index.
   *
   * The Region must be the main region (see ArrangerObjectInfo).
   *
   * @param at The AutomationTrack of this Region, if automation region.
   * @param lane_pos The position of the lane to add to, if applicable.
   * @param idx The index to insert the region at inside its parent, or
   * nullopt to append.
   * @param gen_name Generate a unique region name or not. This will be 0 if
   * the caller already generated a unique name.
   *
   * @throw ZrythmException if the insertion fails.
   */
  template <arrangement::RegionObject RegionT, FinalClass SelfT>
  void insert_region (
    this SelfT                              &self,
    arrangement::ArrangerObjectUuidReference region_ref,
    AutomationTrack *                        at,
    std::optional<int>                       lane_pos,
    std::optional<int>                       idx,
    bool                                     gen_name)
  {
    auto * region = std::get<RegionT *> (region_ref.get_object ());
    // assert (region->validate (false, 0));
    assert (type_can_have_region_type (self.type_, region->type ()));

    if (gen_name)
      {
        if (at)
          {
            region->regionMixin ()->name ()->setName (
              utils::Utf8String::from_utf8_encoded_string (
                fmt::format (
                  "{} - {}", self.get_name (), at->parameter ()->label ()))
                .to_qstring ());
          }
        else
          {
            region->regionMixin ()->name ()->setName (self.getName ());
          }
      }

    assert (!region->regionMixin ()->name ()->get_name ().empty ());
    z_debug (
      "inserting region '{}' to track '{}' at lane {} (idx {})",
      region->regionMixin ()->name ()->get_name (), self.name_, lane_pos, idx);

    // region->set_track_id (self.get_uuid ());

    if constexpr (
      arrangement::LaneOwnedObject<RegionT>
      && std::derived_from<SelfT, LanedTrack>)
      {
        /* enable extra lane if necessary */
        self.create_missing_lanes (*lane_pos);

        auto lane = std::get<typename SelfT::TrackLaneType *> (
          self.lanes_.at (*lane_pos));
        if (idx.has_value ())
          {
            lane->insert_object (region_ref, *idx);
          }
        else
          {
            lane->add_object (region_ref);
          }
      }
    else if constexpr (std::is_same_v<RegionT, arrangement::AutomationRegion>)
      {
        assert (at != nullptr);
        if (idx.has_value ())
          {
            at->insert_object (region_ref, *idx);
          }
        else
          {
            at->add_object (region_ref);
          }
      }
    else if constexpr (
      std::is_same_v<RegionT, arrangement::ChordRegion>
      && std::is_same_v<SelfT, tracks::ChordTrack>)
      {
        if (idx.has_value ())
          {
            self.template ArrangerObjectOwner<
              arrangement::ChordRegion>::insert_object (region_ref, *idx);
          }
        else
          {
            self.template ArrangerObjectOwner<
              arrangement::ChordRegion>::add_object (region_ref);
          }
      }

    /* write clip if audio region */
    if constexpr (std::is_same_v<RegionT, arrangement::AudioRegion>)
      {
        if (!self.is_auditioner ())
          {
            // TODO in pool, just listen to when a new clip is added in the clip
            // registry and auto-write then
#if 0
            auto * clip = region->get_clip ();
            self.write_audio_clip_to_pool_after_adding_audio_region (*clip);
#endif
          }
      }

    z_debug ("inserted: {}", *region);
  }

  /**
   * Appends a Region to the given lane or AutomationTrack of the track.
   *
   * @see insert_region().
   *
   * @throw ZrythmException if the insertion fails.
   */
  template <arrangement::RegionObject RegionT>
  void add_region (
    this auto         &self,
    auto               region_ref,
    AutomationTrack *  at,
    std::optional<int> lane_pos,
    bool               gen_name)
  {
    return self.Track::template insert_region<RegionT> (
      region_ref, at, lane_pos, std::nullopt, gen_name);
  }

  /**
   * @brief Appends all the objects in the track to @p objects.
   *
   * This only appends top-level objects. For example, region children will
   * not be added.
   *
   * @param objects
   */
  void append_objects (std::vector<ArrangerObjectPtrVariant> &objects) const;

  /**
   * Unselects all arranger objects in the track.
   */
  void unselect_all ();

  template <typename DerivedT>
  bool contains_uninstantiated_plugin (this DerivedT &&self)
    requires std::derived_from<base_type<DerivedT>, Track>
             && FinalClass<base_type<DerivedT>>
  {
    std::vector<zrythm::gui::old_dsp::plugins::Plugin *> plugins;
    self.get_plugins (plugins);
    return std::ranges::any_of (plugins, [] (auto pl) {
      return pl->instantiation_failed_;
    });
  }

  /**
   * Removes all objects recursively from the track.
   */
  virtual void clear_objects () { };

  // temporary hack to avoid having to pass constructor arguments through
  // intermediate classes. the whole hierarchy needs to be refactored
  virtual void temporary_virtual_method_hack () const = 0;

  /**
   * Adds the track's folder parents to the given vector.
   *
   * @param prepend Whether to prepend instead of append.
   */
  void
  add_folder_parents (std::vector<FoldableTrack *> &parents, bool prepend) const;

  /**
   * Returns the closest foldable parent or NULL.
   */
  FoldableTrack * get_direct_folder_parent () const
  {
    std::vector<FoldableTrack *> parents;
    add_folder_parents (parents, true);
    if (!parents.empty ())
      {
        return parents.front ();
      }
    return nullptr;
  }

  /**
   * Remove the track from all folders.
   *
   * Used when deleting tracks.
   */
  void remove_from_folder_parents ();

  /**
   * Getter for the track name.
   */
  utils::Utf8String get_name () const { return name_; };

  /**
   * Internally called by set_name_with_action().
   */
  bool set_name_with_action_full (const utils::Utf8String &name);

  /**
   * Setter to be used by the UI to create an undoable action.
   */
  void set_name_with_action (const utils::Utf8String &name);

  /**
   * Setter for the track name.
   *
   * If a track with that name already exists, it adds a number at the end.
   *
   * Must only be called from the GTK thread.
   */
  void set_name (
    const Tracklist         &tracklist,
    const utils::Utf8String &name,
    bool                     pub_events);

  /**
   * Returns a unique name for a new track based on the given name.
   */
  utils::Utf8String
  get_unique_name (const Tracklist &tracklist, const utils::Utf8String &name);

  /**
   * Returns all the regions inside the given range, or all the regions if both
   * @ref p1 and @ref p2 are NULL.
   *
   * @return The number of regions returned.
   */
  virtual void get_regions_in_range (
    std::vector<arrangement::ArrangerObjectUuidReference> &regions,
    std::optional<signed_frame_t>                          p1,
    std::optional<signed_frame_t>                          p2)
  {
  }

  // FIXME: this member is likely not needed
  int  get_index () const { return pos_; }
  void set_index (int index) { pos_ = index; }

  auto get_input_signal_type () const { return in_signal_type_; }
  auto get_output_signal_type () const { return out_signal_type_; }

  /**
   * Fills in the given array with all plugins in the track.
   */
  template <typename DerivedT>
  void get_plugins (
    this DerivedT                                       &&self,
    std::vector<zrythm::gui::old_dsp::plugins::Plugin *> &arr)
    requires std::derived_from<base_type<DerivedT>, Track>
             && FinalClass<base_type<DerivedT>>
  {
    if constexpr (std::derived_from<DerivedT, ChannelTrack>)
      {
        self.channel_->get_plugins (arr);
      }

    if constexpr (std::is_same_v<DerivedT, ModulatorTrack>)
      {
        for (const auto &modulator : self.modulators_)
          {
            if (modulator)
              {
                arr.push_back (modulator.get ());
              }
          }
      }
  }

  /**
   * Activate or deactivate all plugins.
   *
   * This is useful for exporting: deactivating and reactivating a plugin will
   * reset its state.
   */
  template <typename DerivedT>
  void activate_all_plugins (this DerivedT &&self, bool activate)
    requires std::derived_from<base_type<DerivedT>, Track>
             && FinalClass<base_type<DerivedT>>
  {
    std::vector<zrythm::gui::old_dsp::plugins::Plugin *> pls;
    self.get_plugins (pls);

    for (auto &pl : pls)
      {
        if (!pl->instantiated_ && !pl->instantiation_failed_)
          {
            try
              {
                pl->instantiate ();
              }
            catch (const ZrythmException &e)
              {
                e.handle ("Failed to instantiate plugin");
              }
          }

        if (pl->instantiated_)
          {
            pl->activate (activate);
          }
      }
  }

  utils::Utf8String get_comment () const { return comment_; }

  /**
   * @param undoable Create an undable action.
   */
  void set_comment (const utils::Utf8String &comment, bool undoable);

  void set_comment_with_action (const utils::Utf8String &comment)
  {
    set_comment (comment, true);
  }

  /**
   * Sets the track color.
   */
  void set_color (const Color &color, bool undoable, bool fire_events);

  /**
   * Sets the track icon.
   */
  void
  set_icon (const utils::Utf8String &icon_name, bool undoable, bool fire_events);

  /**
   * Freezes or unfreezes the track.
   *
   * When a track is frozen, it is bounced with effects to a temporary file in
   * the pool, which is played back directly from disk.
   *
   * When the track is unfrozen, this file will be removed from the pool and
   * the track will be played normally again.
   *
   * @remark Unimplemented/not used.
   *
   * @throw ZrythmException on error.
   */
  void track_freeze (bool freeze);

  /**
   * Wrapper over Channel.add_plugin() and ModulatorTrack.insert_modulator().
   *
   * @param instantiate_plugin Whether to attempt to instantiate the plugin.
   */
  template <typename DerivedT>
  PluginPtrVariant insert_plugin (
    this DerivedT     &&self,
    PluginUuid          plugin_id,
    plugins::PluginSlot slot,
    bool                instantiate_plugin,
    bool                replacing_plugin,
    bool                moving_plugin,
    bool                confirm,
    bool                gen_automatables,
    bool                recalc_graph,
    bool                fire_events)
    requires std::derived_from<base_type<DerivedT>, Track>
             && FinalClass<base_type<DerivedT>>
  {
    if (!slot.validate_slot_type_slot_combo ())
      {
        throw std::runtime_error ("Invalid slot type and slot combo");
      }

    PluginPtrVariant plugin_var{};

    if (slot.is_modulator ())
      {
        if constexpr (std::is_same_v<DerivedT, ModulatorTrack>)
          {
            plugin_var = self.insert_modulator (
              slot.get_slot_with_index ().second, plugin_id, replacing_plugin,
              confirm, gen_automatables, recalc_graph, fire_events);
          }
      }
    else
      {
        if constexpr (std::derived_from<DerivedT, ChannelTrack>)
          {
            plugin_var = self.get_channel ()->add_plugin (
              plugin_id, slot, confirm, moving_plugin, gen_automatables,
              recalc_graph, fire_events);
          }
      }

    std::visit (
      [&] (auto &&plugin) {
        if (plugin && !plugin->instantiated_ && !plugin->instantiation_failed_)
          {
            try
              {
                plugin->instantiate ();
              }
            catch (const ZrythmException &e)
              {
                e.handle ("Failed to instantiate plugin");
              }
          }
      },
      plugin_var);

    return plugin_var;
  }

  /**
   * Removes a plugin at the given slot from the track.
   *
   * @param moving_plugin Whether or not we are moving the plugin.
   * @param deleting_plugin Whether or not we are deleting the plugin.
   */
  template <typename SelfT>
  void remove_plugin (
    this SelfT         &self,
    plugins::PluginSlot slot,
    bool                moving_plugin = false,
    bool                deleting_plugin = true)
    requires (
      std::derived_from<SelfT, ChannelTrack>
      || std::is_same_v<SelfT, ModulatorTrack>)
  {
    z_debug ("removing plugin from track {}", self.name_);
    if constexpr (std::is_same_v<SelfT, ModulatorTrack>)
      {
        assert (slot.is_modulator ());
        const auto slot_idx = slot.get_slot_with_index ().second;
        auto       plugin_id = self.modulators_[slot_idx];
        auto       plugin_var = plugin_id.get_object ();
        std::visit (
          [&] (auto &&plugin) {
            assert (plugin->get_track_id () == self.get_uuid ());

            plugin->remove_ats_from_automation_tracklist (true, !false && !true);

            z_debug (
              "Removing {} from {}:{}", plugin->get_name (), self.get_name (),
              slot);

            /* if deleting plugin disconnect the plugin entirely */
            plugin->set_selected (false);
            self.tracklist_->disconnect_plugin (plugin->get_uuid ());

            auto it =
              self.modulators_.erase (self.modulators_.begin () + slot_idx);
            for (; it != self.modulators_.end (); ++it)
              {
                const auto &mod_id = *it;
                auto        mod_var = mod_id.get_object ();
                std::visit (
                  [&] (auto &&mod) { mod->set_track (self.get_uuid ()); },
                  mod_var);
              }
          },
          plugin_var);
      }
    else if constexpr (std::derived_from<SelfT, ChannelTrack>)
      {
        assert (!slot.is_modulator ());
        auto channel = self.get_channel ();
        auto plugin_opt = channel->get_plugin_at_slot (slot);
        assert (plugin_opt);
        auto plugin_ptr = *plugin_opt;

        std::visit (
          [&] (auto &&plugin) {
            z_debug (
              "Removing {} from {}:{}", plugin->get_name (), self.get_name (),
              slot);

            /* if moving, the move is already handled in
             * plugin_move_automation() inside plugin_move(). */
            if (!moving_plugin)
              {
                plugin->remove_ats_from_automation_tracklist (
                  deleting_plugin, !deleting_plugin);
              }

            /* if deleting plugin disconnect the plugin entirely */
            if (deleting_plugin)
              {
                if (plugin->is_selected ())
                  {
                    plugin->set_selected (false);
                  }

                self.tracklist_->disconnect_plugin (plugin->get_uuid ());
              }
          },
          plugin_ptr);
      }
  }

  template <typename SelfT>
  plugins::PluginSlot
  get_plugin_slot (this const SelfT &self, const PluginUuid &plugin_id)
    requires (
      std::derived_from<SelfT, ChannelTrack>
      || std::is_same_v<SelfT, ModulatorTrack>)
  {
    if constexpr (std::derived_from<SelfT, ChannelTrack>)
      {
        return self.channel_->get_plugin_slot (plugin_id);
      }
    else if constexpr (std::is_same_v<SelfT, ModulatorTrack>)
      {
        return self.modulator_->get_plugin_slot (plugin_id);
      }
  }

  /**
   * Returns if @p descr can be dropped at @p slot_type in a track of type @p
   * track_type.
   */
  static bool is_plugin_descriptor_valid_for_slot_type (
    const plugins::PluginDescriptor &descr,
    zrythm::plugins::PluginSlotType  slot_type,
    Track::Type                      track_type)
  {
    switch (slot_type)
      {
      case zrythm::plugins::PluginSlotType::Insert:
        if (track_type == Track::Type::Midi)
          {
            return descr.num_midi_outs_ > 0;
          }
        else
          {
            return descr.num_audio_outs_ > 0;
          }
      case zrythm::plugins::PluginSlotType::MidiFx:
        return descr.num_midi_outs_ > 0;
        break;
      case zrythm::plugins::PluginSlotType::Instrument:
        return track_type == Track::Type::Instrument && descr.is_instrument ();
      default:
        break;
      }

    z_return_val_if_reached (false);
  }

  bool is_enabled () const { return enabled_; }
  bool enabled () const { return enabled_; }
  bool disabled () const { return !enabled_; }
  void set_enabled (bool enabled) { enabled_ = enabled; }

  void set_enabled (
    bool enabled,
    bool trigger_undo,
    bool auto_select,
    bool fire_events);

  int
  get_total_bars (const engine::session::Transport &transport, int total_bars)
    const;

  /**
   * Set various caches (snapshots, track name hash, plugin input/output
   * ports, etc).
   */
  void set_caches (CacheType types);

  /**
   * @brief Creates a new track with the given parameters.
   *
   * @param disable_track_idx Track index to disable, or -1.
   * @param ready_cb Callback to be called when the tracks are ready (added
   * to the project).
   *
   * @throw ZrythmException on error.
   */
  static void create_with_action (
    Type                                         type,
    const zrythm::plugins::PluginConfiguration * pl_setting,
    const FileDescriptor *                       file_descr,
    const dsp::Position *                        pos,
    int                                          index,
    int                                          num_tracks,
    int                                          disable_track_idx,
    TracksReadyCallback                          ready_cb);

  /**
   * @brief Creates a new empty track at the given index.
   *
   * @param type
   * @param index
   * @param error
   * @return Track*
   * @throw ZrythmException on error.
   */
  static Track * create_empty_at_idx_with_action (Type type, int index);

  /**
   * @brief Creates a new track for the given plugin at the given index.
   *
   * @param type
   * @param pl_setting
   * @param index
   * @param error
   * @return Track*
   * @throw ZrythmException on error.
   */
  static Track * create_for_plugin_at_idx_w_action (
    Type                                         type,
    const zrythm::plugins::PluginConfiguration * pl_setting,
    int                                          index);

  template <typename T = Track>
  static T * create_for_plugin_at_idx_w_action (
    const zrythm::plugins::PluginConfiguration * pl_setting,
    int                                          index)
  {
    return dynamic_cast<T *> (create_for_plugin_at_idx_w_action (
      get_type_for_class<T> (), pl_setting, index));
  }

  /**
   * @brief Creates a new empty track at the end of the tracklist.
   *
   * @param type
   * @return Track*
   * @throw ZrythmException on error.
   */
  static Track * create_empty_with_action (Type type);

  template <typename T = Track> static T * create_empty_with_action ()
  {
    return dynamic_cast<T *> (
      create_empty_with_action (get_type_for_class<T> ()));
  }

#if 0
  /**
   * @brief Create a track of the given type with the given name and position.
   *
   * @note Only works for non-singleton tracks.
   * @param name
   * @param pos
   */
  [[nodiscard]] static TrackUniquePtrVariant
  create_track (Type type, const utils::Utf8String &name, int pos);
#endif

  // GMenu * generate_edit_context_menu (int num_selected);

  utils::Utf8String get_full_designation_for_port (const dsp::Port &port) const;

  virtual bool currently_muted () const { return false; }

  virtual bool currently_listened () const { return false; }

  /**
   * Returns whether the track is not soloed on its own but its direct out (or
   * its direct out's direct out, etc.) is soloed.
   */
  virtual bool get_implied_soloed () const { return false; }

  virtual bool currently_soloed () const { return false; }

  void set_selection_status_getter (TrackSelectionStatusGetter getter)
  {
    track_selection_status_getter_ = getter;
  }
  void unset_selection_status_getter ()
  {
    track_selection_status_getter_.reset ();
  }

  auto &get_plugin_registry () const { return plugin_registry_; }
  auto &get_plugin_registry () { return plugin_registry_; }
  auto &get_port_registry () const { return port_registry_; }
  auto &get_port_registry () { return port_registry_; }
  auto &get_param_registry () const { return param_registry_; }
  auto &get_param_registry () { return param_registry_; }
  auto &get_object_registry () const { return object_registry_; }
  auto &get_object_registry () { return object_registry_; }

  auto get_type () const { return type_; }
  auto get_icon_name () const { return icon_name_; }

  friend bool operator< (const Track &lhs, const Track &rhs)
  {
    return lhs.pos_ < rhs.pos_;
  }

protected:
  friend void
  init_from (Track &obj, const Track &other, utils::ObjectCloneType clone_type);

  /**
   * @brief Set the playback caches for a track.
   *
   * This is called by @ref set_caches().
   */
  virtual void set_playback_caches () { }

  void add_region_if_in_range (
    std::optional<signed_frame_t>                          p1,
    std::optional<signed_frame_t>                          p2,
    std::vector<arrangement::ArrangerObjectUuidReference> &regions,
    arrangement::ArrangerObjectUuidReference               region);

private:
  static constexpr std::string_view kTypeKey = "type";
  static constexpr std::string_view kNameKey = "name";
  static constexpr std::string_view kIconNameKey = "iconName";
  static constexpr std::string_view kIndexKey = "index";
  static constexpr std::string_view kVisibleKey = "visible";
  static constexpr std::string_view kMainHeightKey = "mainHeight";
  static constexpr std::string_view kEnabledKey = "enabled";
  static constexpr std::string_view kColorKey = "color";
  static constexpr std::string_view kInputSignalTypeKey = "inSignalType";
  static constexpr std::string_view kOutputSignalTypeKey = "outSignalType";
  static constexpr std::string_view kCommentKey = "comment";
  static constexpr std::string_view kFrozenClipIdKey = "frozenClipId";
  friend void to_json (nlohmann::json &j, const Track &track)
  {
    to_json (j, static_cast<const UuidIdentifiableObject &> (track));
    j[kTypeKey] = track.type_;
    j[kNameKey] = track.name_;
    j[kIconNameKey] = track.icon_name_;
    j[kIndexKey] = track.pos_;
    j[kVisibleKey] = track.visible_;
    j[kMainHeightKey] = track.main_height_;
    j[kEnabledKey] = track.enabled_;
    j[kColorKey] = track.color_;
    j[kInputSignalTypeKey] = track.in_signal_type_;
    j[kOutputSignalTypeKey] = track.out_signal_type_;
    j[kCommentKey] = track.comment_;
    j[kFrozenClipIdKey] = track.frozen_clip_id_;
  }
  friend void from_json (const nlohmann::json &j, Track &track);

  /**
   * @brief Create a new track.
   *
   * @param type
   * @param pl_setting
   * @param index
   * @return void*
   * @throw ZrythmException on error.
   */
  static Track * create_without_file_with_action (
    Type                                         type,
    const zrythm::plugins::PluginConfiguration * pl_setting,
    int                                          index);

protected:
  PluginRegistry                      &plugin_registry_;
  dsp::PortRegistry                   &port_registry_;
  dsp::ProcessorParameterRegistry     &param_registry_;
  arrangement::ArrangerObjectRegistry &object_registry_;

  /**
   * Position in the Tracklist.
   *
   * This is also used in the Mixer for the Channels.
   * If a track doesn't have a Channel, the Mixer can just skip.
   */
  int pos_ = 0;

  /** The type of track this is. */
  Type type_ = {};

  /** Track name, used in channel too. */
  utils::Utf8String name_;

  /** Icon name of the track. */
  utils::Utf8String icon_name_;

  /**
   * Track Widget created dynamically.
   *
   * 1 track has 1 widget.
   */
  // TrackWidget * widget_ = nullptr;

  /** Whole Track is visible or not. */
  bool visible_ = true;

  /** Track will be hidden if true (temporary and not serializable). */
  bool filtered_ = false;

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

public:
  /**
   * Set to ON during bouncing if this track should be included.
   *
   * Only relevant for tracks that output audio.
   */
  bool bounce_{};

  /**
   * Whether to temporarily route the output to master (e.g., when bouncing
   * the track on its own without its parents).
   */
  bool bounce_to_master_ = false;

  /** Whether currently disconnecting. */
  bool disconnecting_ = false;

  /** Pool ID of the clip if track is frozen (unset if not frozen). */
  std::optional<dsp::FileAudioSourceUuidReference> frozen_clip_id_;

  /** Pointer to owner tracklist, if any. */
  Tracklist * tracklist_ = nullptr;

  /**
   * @brief Track selection status getter.
   *
   * To be set by the tracklist when a track gets added to it.
   */
  std::optional<TrackSelectionStatusGetter> track_selection_status_getter_;
};

template <typename T>
concept TrackSubclass = std::derived_from<T, Track>;

template <typename TrackT>
concept FinalTrackSubclass = TrackSubclass<TrackT> && FinalClass<TrackT>;

template <typename TrackT>
concept TrackWithPlugins =
  FinalTrackSubclass<TrackT>
  && (std::derived_from<TrackT, ChannelTrack> || std::is_same_v<TrackT, ModulatorTrack>);

using TrackRegistry = utils::OwningObjectRegistry<TrackPtrVariant, Track>;
using TrackRegistryRef = std::reference_wrapper<TrackRegistry>;
using TrackUuidReference = utils::UuidReference<TrackRegistry>;
using TrackSelectionManager =
  utils::UuidIdentifiableObjectSelectionManager<TrackRegistry>;

struct FinalTrackDependencies
{
  const dsp::TempoMap                   &tempo_map_;
  dsp::FileAudioSourceRegistry          &file_audio_source_registry_;
  TrackRegistry                         &track_registry_;
  gui::old_dsp::plugins::PluginRegistry &plugin_registry_;
  dsp::PortRegistry                     &port_registry_;
  dsp::ProcessorParameterRegistry       &param_registry_;
  arrangement::ArrangerObjectRegistry   &obj_registry_;
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
