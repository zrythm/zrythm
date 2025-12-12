// SPDX-FileCopyrightText: © 2018-2022, 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <functional>
#include <vector>

#include "dsp/tempo_map.h"
#include "utils/note_type.h"

#include <QObject>
#include <QtQmlIntegration/qqmlintegration.h>

namespace zrythm::dsp
{

/**
 * @brief Snap/grid information.
 */
class SnapGrid : public QObject
{
  Q_OBJECT
  Q_PROPERTY (
    bool snapAdaptive READ snapAdaptive WRITE setSnapAdaptive NOTIFY
      snapAdaptiveChanged)
  Q_PROPERTY (
    bool snapToGrid READ snapToGrid WRITE setSnapToGrid NOTIFY snapToGridChanged)
  Q_PROPERTY (
    bool snapToEvents READ snapToEvents WRITE setSnapToEvents NOTIFY
      snapToEventsChanged)
  Q_PROPERTY (
    bool keepOffset READ keepOffset WRITE setKeepOffset NOTIFY keepOffsetChanged)
  Q_PROPERTY (
    bool sixteenthsVisible READ sixteenthsVisible WRITE setSixteenthsVisible
      NOTIFY sixteenthsVisibleChanged)
  Q_PROPERTY (
    bool beatsVisible READ beatsVisible WRITE setBeatsVisible NOTIFY
      beatsVisibleChanged)
  Q_PROPERTY (
    zrythm::utils::NoteLength snapNoteLength READ snapNoteLength WRITE
      setSnapNoteLength NOTIFY snapNoteLengthChanged)
  Q_PROPERTY (
    zrythm::utils::NoteType snapNoteType READ snapNoteType WRITE setSnapNoteType
      NOTIFY snapNoteTypeChanged)
  Q_PROPERTY (
    NoteLengthType lengthType READ lengthType WRITE setLengthType NOTIFY
      lengthTypeChanged)
  Q_PROPERTY (QString snapString READ snapString NOTIFY snapChanged)
  QML_ELEMENT
  QML_UNCREATABLE ("")
  QML_EXTENDED_NAMESPACE (zrythm::utils)

public:
  enum class NoteLengthType
  {
    /** Link length with snap setting. */
    Link,

    /** Use last created object's length. */
    LastObject,

    /** Custom length. */
    Custom,
  };
  Q_ENUM (NoteLengthType)

  using SnapEventCallback =
    std::function<std::vector<double> (double start_ticks, double end_ticks)>;
  using LastObjectLengthProvider = std::function<double ()>;

  SnapGrid (
    const TempoMap          &tempo_map,
    utils::NoteLength        default_note_length,
    LastObjectLengthProvider last_object_length_provider,
    QObject *                parent = nullptr);

  // QML Properties
  bool snapAdaptive () const { return snap_adaptive_; }
  void setSnapAdaptive (bool adaptive);

  bool snapToGrid () const { return snap_to_grid_; }
  void setSnapToGrid (bool snap);

  bool snapToEvents () const { return snap_to_events_; }
  void setSnapToEvents (bool snap);

  bool keepOffset () const { return snap_to_grid_keep_offset_; }
  void setKeepOffset (bool keep);

  bool sixteenthsVisible () const { return sixteenths_visible_; }
  void setSixteenthsVisible (bool visible);

  bool beatsVisible () const { return beats_visible_; }
  void setBeatsVisible (bool visible);

  utils::NoteLength snapNoteLength () const { return snap_note_length_; }
  void              setSnapNoteLength (utils::NoteLength length);

  utils::NoteType snapNoteType () const { return snap_note_type_; }
  void            setSnapNoteType (utils::NoteType type);

  NoteLengthType lengthType () const { return length_type_; }
  void           setLengthType (NoteLengthType type);

  QString snapString () const;

  // QML Invokable Methods
  Q_INVOKABLE double snapWithoutStartTicks (double ticks)
  {
    return snap (units::ticks (ticks)).in (units::ticks);
  }
  Q_INVOKABLE double snapWithStartTicks (double ticks, double startTicks)
  {
    return snap (units::ticks (ticks), units::ticks (startTicks))
      .in (units::ticks);
  }
  Q_INVOKABLE double nextSnapPoint (double ticks) const;
  Q_INVOKABLE double prevSnapPoint (double ticks) const;
  Q_INVOKABLE double closestSnapPoint (double ticks) const;
  Q_INVOKABLE double snapTicks (int64_t ticks) const;
  Q_INVOKABLE double defaultTicks (int64_t ticks) const;

  /**
   * @brief
   *
   * @param ticks
   * @param start_ticks Used when "keep offset" is enabled.
   */
  units::precise_tick_t snap (
    units::precise_tick_t                ticks,
    std::optional<units::precise_tick_t> start_ticks = std::nullopt) const;

  // Callback configuration
  void set_event_callback (SnapEventCallback callback);
  void set_last_object_length_callback (LastObjectLengthProvider callback)
  {
    last_object_length_provider_ = callback;
  }
  void clear_callbacks ();

