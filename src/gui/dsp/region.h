// SPDX-FileCopyrightText: © 2018-2022, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef __DSP_REGION_H__
#define __DSP_REGION_H__

#include <format>
#include <memory>

#include "dsp/position.h"
#include "gui/dsp/arranger_object.h"
#include "gui/dsp/automation_point.h"
#include "gui/dsp/colored_object.h"
#include "gui/dsp/loopable_object.h"
#include "gui/dsp/muteable_object.h"
#include "gui/dsp/named_object.h"
#include "gui/dsp/timeline_object.h"
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
class AutomatableTrack;

/**
 * @addtogroup dsp
 *
 * @{
 */

#define DEFINE_REGION_QML_PROPERTIES(ClassType) \
public: \
  DEFINE_LOOPABLE_OBJECT_QML_PROPERTIES (ClassType) \
  DEFINE_NAMEABLE_OBJECT_QML_PROPERTIES (ClassType) \
  DEFINE_COLORED_OBJECT_QML_PROPERTIES (ClassType) \
  /* ================================================================ */ \
  /* helpers */ \
  /* ================================================================ */

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
      virtual public NamedObject,
      virtual public MuteableObject,
      virtual public LoopableObject,
      public ColoredObject,
      public zrythm::utils::serialization::ISerializable<Region>
{
public:
  ~Region () override = default;

  bool is_midi () const { return get_type () == Type::MidiRegion; }
  bool is_audio () const { return get_type () == Type::AudioRegion; }
  bool is_automation () const { return get_type () == Type::AutomationRegion; }
  bool is_chord () const { return get_type () == Type::ChordRegion; }

  /**
   * Returns the region's link group.
   */
  RegionLinkGroup * get_link_group ();

  bool has_link_group () const { return link_group_.has_value (); }

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
  [[gnu::hot]] signed_frame_t
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
  [[gnu::nonnull]] void get_frames_till_next_loop_or_end (
    signed_frame_t   timeline_frames,
    signed_frame_t * ret_frames,
    bool *           is_loop) const;

  /**
   * Updates all other regions in the region link group, if any.
   */
  void update_link_group ();

  /**
   * Returns if this region is currently being recorded onto.
   */
  bool is_recording ();

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
  static std::optional<ArrangerObjectPtrVariant> get_at_pos (
    dsp::Position     pos,
    Track *           track,
    AutomationTrack * at,
    bool              include_region_end = false);

protected:
  Region (ArrangerObjectRegistry &object_registry)
      : object_registry_ (object_registry)
  {
  }
  Q_DISABLE_COPY_MOVE (Region)

  void copy_members_from (const Region &other, ObjectCloneType clone_type);

  DECLARE_DEFINE_BASE_FIELDS_METHOD ();

public:
  ArrangerObjectRegistry &object_registry_;

  // FIXME: consider tracking link groups separately to decouple the link logic,
  // region (probably) shouldn't know if it's in a link group
  std::optional<int> link_group_;

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
  void
  append_children (this RegionT &self, std::vector<RegionOwnedObject *> &children)
    requires RegionWithChildren<RegionT>
  {
    for (auto * obj : self.get_children_view ())
      {
        children.push_back (obj);
      }
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
    TrackPtrVariant              track_var,
    const EngineProcessTimeInfo &time_nfo,
    bool                         note_off_at_end,
    bool                         is_note_off_for_loop_or_region_end,
    MidiEventVector             &midi_events) const
    requires RegionTypeWithMidiEvents<RegionT>;

  auto get_object_ptr (const ArrangerObject::Uuid id) const
    requires RegionWithChildren<RegionT>
  {
    return std::get<ChildT *> (
      get_arranger_object_registry ().find_by_id_or_throw (id));
  }
  auto get_object_ptr (const ArrangerObjectUuidReference &id) const
    requires RegionWithChildren<RegionT>
  {
    return std::get<ChildT *> (id.get_object ());
  }

#if 0
  auto * get_region_owner (this const RegionT &self)
  {
    if constexpr (is_laned ())
      {
        auto &lane = self.get_lane ();
        return std::addressof (lane);
      }
    else if constexpr (is_automation ())
      {
        return std::visit (
          [&] (auto &&automatable_track) -> AutomationTrack * {
            using TrackT = base_type<decltype (automatable_track)>;
            if constexpr (std::derived_from<TrackT, AutomatableTrack>)
              {
                auto * at =
                  automatable_track->get_automation_tracklist ()
                    .get_automation_track_by_port_id (self.automatable_port_id_);
                return at;
              }
            else
              {
                throw std::runtime_error ("Invalid track");
              }
          },
          self.get_track ());
      }
    else if constexpr (is_chord ())
      {
        return std::get<ChordTrack *> (self.get_track ());
      }
  }
#endif

  auto &get_arranger_object_registry () const { return object_registry_; }
  auto &get_arranger_object_registry () { return object_registry_; }

  /**
   * Removes all children objects from the region.
   */
  void remove_all_children (this RegionT &self)
    requires RegionWithChildren<RegionT>
  {
    z_debug ("removing all children from {} ", self.get_name ());

    auto vec = self.get_children_vector ();
    for (auto &obj : vec)
      {
        self.remove_object (obj.id ());
      }
  }

  bool get_muted (bool check_parent) const override;

  ArrangerObjectPtrVariant add_clone_to_project (bool fire_events) const final
  {
    return insert_clone_to_project_at_index (-1, fire_events);
  }

  ArrangerObjectPtrVariant insert_clone_to_project () const final
  {
    return insert_clone_to_project_at_index (-1, true);
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
  // void disconnect_region ();

  /**
   * To be called every time the identifier changes to update the
   * region's children.
   */
  void update_identifier (this RegionT &self)
  {
    /* reset link group */
    // set_link_group (id_.link_group_, false);

    // track_id_ = id_.track_uuid_;
    if constexpr (RegionWithChildren<RegionT>)
      {
        for (auto * obj : self.get_children_view ())
          {
            obj->set_region_and_index (self);
          }
      }
  }

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

  bool
  are_members_valid (bool is_project, dsp::FramesPerTick frames_per_tick) const;

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
  [[gnu::hot]] void send_note_offs (
    MidiEventVector      &events,
    EngineProcessTimeInfo time_nfo,
    bool                  is_note_off_for_loop_or_region_end) const
    requires RegionTypeWithMidiEvents<RegionT>;

  const RegionT &get_derived () const
  {
    return static_cast<const RegionT &> (*this);
  }
  RegionT &get_derived () { return static_cast<RegionT &> (*this); }
};

template <typename RegionT>
concept FinalRegionSubclass =
  std::derived_from<RegionImpl<RegionT>, Region> && FinalClass<RegionT>;

DEFINE_ENUM_FORMATTER (
  MusicalMode,
  MusicalMode,
  QT_TRANSLATE_NOOP_UTF8 ("MusicalMode", "Inherit"),
  QT_TRANSLATE_NOOP_UTF8 ("MusicalMode", "Off"),
  QT_TRANSLATE_NOOP_UTF8 ("MusicalMode", "On"));

DEFINE_OBJECT_FORMATTER (Region, Region, [] (const Region &r) {
  return fmt::format (
    "Region[id: {}, name: {}, "
    "<{}> to <{}> ({} frames, {} ticks) - loop end <{}> - link group {}"
    "]",
    r.get_uuid (), r.get_name (), r.get_position (), *r.end_pos_,
    r.end_pos_->frames_ - r.get_position ().frames_,
    r.end_pos_->ticks_ - r.get_position ().ticks_, r.loop_end_pos_,
    r.link_group_);
})

inline bool
operator== (const Region &lhs, const Region &rhs)
{
  return lhs.get_uuid () == rhs.get_uuid ();
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
