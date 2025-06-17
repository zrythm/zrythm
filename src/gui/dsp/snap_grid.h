// SPDX-FileCopyrightText: © 2018-2022, 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "dsp/position.h"
#include "utils/note_type.h"

namespace zrythm::structure
{
namespace tracks
{
class Track;
}
namespace arrangement
{
class Region;
}
}

namespace zrythm::gui
{

#define SNAP_GRID_TIMELINE (PROJECT->snap_grid_timeline_)
#define SNAP_GRID_EDITOR (PROJECT->snap_grid_editor_)
#define SNAP_GRID_IS_EDITOR(sg) (SNAP_GRID_EDITOR == sg)
#define SNAP_GRID_IS_TIMELINE(sg) (SNAP_GRID_TIMELINE == sg)

enum class NoteLengthType
{
  /** Link length with snap setting. */
  NOTE_LENGTH_LINK,

  /** Use last created object's length. */
  NOTE_LENGTH_LAST_OBJECT,

  /** Custom length. */
  NOTE_LENGTH_CUSTOM,
};

/**
 * @brief Snap/grid information.
 */
class SnapGrid final
{
public:
  enum class Type
  {
    Timeline,
    Editor,
  };

  static constexpr int DEFAULT_MAX_BAR = 10000;

  using FramesPerTickProvider = std::function<dsp::FramesPerTick (void)>;
  using TicksPerBarProvider = std::function<int (void)>;
  using TicksPerBeatProvider = std::function<int (void)>;
  using Position = zrythm::dsp::Position;
  using Track = structure::tracks::Track;
  using Region = structure::arrangement::Region;

public:
  SnapGrid () = default;

  SnapGrid (
    Type                  type,
    utils::NoteLength     note_length,
    bool                  adaptive,
    FramesPerTickProvider frames_per_tick_provider,
    TicksPerBarProvider   ticks_per_bar_provider,
    TicksPerBeatProvider  ticks_per_beat_provider)
      : type_ (type), snap_adaptive_ (adaptive),
        snap_note_length_ (note_length), default_note_length_ (note_length),
        frames_per_tick_provider_ (std::move (frames_per_tick_provider)),
        ticks_per_bar_provider_ (std::move (ticks_per_bar_provider)),
        ticks_per_beat_provider_ (std::move (ticks_per_beat_provider))
  {
  }

  void set_ticks_per_bar_provider (TicksPerBarProvider ticks_per_bar_provider)
  {
    ticks_per_bar_provider_ = std::move (ticks_per_bar_provider);
  }
  void
  set_ticks_per_beat_provider (TicksPerBeatProvider ticks_per_beat_provider)
  {
    ticks_per_beat_provider_ = std::move (ticks_per_beat_provider);
  }
  void
  set_frames_per_tick_provider (FramesPerTickProvider frames_per_tick_provider)
  {
    frames_per_tick_provider_ = std::move (frames_per_tick_provider);
  }

  /**
   * @brief Returns whether any snapping is enabled.
   */
  bool any_snap () const { return snap_to_grid_ || snap_to_events_; }

  static int get_ticks_from_length_and_type (
    utils::NoteLength length,
    utils::NoteType   type,
    int               ticks_per_bar,
    int               ticks_per_beat);

  /**
   * Gets a snap point's length in ticks.
   */
  int get_snap_ticks () const;

  /**
   * @brief Get the snap length in frames.
   */
  double get_snap_frames () const;

  /**
   * Gets a the default length in ticks.
   */
  int get_default_ticks () const;

  /**
   * Returns the grid intensity as a human-readable string.
   */
  static utils::Utf8String stringize_length_and_type (
    utils::NoteLength note_length,
    utils::NoteType   note_type);

  /**
   * Returns the grid intensity as a human-readable string.
   */
  utils::Utf8String stringize () const;

