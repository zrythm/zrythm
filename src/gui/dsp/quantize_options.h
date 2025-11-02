// SPDX-FileCopyrightText: © 2019-2021, 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "utils/note_type.h"
#include "utils/pcg_rand.h"
#include "utils/units.h"

#define QUANTIZE_OPTIONS_IS_EDITOR(qo) \
  (PROJECT->quantize_opts_editor_.get () == qo)
#define QUANTIZE_OPTIONS_IS_TIMELINE(qo) \
  (PROJECT->quantize_opts_timeline_.get () == qo)
#define QUANTIZE_OPTIONS_TIMELINE (PROJECT->quantize_opts_timeline_)
#define QUANTIZE_OPTIONS_EDITOR (PROJECT->quantize_opts_editor_)

namespace zrythm::engine::session
{
class Transport;
}

namespace zrythm::gui::old_dsp
{

class QuantizeOptions final
{
public:
  static constexpr auto MAX_SNAP_POINTS = 120096;
  using NoteLength = utils::NoteLength;
  using NoteType = utils::NoteType;

public:
  QuantizeOptions () = default;
  QuantizeOptions (NoteLength note_length) { init (note_length); }

  void init (NoteLength note_length);

  /**
   * Updates snap points.
   */
  void update_quantize_points (const zrythm::dsp::Transport &transport);

  float get_swing () const;

  float get_amount () const;

  float get_randomization () const;

  void set_swing (float swing);

  void set_amount (float amount);

  void set_randomization (float randomization);

  /**
   * Returns the grid intensity as a human-readable string.
   */
  static utils::Utf8String
  to_string (NoteLength note_length, NoteType note_type);

  /**
   * Quantizes the given Position using the given
   * QuantizeOptions.
   *
   * This assumes that the start/end check has been
   * done already and it ignores the adjust_start and
   * adjust_end options.
   *
   * @return The resulting position and amount of ticks moved (negative for
   *   backwards).
   */
  std::pair<units::precise_tick_t, double>
  quantize_position (units::precise_tick_t pos);

private:
  static constexpr auto kNoteLengthKey = "noteLength"sv;
  static constexpr auto kNoteTypeKey = "noteType"sv;
  static constexpr auto kAdjustStartKey = "adjustStart"sv;
  static constexpr auto kAdjustEndKey = "adjustEnd"sv;
  static constexpr auto kAmountKey = "amount"sv;
  static constexpr auto kSwingKey = "swing"sv;
  static constexpr auto kRandomizationTicksKey = "randTicks"sv;
  friend void           to_json (nlohmann::json &j, const QuantizeOptions &p)
  {
    j = nlohmann::json{
      { kNoteLengthKey,         p.note_length_         },
      { kNoteTypeKey,           p.note_type_           },
      { kAdjustStartKey,        p.adjust_start_        },
      { kAdjustEndKey,          p.adjust_end_          },
      { kAmountKey,             p.amount_              },
      { kSwingKey,              p.swing_               },
      { kRandomizationTicksKey, p.randomization_ticks_ },
    };
  }
  friend void from_json (const nlohmann::json &j, QuantizeOptions &p)
  {
    j.at (kNoteLengthKey).get_to (p.note_length_);
    j.at (kNoteTypeKey).get_to (p.note_type_);
    j.at (kAdjustStartKey).get_to (p.adjust_start_);
    j.at (kAdjustEndKey).get_to (p.adjust_end_);
    j.at (kAmountKey).get_to (p.amount_);
    j.at (kSwingKey).get_to (p.swing_);
    j.at (kRandomizationTicksKey).get_to (p.randomization_ticks_);
  }

  units::precise_tick_t get_prev_point (units::precise_tick_t pos) const;
  units::precise_tick_t get_next_point (units::precise_tick_t pos) const;

public:
  /**
   * Quantize points.
   *
   * These only take into account note_length, note_type and swing. They don't
   * take into account the amount % or randomization ticks.
   *
   * Not to be serialized.
   */
  std::vector<units::precise_tick_t> q_points_;

  /** See SnapGrid. */
  NoteLength note_length_{};

  /** See SnapGrid. */
  NoteType note_type_{};

  /** Percentage to apply quantize (0-100). */
  float amount_ = 100.f;

  /** Adjust start position or not (only applies to objects with length. */
  bool adjust_start_ = true;

  /** Adjust end position or not (only applies to objects with length. */
  bool adjust_end_ = false;

  /** Swing amount (0-100). */
  float swing_ = 0.f;

  /** Number of ticks for randomization. */
  double randomization_ticks_ = 0.f;

  PCGRand rand_;
};

}; // namespace zrythm::dsp