  static constexpr int get_ticks_from_length_and_type (
    utils::NoteLength length,
    utils::NoteType   type,
    int               ticks_per_bar,
    int               ticks_per_beat)
  {
    assert (ticks_per_beat > 0);
    assert (ticks_per_bar > 0);

    int ticks = 0;
    switch (length)
      {
      case utils::NoteLength::Bar:
        ticks = ticks_per_bar;
        break;
      case utils::NoteLength::Beat:
        ticks = ticks_per_beat;
        break;
      case utils::NoteLength::Note_2_1:
        ticks = 8 * TempoMap::get_ppq ();
        break;
      case utils::NoteLength::Note_1_1:
        ticks = 4 * TempoMap::get_ppq ();
        break;
      case utils::NoteLength::Note_1_2:
        ticks = 2 * TempoMap::get_ppq ();
        break;
      case utils::NoteLength::Note_1_4:
        ticks = TempoMap::get_ppq ();
        break;
      case utils::NoteLength::Note_1_8:
        ticks = TempoMap::get_ppq () / 2;
        break;
      case utils::NoteLength::Note_1_16:
        ticks = TempoMap::get_ppq () / 4;
        break;
      case utils::NoteLength::Note_1_32:
        ticks = TempoMap::get_ppq () / 8;
        break;
      case utils::NoteLength::Note_1_64:
        ticks = TempoMap::get_ppq () / 16;
        break;
      case utils::NoteLength::Note_1_128:
        ticks = TempoMap::get_ppq () / 32;
        break;
      }

    switch (type)
      {
      case utils::NoteType::Normal:
        break;
      case utils::NoteType::Dotted:
        ticks = 3 * ticks;
        assert (ticks % 2 == 0);
        ticks = ticks / 2;
        break;
      case utils::NoteType::Triplet:
        ticks = 2 * ticks;
        assert (ticks % 3 == 0);
        ticks = ticks / 3;
        break;
      }

    return ticks;
  }

  static QString stringize_length_and_type (
    utils::NoteLength note_length,
    utils::NoteType   note_type);

Q_SIGNALS:
  void snapAdaptiveChanged ();
  void snapToGridChanged ();
  void snapToEventsChanged ();
  void keepOffsetChanged ();
  void sixteenthsVisibleChanged ();
  void beatsVisibleChanged ();
  void snapNoteLengthChanged ();
  void snapNoteTypeChanged ();
  void lengthTypeChanged ();
  void snapChanged ();

private:
  static constexpr auto kSnapNoteLengthKey = "snapNoteLength"sv;
  static constexpr auto kSnapNoteTypeKey = "snapNoteType"sv;
  static constexpr auto kSnapAdaptiveKey = "snapAdaptive"sv;
  static constexpr auto kLengthTypeKey = "lengthType"sv;
  static constexpr auto kSnapToGridKey = "snapToGrid"sv;
  static constexpr auto kKeepOffsetKey = "keepOffset"sv;
  static constexpr auto kSnapToEventsKey = "snapToEvents"sv;
  friend void           to_json (nlohmann::json &j, const SnapGrid &p)
  {
    j = nlohmann::json{
      { kSnapNoteLengthKey, p.snap_note_length_         },
      { kSnapNoteTypeKey,   p.snap_note_type_           },
      { kSnapAdaptiveKey,   p.snap_adaptive_            },
      { kLengthTypeKey,     p.length_type_              },
      { kSnapToGridKey,     p.snap_to_grid_             },
      { kKeepOffsetKey,     p.snap_to_grid_keep_offset_ },
      { kSnapToEventsKey,   p.snap_to_events_           }
    };
  }
  friend void from_json (const nlohmann::json &j, SnapGrid &p)
  {
    j.at (kSnapNoteLengthKey).get_to (p.snap_note_length_);
    j.at (kSnapNoteTypeKey).get_to (p.snap_note_type_);
    j.at (kSnapAdaptiveKey).get_to (p.snap_adaptive_);
    j.at (kLengthTypeKey).get_to (p.length_type_);
    j.at (kSnapToGridKey).get_to (p.snap_to_grid_);
    j.at (kKeepOffsetKey).get_to (p.snap_to_grid_keep_offset_);
    j.at (kSnapToEventsKey).get_to (p.snap_to_events_);
  }

  bool get_prev_or_next_snap_point (
    double  pivot_ticks,
    double &out_ticks,
    bool    get_prev_point) const;
  std::vector<double>
  get_event_snap_points (double start_ticks, double end_ticks) const;

  /**
   * @brief Returns the currently active note length.
   *
   * If adaptive snap is turned on, it calculates the corresponding note length,
   * otherwise returns the specified note length as-is.
   */
  constexpr utils::NoteLength get_effective_note_length () const
  {
    return snap_adaptive_
             ? (sixteenths_visible_
                  ? utils::NoteLength::Note_1_16
                  : (beats_visible_ ? utils::NoteLength::Beat : utils::NoteLength::Bar))
             : snap_note_length_;
  }

private:
  bool              snap_adaptive_ = false;
  utils::NoteLength snap_note_length_{ utils::NoteLength::Note_1_4 };
  utils::NoteType   snap_note_type_{ utils::NoteType::Normal };

  /** Whether to snap to the grid. */
  bool snap_to_grid_ = true;

  /**
   * Whether to keep the offset when moving items.
   *
   * This requires @ref snap_to_grid to be enabled.
   */
  bool snap_to_grid_keep_offset_ = false;

  /**
   * @brief Whether to snap to events.
   *
   * This doesn't require snapping to be enabled, but snapping to events will
   * only be done if the distance to the event point is < snapTicks().
   */
  bool snap_to_events_ = false;

  NoteLengthType length_type_ = NoteLengthType::Link;
  bool           sixteenths_visible_ = false;
  bool           beats_visible_ = false;

  /**
   * @brief This is only used as the default object length eg, when creating new
   * objects.
   */
  utils::NoteLength default_note_length_;

  const TempoMap                  &tempo_map_;
  std::optional<SnapEventCallback> event_callback_;
  LastObjectLengthProvider         last_object_length_provider_;
};

} // namespace zrythm::dsp
