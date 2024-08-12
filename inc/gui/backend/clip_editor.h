// SPDX-FileCopyrightText: © 2019-2021, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef __GUI_BACKEND_CLIP_EDITOR_H__
#define __GUI_BACKEND_CLIP_EDITOR_H__

#include "dsp/region.h"
#include "dsp/region_identifier.h"
#include "gui/backend/audio_clip_editor.h"
#include "gui/backend/automation_editor.h"
#include "gui/backend/chord_editor.h"
#include "gui/backend/piano_roll.h"

class ArrangerSelections;

/**
 * @addtogroup gui_backend
 *
 * @{
 */

#define CLIP_EDITOR (&PROJECT->clip_editor_)

/**
 * Clip editor serializable backend.
 *
 * The actual widgets should reflect the information here.
 */
class ClipEditor final : public ISerializable<ClipEditor>
{
public:
  /**
   * Inits the ClipEditor after a Project is loaded.
   */
  void init_loaded ();

  /**
   * Inits the clip editor.
   */
  void init ();

  /**
   * Sets the track and refreshes the piano roll widgets.
   *
   * To be called only from GTK threads.
   */
  void set_region (Region * region, bool fire_events);

  /**
   * @brief Get the region object dynamic-casted to @p T.
   *
   * @tparam T
   * @return T*
   */
  template <FinalRegionSubclass T> T * get_region () const;

  Region * get_region () const;

  ArrangerSelections * get_arranger_selections ();

  Track * get_track ();

  /**
   * To be called when recalculating the graph.
   */
  void set_caches ();

  DECLARE_DEFINE_FIELDS_METHOD ();

public:
  /** Region currently attached to the clip editor. */
  RegionIdentifier region_id_;

  /** Whether @ref region_id is a valid region. */
  bool has_region_ = false;

  PianoRoll        piano_roll_;
  AudioClipEditor  audio_clip_editor_;
  AutomationEditor automation_editor_;
  ChordEditor      chord_editor_;

  /** Flag used by rulers when region first changes. */
  bool region_changed_ = false;

  /* --- caches --- */
  Region * region_ = nullptr;

  Track * track_ = nullptr;
};

/**
 * @}
 */

#endif
