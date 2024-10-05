// SPDX-FileCopyrightText: © 2021, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef __AUDIO_FOLDABLE_TRACK_H__
#define __AUDIO_FOLDABLE_TRACK_H__

#include "common/dsp/track.h"

TYPEDEF_STRUCT_UNDERSCORED (FolderChannelWidget);

/**
 * @brief Abstract base for a foldable track.
 */
class FoldableTrack : virtual public Track, public ISerializable<FoldableTrack>
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
  // Rule of 0

  virtual ~FoldableTrack () = default;

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

protected:
  DECLARE_DEFINE_BASE_FIELDS_METHOD ();

public:
  /**
   * @brief Number of tracks inside this track.
   *
   * 1 means no other tracks inside.
   */
  int size_ = 1;

  /** Whether currently folded. */
  bool folded_ = false;

  FolderChannelWidget * folder_ch_widget_ = nullptr;
};

#endif /* __AUDIO_FOLDABLE_TRACK_H__ */