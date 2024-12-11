// SPDX-FileCopyrightText: © 2018-2022, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef __DSP_REGION_H__
#define __DSP_REGION_H__

#include <format>
#include <memory>

#include "gui/dsp/arranger_object.h"
#include "gui/dsp/automation_point.h"
#include "gui/dsp/colored_object.h"
#include "gui/dsp/loopable_object.h"
#include "gui/dsp/muteable_object.h"
#include "gui/dsp/nameable_object.h"
#include "gui/dsp/region_identifier.h"
#include "gui/dsp/timeline_object.h"

#include "dsp/position.h"
#include "utils/format.h"

class Track;

class RegionLinkGroup;
// class RegionOwner;
class MidiNote;
class MidiRegion;
class ChordRegion;
// class LaneOwnedObject;
class ChordObject;
class AudioRegion;
class AutomationRegion;
template <typename RegionT> class RegionOwnerImpl;

/**
 * @addtogroup dsp
 *
 * @{
 */

constexpr int REGION_MAGIC = 93075327;
#define IS_REGION(x) (static_cast<Region *> (x)->magic_ == REGION_MAGIC)
#define IS_REGION_AND_NONNULL(x) (x && IS_REGION (x))

#define DEFINE_REGION_QML_PROPERTIES(ClassType) \
public: \
  DEFINE_LOOPABLE_OBJECT_QML_PROPERTIES (ClassType) \
  DEFINE_NAMEABLE_OBJECT_QML_PROPERTIES (ClassType) \
  DEFINE_COLORED_OBJECT_QML_PROPERTIES (ClassType) \
  /* ================================================================ */ \
  /* regionType */ \
  /* ================================================================ */ \
  Q_PROPERTY (int regionType READ getRegionType CONSTANT) \
  int getRegionType () const \
  { \
    return ENUM_VALUE_TO_INT (id_.type_); \
  } \
  /* ================================================================ */ \
  /* helpers */ \
  /* ================================================================ */

constexpr const char * REGION_PRINTF_FILENAME = "%s_%s.mid";

/**
 * Returns if the given Region type can exist in TrackLane's.
 */
static constexpr bool
region_type_has_lane (const RegionType type)
{
  return type == RegionType::Midi || type == RegionType::Audio;
}

/**
 * Returns if the given Region type can have fades.
 */
static constexpr bool
region_type_can_fade (RegionType rtype)
{
  return rtype == RegionType::Audio;
}

#if 0
template <typename T> struct RegionTypeFromObject;

template <> struct RegionTypeFromObject<MidiNote>
{
  using type = MidiRegion;
};

template <> struct RegionTypeFromObject<ChordObject>
{
  using type = ChordRegion;
};

template <> struct RegionTypeFromObject<AutomationPoint>
{
  using type = AutomationRegion;
};

template <typename T>
using RegionTypeFromObject_t = typename RegionTypeFromObject<T>::type;
#endif

template <typename RegionT> struct RegionChildType;

template <> struct RegionChildType<MidiRegion>
{
  using type = MidiNote;
};

template <> struct RegionChildType<ChordRegion>
{
  using type = ChordObject;
};

template <> struct RegionChildType<AutomationRegion>
{
  using type = AutomationPoint;
};

class DummyRegionOwnedObject
{
};

// avoid errors
template <> struct RegionChildType<AudioRegion>
{
  using type = DummyRegionOwnedObject;
};

template <typename RegionT>
using RegionChildType_t = typename RegionChildType<RegionT>::type;

template <typename RegionT>
concept RegionWithChildren =
  !std::is_same_v<RegionChildType_t<RegionT>, DummyRegionOwnedObject>;

/**
 * Musical mode setting for audio regions.
 */
enum class MusicalMode
{
  /** Inherit from global musical mode setting. */
  Inherit,
  /** Musical mode off - don't auto-stretch when BPM changes. */
  Off,
  /** Musical mode on - auto-stretch when BPM changes. */
  On,
};

using RegionVariant =
  std::variant<MidiRegion, ChordRegion, AutomationRegion, AudioRegion>;
using RegionPtrVariant = to_pointer_variant<RegionVariant>;

template <typename T>
concept RegionTypeWithMidiEvents =
  std::same_as<T, MidiRegion> || std::same_as<T, ChordRegion>;