  /**
   * Returns the next or previous SnapGrid point.
   *
   * @param[out] ret_pos Output position, if found.
   * @param ticks Position to search for.
   * @param return_prev 1 to return the previous element or 0 to return the
   * next.
   *
   * @return Whether successful.
   */
  bool
  get_nearby_snap_point (Position &ret_pos, double ticks, bool return_prev);

  /**
   * @brief Get closest snap point.
   *
   * @param pivot_pos Position to check snap points for.
   * @param track Track, used when moving things in the timeline. If keep
   * offset is on and this is passed, the objects in the track will be taken
   * into account. If keep offset is on and this is nullptr, all applicable
   * objects will be taken into account. Not used if keep offset is off.
   * @param region Region, used when moving things in the editor. Same
   * behavior as @ref track.
   * @param closest_sp Position to set.
   *
   * @return Whether a snap point was found or not.
   */
  bool get_closest_snap_point (
    const Position &pivot_pos,
    Track *         track,
    Region *        region,
    Position       &closest_sp) const;

  /**
   * @brief Get next snap point.
   *
   * Must only be called on positive positions.
   *
   * @param pivot_pos Position to check snap points for.
   * @param track Track, used when moving things in the timeline. If keep
   * offset is on and this is passed, the objects in the track will be taken
   * into account. If keep offset is on and this is nullptr, all applicable
   * objects will be taken into account. Not used if keep offset is off.
   * @param region Region, used when moving things in the editor. Same
   * behavior as @ref track.
   * @param next_snap_point Position to set.
   *
   * @return Whether a snap point was found or not.
   */
  [[gnu::hot]] bool get_next_snap_point (
    const Position &pivot_pos,
    Track *         track,
    Region *        region,
    Position       &next_sp) const;

  /**
   * Gets the previous snap point.
   *
   * Must only be called on positive positions.
   *
   * TODO check what happens if there is no snap point & improve this API
   *
   * @param pivot_pos Position to check snap points for.
   * @param track Track, used when moving things in the timeline. If keep
   * offset is on and this is passed, the objects in the track will be taken
   * into account. If keep offset is on and this is nullptr, all applicable
   * objects will be taken into account. Not used if keep offset is off.
   * @param region Region, used when moving things in the editor. Same
   * behavior as @p track.
   * @param prev_snap_point The position to set.
   *
   * @return Whether a snap point was found or not.
   */
  [[gnu::hot]] bool get_prev_snap_point (
    const Position &pivot_pos,
    Track *         track,
    Region *        region,
    Position       &prev_sp) const;

  /**
   * @brief Common logic for get_prev_snap_point and get_next_snap_point.
   */
  bool get_prev_or_next_snap_point (
    const Position &pivot_pos,
    Track *         track,
    Region *        region,
    Position       &out_pos,
    bool            get_prev_point) const;

  /**
   * Snaps @p pivot_pos using given options.
   *
   * @param start_pos The previous position (ie, the position the drag started
   * at. This is only used when the "keep offset" setting is on.
   * @param track Track, used when moving things in the timeline. If keep offset
   * is on and this is passed, the objects in the track will be taken into
   * account. If keep offset is on and this is nullptr, all applicable objects
   * will be taken into account. Not used if keep offset is off.
   * @param region Region, used when moving things in the editor. Same behavior
   * as @p track.
   */
  Position snap (
    const Position  &pivot_pos,
    const Position * start_pos,
    Track *          track,
    Region *         region) const;

  /**
   * @brief Snaps @p pivot_pos using given options.
   *
   * @param sg
   */
  Position snap_simple (const Position &pivot_pos) const
  {
    return snap (pivot_pos, nullptr, nullptr, nullptr);
  }

