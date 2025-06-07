// SPDX-FileCopyrightText: © 2019-2021, 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "gui/backend/backend/audio_clip_editor.h"
#include "gui/backend/backend/automation_editor.h"
#include "gui/backend/backend/chord_editor.h"
#include "gui/backend/backend/piano_roll.h"
#include "structure/arrangement/arranger_object_span.h"
#include "structure/arrangement/region.h"

class ArrangerSelections;

/**
 * @addtogroup gui_backend
 *
 * @{
 */

#define CLIP_EDITOR (PROJECT->clip_editor_)

/**
 * Backend for the clip editor part of the UI.
 */
class ClipEditor final : public QObject
{
  Q_OBJECT
  QML_ELEMENT
  Q_PROPERTY (
    QVariant region READ getRegion WRITE setRegion NOTIFY regionChanged)
  Q_PROPERTY (QVariant track READ getTrack NOTIFY trackChanged)
  Q_PROPERTY (PianoRoll * pianoRoll READ getPianoRoll CONSTANT FINAL)
  Q_PROPERTY (ChordEditor * chordEditor READ getChordEditor CONSTANT FINAL)
  Q_PROPERTY (
    AudioClipEditor * audioEditor READ getAudioClipEditor CONSTANT FINAL)
  Q_PROPERTY (
    AutomationEditor * automationEditor READ getAutomationEditor CONSTANT FINAL)

  using ArrangerObjectRegistry = structure::arrangement::ArrangerObjectRegistry;
  using TrackResolver = structure::tracks::TrackResolver;
  using Region = structure::arrangement::Region;
  using RegionPtrVariant = structure::arrangement::RegionPtrVariant;
  using TrackPtrVariant = structure::tracks::TrackPtrVariant;
  using TrackUuid = structure::tracks::TrackUuid;
  using ArrangerObject = structure::arrangement::ArrangerObject;

public:
  ClipEditor (
    ArrangerObjectRegistry &reg,
    TrackResolver           track_resolver,
    QObject *               parent = nullptr);

  // ============================================================================
  // QML Interface
  // ============================================================================

  PianoRoll *        getPianoRoll () const { return piano_roll_; }
  ChordEditor *      getChordEditor () const { return chord_editor_; }
  AudioClipEditor *  getAudioClipEditor () const { return audio_clip_editor_; }
  AutomationEditor * getAutomationEditor () const { return automation_editor_; }

  QVariant getRegion () const
  {
    if (has_region ())
      {
        return QVariant::fromStdVariant (get_region ().value ());
      }

    return {};
  }
  void             setRegion (QVariant region);
  Q_INVOKABLE void unsetRegion ();
  Q_SIGNAL void    regionChanged (QVariant region);

  QVariant      getTrack () const;
  Q_SIGNAL void trackChanged (QVariant track);

  // ============================================================================

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
   */
  void set_region (Region::Uuid region_id)
  {
    if (region_id_.has_value () && region_id_.value () == region_id)
      return;

    region_id_ = region_id;
    Q_EMIT regionChanged (QVariant::fromStdVariant (get_region ().value ()));
  };

  bool has_region () const { return region_id_.has_value (); }

  std::optional<RegionPtrVariant> get_region () const;
  std::optional<Region::Uuid>     get_region_id () const { return region_id_; }

  // ArrangerObjectSpan get_arranger_selections ();

  std::optional<TrackPtrVariant> get_track () const;

  std::optional<TrackUuid> get_track_id () const;
  /**
   * @brief Unsets the region if it belongs to the given track.
   */
  void unset_region_if_belongs_to_track (const TrackUuid &track_id);

  /**
   * To be called when recalculating the graph.
   */
  void set_caches ();

  friend void init_from (
    ClipEditor            &obj,
    const ClipEditor      &other,
    utils::ObjectCloneType clone_type)

  {
    obj.region_id_ = other.region_id_;
    obj.audio_clip_editor_ = other.audio_clip_editor_;
    obj.automation_editor_ = other.automation_editor_;
    obj.chord_editor_ = other.chord_editor_;
    obj.track_ = other.track_;
  }

private:
  static constexpr auto kRegionIdKey = "regionId"sv;
  static constexpr auto kPianoRollKey = "pianoRoll"sv;
  static constexpr auto kAutomationEditorKey = "automationEditor"sv;
  static constexpr auto kChordEditorKey = "chordEditor"sv;
  static constexpr auto kAudioClipEditorKey = "audioClipEditor"sv;
  friend void           to_json (nlohmann::json &j, const ClipEditor &editor)
  {
    j[kRegionIdKey] = editor.region_id_;
    j[kPianoRollKey] = editor.piano_roll_;
    j[kAutomationEditorKey] = editor.automation_editor_;
    j[kChordEditorKey] = editor.chord_editor_;
    j[kAudioClipEditorKey] = editor.audio_clip_editor_;
  }
  friend void from_json (const nlohmann::json &j, ClipEditor &editor)
  {
    j.at (kRegionIdKey).get_to (editor.region_id_);
    editor.piano_roll_ = new PianoRoll (&editor);
    editor.automation_editor_ = new AutomationEditor (&editor);
    editor.chord_editor_ = new ChordEditor (&editor);
    editor.audio_clip_editor_ = new AudioClipEditor (&editor);
    j.at (kPianoRollKey).get_to (*editor.piano_roll_);
    j.at (kAutomationEditorKey).get_to (*editor.automation_editor_);
    j.at (kChordEditorKey).get_to (*editor.chord_editor_);
    j.at (kAudioClipEditorKey).get_to (*editor.audio_clip_editor_);
  }

public:
  ArrangerObjectRegistry &object_registry_;
  TrackResolver           track_resolver_;

  /** Region currently attached to the clip editor. */
  std::optional<ArrangerObject::Uuid> region_id_;

  PianoRoll *        piano_roll_{};
  AudioClipEditor *  audio_clip_editor_{};
  AutomationEditor * automation_editor_{};
  ChordEditor *      chord_editor_{};

  /* --- caches --- */
  // std::optional<RegionPtrVariant> region_;

  std::optional<TrackPtrVariant> track_;
};

/**
 * @}
 */