/**
 * A region (clip) is an object on the timeline that contains either
 * MidiNote's or AudioClip's.
 *
 * It is uniquely identified using its name, so it must be unique
 * throughout the Project.
 */
class Region
    : virtual public TimelineObject,
      virtual public NameableObject,
      virtual public MuteableObject,
      virtual public LoopableObject,
      virtual public ColoredObject,
      public zrythm::utils::serialization::ISerializable<Region>
{
public:
  ~Region () override = default;

  /**
   * Only to be used by implementing structs.
   */
  void init (
    const dsp::Position &start_pos,
    const dsp::Position &end_pos,
    unsigned int         track_name_hash,
    int                  lane_pos_or_at_idx,
    int                  idx_inside_lane_or_at,
    double               ticks_per_frame = 0.0);

  /**
   * Adds the given ticks to each included object.
   */
  virtual void add_ticks_to_children (double ticks) = 0;

  RegionType get_type () const { return id_.type_; }

  bool is_midi () const { return get_type () == RegionType::Midi; }
  bool is_audio () const { return get_type () == RegionType::Audio; }
  bool is_automation () const { return get_type () == RegionType::Automation; }
  bool is_chord () const { return get_type () == RegionType::Chord; }

  std::string print_to_str () const override;

  /**
   * Returns the region's link group.
   */
  RegionLinkGroup * get_link_group ();

  bool has_link_group () const { return id_.link_group_ >= 0; }

  /**
   * Converts frames on the timeline (global) to local frames (in the
   * clip).
   *
   * If normalize is 1 it will only return a position from 0 to loop_end
   * (it will traverse the loops to find the appropriate position),
   * otherwise it may exceed loop_end.
   *
   * @param timeline_frames Timeline position in frames.
   *
   * @return The local frames.
   */
  ATTR_HOT signed_frame_t
  timeline_frames_to_local (signed_frame_t timeline_frames, bool normalize) const;

  /**
   * Returns the number of frames until the next loop end point or the
   * end of the region.
   *
   * @param[in] timeline_frames Global frames at start.
   * @param[out] ret_frames Return frames.
   * @param[out] is_loop Whether the return frames are for a loop (if
   * false, the return frames are for the region's end).
   */
  ATTR_NONNULL void get_frames_till_next_loop_or_end (
    signed_frame_t   timeline_frames,
    signed_frame_t * ret_frames,
    bool *           is_loop) const;

  inline bool has_lane () const { return region_type_has_lane (id_.type_); }

  /**
   * Generates a name for the Region, either using the given
   * AutomationTrack or Track, or appending to the given base name.
   */
  void gen_name (const char * base_name, AutomationTrack * at, Track * track);

  /**
   * Updates all other regions in the region link group, if any.
   */
  void update_link_group ();

  static std::optional<RegionPtrVariant> find (const RegionIdentifier &id);

  /**
   * Generates the filename for this region.
   */
  std::string generate_filename ();

  /**
   * Returns if this region is currently being recorded onto.
   */
  bool is_recording ();

  /**
   * Returns the ArrangerSelections based on the given region type.
   */
  virtual std::optional<ClipEditorArrangerSelectionsPtrVariant>
  get_arranger_selections () const = 0;

  bool can_have_lanes () const override
  {
    return region_type_has_lane (id_.type_);
  }

  bool can_fade () const override { return region_type_can_fade (id_.type_); }

  void post_deserialize_children ();

  /**
   * @brief Returns the region at the given position, if any.
   *
   * Either @p at (for automation regions) or @p track (for other regions)
   * must be passed.
   *
   * @param track
   * @param at
   * @param include_region_end
   */
  static std::optional<RegionPtrVariant> get_at_pos (
    dsp::Position     pos,
    Track *           track,
    AutomationTrack * at,
    bool              include_region_end = false);

protected:
  Region () = default;
  Q_DISABLE_COPY_MOVE (Region)

  void copy_members_from (const Region &other);

  DECLARE_DEFINE_BASE_FIELDS_METHOD ();

public:
  /** Unique ID. */
  RegionIdentifier id_;

  /**
   * Set to ON during bouncing if this region should be included.
   *
   * Only relevant for audio and midi regions.
   */
  int bounce_ = 0;

  /* --- stretching related --- */

  /**
   * Whether currently running the stretching algorithm.
   *
   * If this is true, region drawing will be deferred.
   */
  bool stretching_ = false;

  /** Used during arranger UI overlay actions. */
  double stretch_ratio_ = 1.0;

  /**
   * The length before stretching, in ticks.
   */
  double before_length_ = 0.0;

  /* --- stretching related end --- */

  int magic_ = REGION_MAGIC;
};

