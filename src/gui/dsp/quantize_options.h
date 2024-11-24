// SPDX-FileCopyrightText: © 2019-2021, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef __AUDIO_QUANTIZE_OPTIONS_H__
#define __AUDIO_QUANTIZE_OPTIONS_H__

#include "gui/dsp/snap_grid.h"

#include "dsp/position.h"

#define QUANTIZE_OPTIONS_IS_EDITOR(qo) \
  (PROJECT->quantize_opts_editor_.get () == qo)
#define QUANTIZE_OPTIONS_IS_TIMELINE(qo) \
  (PROJECT->quantize_opts_timeline_.get () == qo)
#define QUANTIZE_OPTIONS_TIMELINE (PROJECT->quantize_opts_timeline_)
#define QUANTIZE_OPTIONS_EDITOR (PROJECT->quantize_opts_editor_)

class Transport;

namespace zrythm::gui::dsp
{

class QuantizeOptions
  final : public zrythm::utils::serialization::ISerializable<QuantizeOptions>
{
public:
  static constexpr auto MAX_SNAP_POINTS = 120096;
  using NoteLength = utils::NoteLength;
  using NoteType = utils::NoteType;
  using Position = zrythm::dsp::Position;

public:
  QuantizeOptions () = default;
  QuantizeOptions (NoteLength note_length) { init (note_length); }

  void init (NoteLength note_length);

  /**
   * Updates snap points.
   */
  void update_quantize_points (const Transport &transport);

  float get_swing () const;

  float get_amount () const;

  float get_randomization () const;

  void set_swing (float swing);

  void set_amount (float amount);

  void set_randomization (float randomization);

  /**
   * Returns the grid intensity as a human-readable string.
   */
  static std::string to_string (NoteLength note_length, NoteType note_type);

  /**
   * Quantizes the given Position using the given
   * QuantizeOptions.
   *
   * This assumes that the start/end check has been
   * done already and it ignores the adjust_start and
   * adjust_end options.
   *
   * @return The amount of ticks moved (negative for
   *   backwards).
   */
  double quantize_position (Position * pos);

  DECLARE_DEFINE_FIELDS_METHOD ();

private:
  const Position * get_prev_point (Position * pos) const;
  const Position * get_next_point (Position * pos) const;

public:
  /**
   * Quantize points.
   *
   * These only take into account note_length, note_type and swing. They don't
   * take into account the amount % or randomization ticks.
   *
   * Not to be serialized.
   */
  std::vector<Position> q_points_;

  /** See SnapGrid. */
  NoteLength note_length_{};

  /** See SnapGrid. */
  NoteType note_type_{};

  /** Percentage to apply quantize (0-100). */
  float amount_ = 100.f;

  /** Adjust start position or not (only applies to objects with length. */
  bool adj_start_ = true;

  /** Adjust end position or not (only applies to objects with length. */
  bool adj_end_ = false;

  /** Swing amount (0-100). */
  float swing_ = 0.f;

  /** Number of ticks for randomization. */
  double rand_ticks_ = 0.f;
};

}; // namespace zrythm::dsp

#endif
