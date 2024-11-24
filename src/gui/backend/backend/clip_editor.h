// SPDX-FileCopyrightText: © 2019-2021, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef __GUI_BACKEND_CLIP_EDITOR_H__
#define __GUI_BACKEND_CLIP_EDITOR_H__

#include "gui/backend/backend/audio_clip_editor.h"
#include "gui/backend/backend/automation_editor.h"
#include "gui/backend/backend/chord_editor.h"
#include "gui/backend/backend/piano_roll.h"
#include "gui/dsp/region.h"
#include "gui/dsp/region_identifier.h"

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
class ClipEditor
  final : public zrythm::utils::serialization::ISerializable<ClipEditor>
{
  // Q_OBJECT
  // QML_ELEMENT
  // TODO
  // Q_PROPERTY(QVariant region READ get_region NOTIFY regionChanged)
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
  void
  set_region (std::optional<RegionPtrVariant> region_opt_var, bool fire_events);

  std::optional<RegionPtrVariant> get_region () const;

  std::optional<ClipEditorArrangerSelectionsPtrVariant>
  get_arranger_selections ();

  std::optional<TrackPtrVariant> get_track ();

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

  /* --- caches --- */
  std::optional<RegionPtrVariant> region_;

  std::optional<TrackPtrVariant> track_;
};

/**
 * @}
 */

#endif