/**
 * @tparam RegionT The region type to specialize for (CRTP type).
 */
template <typename RegionT> class RegionImpl : virtual public Region
{
public:
  ~RegionImpl () override = default;

  using RegionTPtr = RegionT *;

  static constexpr bool is_midi ()
  {
    return std::is_same_v<RegionT, MidiRegion>;
  }
  static constexpr bool is_audio ()
  {
    return std::is_same_v<RegionT, AudioRegion>;
  }
  static constexpr bool is_automation ()
  {
    return std::is_same_v<RegionT, AutomationRegion>;
  }
  static constexpr bool is_chord ()
  {
    return std::is_same_v<RegionT, ChordRegion>;
  }
  static constexpr bool is_laned ()
  {
    return std::is_same_v<MidiRegion, RegionT>
           || std::is_same_v<AudioRegion, RegionT>;
  }

  static constexpr bool has_children ()
  {
    return !std::is_same_v<RegionT, AudioRegion>;
  }

  using ChildT = RegionChildType_t<RegionT>;
  using ChildTPtr = ChildT *;

  /**
   * @brief Fills the given vector with all the children of this region.
   */
  virtual void append_children (
    std::vector<RegionOwnedObjectImpl<RegionT> *> &children) const = 0;

  /**
   * Looks for the Region matching the identifier.
   */
  ATTR_HOT static RegionTPtr find (const RegionIdentifier &id);

  std::optional<ArrangerObjectPtrVariant> find_in_project () const final
  {
    return find (id_);
  }

  /**
   * @brief Fills MIDI event queue from this MIDI or Chord region.
   *
   * The events are dequeued right after the call to this function.
   *
   * @note The caller already splits calls to this function at each
   * sub-loop inside the region, so region loop related logic is not
   * needed.
   *
   * @param note_off_at_end Whether a note off should be added at the
   * end frame (eg, when the caller knows there is a region loop or the
   * region ends).
   * @param is_note_off_for_loop_or_region_end Whether @p
   * note_off_at_end is for a region loop end or the region's end (in
   * this case normal note offs will be sent, otherwise a single ALL
   * NOTES OFF event will be sent).
   * @param midi_events MidiEvents (queued) to fill (from Piano Roll
   * Port for example).
   */
  void fill_midi_events (
    const EngineProcessTimeInfo &time_nfo,
    bool                         note_off_at_end,
    bool                         is_note_off_for_loop_or_region_end,
    MidiEventVector             &midi_events) const
    requires RegionTypeWithMidiEvents<RegionT>
  ATTR_NONBLOCKING;

  /**
   * @brief Get the objects (midi notes/chord objects/etc) of this
   * region.
   *
   * @tparam T Object type.
   * @return A vector of object pointers.
   */
  std::span<ChildTPtr> get_objects ()
    requires RegionWithChildren<RegionT>;

  std::vector<ChildTPtr> &get_objects_vector ()
    requires RegionWithChildren<RegionT>;

  RegionOwnerImpl<RegionT> * get_region_owner () const;

  /**
   * Removes all children objects from the region.
   */
  void remove_all_children ()
    requires RegionWithChildren<RegionT>;

  /**
   * Clones and copies all children from @p src to @p dest.
   */
  void copy_children (const RegionImpl &other);

  /**
   * @brief Inserts the given object to this region (if this region has
   * children).
   *
   * @param obj The object to insert.
   * @param index The index to insert at.
   * @param fire_events Whether to fire UI events.
   * @return RegionOwnedObject& The inserted object.
   */
  void insert_object (ChildTPtr obj, int index, bool fire_events = false)
    requires RegionWithChildren<RegionT>;

  /**
   * @see insert_object().
   */
  void append_object (ChildTPtr obj, bool fire_events = false)
    requires RegionWithChildren<RegionT>;

  /**
   * @brief Removes the given object from this region.
   *
   * @tparam T
   * @param obj
   */
  std::optional<ChildTPtr>
  remove_object (ChildT &obj, bool free_obj, bool fire_events = false)
    requires RegionWithChildren<RegionT>;

  bool get_muted (bool check_parent) const override;

  ArrangerObjectPtrVariant add_clone_to_project (bool fire_events) const final
  {
    return insert_clone_to_project_at_index (-1, fire_events);
  }

  ArrangerObjectPtrVariant insert_clone_to_project () const final
  {
    return insert_clone_to_project_at_index (id_.idx_, true);
  }

  /**
   * Sets the link group to the region.
   *
   * @param group_idx If -1, the region will be
   *   removed from its current link group, if any.
   */
  void set_link_group (int group_idx, bool update_identifier);

  /**
   * Disconnects the region and anything using it.
   *
   * Does not free the Region or its children's resources.
   */
  void disconnect ();
  void disconnect_region ();

  /**
   * To be called every time the identifier changes to update the
   * region's children.
   */
  void update_identifier ();

  /**
   * Returns the region at the given position in the given Track.
   *
   * @param at The automation track to look in.
   * @param track The track to look in, if at is NULL.
   * @param pos The position.
   */
  static RegionT *
  at_position (const Track * track, const AutomationTrack * at, dsp::Position pos);

  /**
   * Removes the link group from the region, if any.
   */
  void unlink ();

  bool are_members_valid (bool is_project) const;

  /**
   * Moves the Region to the given Track, maintaining the selection
   * status of the Region.
   *
   * Assumes that the Region is already in a TrackLane or
   * AutomationTrack.
   *
   * @param lane_or_at_index If MIDI or audio, lane position.
   *   If automation, automation track index in the automation
   * tracklist. If -1, the track lane or automation track index will be
   * inferred from the region.
   * @param index If MIDI or audio, index in lane in the new track to
   * insert the region to, or -1 to append. If automation, index in the
   * automation track.
   *
   * @throw ZrythmException on error.
   */
  void move_to_track (Track * track, int lane_or_at_index, int index);

  /**
   * Stretch the region's contents.
   *
   * This should be called right after changing the region's size.
   *
   * @param ratio The ratio to stretch by.
   *
   * @throw ZrythmException on error.
   */
  void stretch (double ratio);

  void create_link_group_if_none ();

private:
  RegionTPtr
  insert_clone_to_project_at_index (int index, bool fire_events) const;

  /**
   * @brief Sends MIDI note off events or an "all notes off" event at
   * the current time.
   *
   * This is called on MIDI or Chord regions.
   *
   * @param is_note_off_for_loop_or_region_end Whether this is called to
   * send note off events for notes at the loop end/region end boundary
   * (as opposed to a transport loop boundary). If true, separate MIDI
   * note off events will be sent for notes at the border. Otherwise, a
   * single all notes off event will be sent.
   */
  ATTR_HOT void send_note_offs (
    MidiEventVector            &events,
    const EngineProcessTimeInfo time_nfo,
    bool                        is_note_off_for_loop_or_region_end) const
    requires RegionTypeWithMidiEvents<RegionT>;

public:
  RegionT &derived_ = static_cast<RegionT &> (*this);
};

