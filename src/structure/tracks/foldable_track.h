// SPDX-FileCopyrightText: © 2021, 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "structure/tracks/track.h"

namespace zrythm::structure::tracks
{
/**
 * @brief Abstract base for a foldable track.
 */
class FoldableTrack : virtual public Track
{
public:
  enum class MixerStatus
  {
    Muted,
    Soloed,
    ImpliedSoloed,
    Listened,
  };

public:
  FoldableTrack () noexcept { }
  ~FoldableTrack () noexcept override = default;
  Z_DISABLE_COPY_MOVE (FoldableTrack)

  /**
   * Used to check if soloed/muted/etc.
   */
  bool is_status (MixerStatus status) const;

  /**
   * Returns whether @p child is a direct folder child of this track.
   */
  bool is_direct_child (const Track &child) const;

  /**
   * Returns whether @p child is a folder child of this track.
   */
  bool is_child (const Track &child) const;

  /**
   * Adds to the size recursively.
   *
   * This must only be called from the lowest-level foldable track.
   */
  void add_to_size (int delta);

  /**
   * Sets track folded and optionally adds the action to the undo stack.
   */
  void
  set_folded (bool folded, bool trigger_undo, bool auto_select, bool fire_events);

private:
  static constexpr auto kSizeKey = "size"sv;
  static constexpr auto kFoldedKey = "folded"sv;
  friend void           to_json (nlohmann::json &j, const FoldableTrack &track)
  {
    j[kSizeKey] = track.size_;
    j[kFoldedKey] = track.folded_;
  }
  friend void from_json (const nlohmann::json &j, FoldableTrack &track)
  {
    j.at (kSizeKey).get_to (track.size_);
    j.at (kFoldedKey).get_to (track.folded_);
  }

public:
  /**
   * @brief Number of tracks inside this track.
   *
   * 1 means no other tracks inside.
   */
  int size_ = 1;

  /** Whether currently folded. */
  bool folded_ = false;
};

} // namespace zrythm::structure::tracks