  /**
   * Returns an end position that is 1 snap point away from @p start_pos.
   *
   * @param start_pos Start Position.
   */
  Position get_snapped_end_position (const Position &start_pos) const;

private:
  static constexpr auto kTypeKey = "type"sv;
  static constexpr auto kSnapNoteLengthKey = "snapNoteLength"sv;
  static constexpr auto kSnapNoteTypeKey = "snapNoteType"sv;
  static constexpr auto kSnapAdaptiveKey = "snapAdaptive"sv;
  static constexpr auto kDefaultNoteLengthKey = "defaultNoteLength"sv;
  static constexpr auto kDefaultNoteTypeKey = "defaultNoteType"sv;
  static constexpr auto kDefaultAdaptiveKey = "defaultAdaptive"sv;
  static constexpr auto kLengthTypeKey = "lengthType"sv;
  static constexpr auto kSnapToGridKey = "snapToGrid"sv;
  static constexpr auto kKeepOffsetKey = "keepOffset"sv;
  static constexpr auto kSnapToEventsKey = "snapToEvents"sv;
  friend void           to_json (nlohmann::json &j, const SnapGrid &p)
  {
    j = nlohmann::json{
      { kTypeKey,              p.type_                     },
      { kSnapNoteLengthKey,    p.snap_note_length_         },
      { kSnapNoteTypeKey,      p.snap_note_type_           },
      { kSnapAdaptiveKey,      p.snap_adaptive_            },
      { kDefaultNoteLengthKey, p.default_note_length_      },
      { kDefaultNoteTypeKey,   p.default_note_type_        },
      { kDefaultAdaptiveKey,   p.default_adaptive_         },
      { kLengthTypeKey,        p.length_type_              },
      { kSnapToGridKey,        p.snap_to_grid_             },
      { kKeepOffsetKey,        p.snap_to_grid_keep_offset_ },
      { kSnapToEventsKey,      p.snap_to_events_           }
    };
  }
  friend void from_json (const nlohmann::json &j, SnapGrid &p)
  {
    j.at (kTypeKey).get_to (p.type_);
    j.at (kSnapNoteLengthKey).get_to (p.snap_note_length_);
    j.at (kSnapNoteTypeKey).get_to (p.snap_note_type_);
    j.at (kSnapAdaptiveKey).get_to (p.snap_adaptive_);
    j.at (kDefaultNoteLengthKey).get_to (p.default_note_length_);
    j.at (kDefaultNoteTypeKey).get_to (p.default_note_type_);
    j.at (kDefaultAdaptiveKey).get_to (p.default_adaptive_);
    j.at (kLengthTypeKey).get_to (p.length_type_);
    j.at (kSnapToGridKey).get_to (p.snap_to_grid_);
    j.at (kKeepOffsetKey).get_to (p.snap_to_grid_keep_offset_);
    j.at (kSnapToEventsKey).get_to (p.snap_to_events_);
  }

public:
  Type type_{};

  /**
   * If this is on, the snap note length will be determined automatically
   * based on the current zoom level.
   *
   * The snap note type still applies.
   */
  bool snap_adaptive_ = false;

  /** Snap note length. */
  utils::NoteLength snap_note_length_{};

  /** Snap note type. */
  utils::NoteType snap_note_type_{ utils::NoteType::Normal };

  /** Whether to snap to the grid. */
  bool snap_to_grid_ = true;

  /**
   * Whether to keep the offset when moving items.
   *
   * This requires @ref snap_to_grid to be enabled.
   */
  bool snap_to_grid_keep_offset_ = false;

  /** Whether to snap to events. */
  bool snap_to_events_ = false;

  /** Default note length. */
  utils::NoteLength default_note_length_{};
  /** Default note type. */
  utils::NoteType default_note_type_{ utils::NoteType::Normal };

  /**
   * If this is on, the default note length will be determined automatically
   * based on the current zoom level.
   *
   * The default note type still applies.
   *
   * TODO this will be done after v1.
   */
  bool default_adaptive_ = false;

  /**
   * See NoteLengthType.
   */
  NoteLengthType length_type_ = NoteLengthType::NOTE_LENGTH_LINK;

private:
  FramesPerTickProvider frames_per_tick_provider_;
  TicksPerBarProvider   ticks_per_bar_provider_;
  TicksPerBeatProvider  ticks_per_beat_provider_;
};

}; // namespace zrythm::gui