#if 0
/**
 * @brief Partial specialization with constraints.
 *
 * @tparam RegionT
 */
template <typename RegionT>
requires RegionSubclass<RegionT> class RegionImpl<RegionT>
{
};
#endif

template <typename RegionT>
concept FinalRegionSubclass =
  std::derived_from<RegionImpl<RegionT>, Region> && FinalClass<RegionT>;

DEFINE_ENUM_FORMATTER (
  MusicalMode,
  MusicalMode,
  QT_TRANSLATE_NOOP_UTF8 ("MusicalMode", "Inherit"),
  QT_TRANSLATE_NOOP_UTF8 ("MusicalMode", "Off"),
  QT_TRANSLATE_NOOP_UTF8 ("MusicalMode", "On"));

inline bool
operator== (const Region &lhs, const Region &rhs)
{
  return lhs.id_ == rhs.id_;
}

using RegionVariant =
  std::variant<MidiRegion, ChordRegion, AutomationRegion, AudioRegion>;
using RegionPtrVariant = to_pointer_variant<RegionVariant>;

template <typename RegionT>
concept RegionSubclass = DerivedButNotBase<RegionT, Region>;

extern template class RegionImpl<MidiRegion>;
extern template class RegionImpl<AudioRegion>;
extern template class RegionImpl<AutomationRegion>;
extern template class RegionImpl<ChordRegion>;

/**
 * @}
 */

#endif // __DSP_REGION_H__
