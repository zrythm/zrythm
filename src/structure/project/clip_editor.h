// SPDX-FileCopyrightText: © 2019-2021, 2024-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "structure/arrangement/audio_clip_editor.h"
#include "structure/arrangement/automation_editor.h"
#include "structure/arrangement/chord_editor.h"
#include "structure/arrangement/piano_roll.h"
#include "structure/tracks/track.h"
#include "structure/tracks/track_fwd.h"

class ArrangerSelections;

namespace zrythm::structure::project
{

/**
 * @brief Backend for the clip editor part of the UI.
 */
class ClipEditor : public QObject
{
  Q_OBJECT
  Q_PROPERTY (QVariant region READ region NOTIFY regionChanged)
  Q_PROPERTY (QVariant track READ track NOTIFY regionChanged)
  Q_PROPERTY (
    zrythm::structure::arrangement::PianoRoll * pianoRoll READ getPianoRoll
      CONSTANT FINAL)
  Q_PROPERTY (
    zrythm::structure::arrangement::ChordEditor * chordEditor READ
      getChordEditor CONSTANT FINAL)
  Q_PROPERTY (
    zrythm::structure::arrangement::AudioClipEditor * audioEditor READ
      getAudioClipEditor CONSTANT FINAL)
  Q_PROPERTY (
    zrythm::structure::arrangement::AutomationEditor * automationEditor READ
      getAutomationEditor CONSTANT FINAL)
  QML_ELEMENT
  QML_UNCREATABLE ("")

  using ArrangerObjectRegistry = structure::arrangement::ArrangerObjectRegistry;
  using TrackResolver = structure::tracks::TrackResolver;
  using TrackPtrVariant = structure::tracks::TrackPtrVariant;
  using TrackUuid = structure::tracks::TrackUuid;
  using ArrangerObject = structure::arrangement::ArrangerObject;
  using ArrangerObjectPtrVariant =
    structure::arrangement::ArrangerObjectPtrVariant;

public:
  ClipEditor (
    ArrangerObjectRegistry &reg,
    TrackResolver           track_resolver,
    QObject *               parent = nullptr);

  // ============================================================================
  // QML Interface
  // ============================================================================

  structure::arrangement::PianoRoll * getPianoRoll () const
  {
    return piano_roll_.get ();
  }
  structure::arrangement::ChordEditor * getChordEditor () const
  {
    return chord_editor_.get ();
  }
  structure::arrangement::AudioClipEditor * getAudioClipEditor () const
  {
    return audio_clip_editor_.get ();
  }
  structure::arrangement::AutomationEditor * getAutomationEditor () const
  {
    return automation_editor_.get ();
  }

  QVariant region () const
  {
    if (has_region ())
      {
        return QVariant::fromStdVariant (get_region_and_track ()->first);
      }

    return {};
  }
  QVariant track () const
  {
    if (has_region ())
      {
        return QVariant::fromStdVariant (get_region_and_track ()->second);
      }

    return {};
  }
  Q_INVOKABLE void setRegion (QVariant region, QVariant track);
  Q_INVOKABLE void unsetRegion ();
  Q_SIGNAL void    regionChanged (QVariant region);

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
  void set_region (ArrangerObject::Uuid region_id, TrackUuid track_id)
  {
    if (region_id_.has_value () && region_id_.value ().first == region_id)
      return;

    region_id_ = { region_id, track_id };
    Q_EMIT regionChanged (
      QVariant::fromStdVariant (get_region_and_track ().value ().first));
  };

  bool has_region () const { return region_id_.has_value (); }

  std::optional<std::pair<ArrangerObjectPtrVariant, TrackPtrVariant>>
  get_region_and_track () const;

  friend void init_from (
    ClipEditor            &obj,
    const ClipEditor      &other,
    utils::ObjectCloneType clone_type)

  {
    obj.region_id_ = other.region_id_;
    init_from (*obj.audio_clip_editor_, *other.audio_clip_editor_, clone_type);
    init_from (*obj.automation_editor_, *other.automation_editor_, clone_type);
    init_from (*obj.chord_editor_, *other.chord_editor_, clone_type);
    init_from (*obj.piano_roll_, *other.piano_roll_, clone_type);
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
    j.at (kPianoRollKey).get_to (*editor.piano_roll_);
    j.at (kAutomationEditorKey).get_to (*editor.automation_editor_);
    j.at (kChordEditorKey).get_to (*editor.chord_editor_);
    j.at (kAudioClipEditorKey).get_to (*editor.audio_clip_editor_);
  }

private:
  ArrangerObjectRegistry &object_registry_;
  TrackResolver           track_resolver_;

  /** Region currently attached to the clip editor. */
  std::optional<std::pair<ArrangerObject::Uuid, TrackUuid>> region_id_;

  utils::QObjectUniquePtr<structure::arrangement::PianoRoll> piano_roll_;
  utils::QObjectUniquePtr<structure::arrangement::AudioClipEditor>
    audio_clip_editor_;
  utils::QObjectUniquePtr<structure::arrangement::AutomationEditor>
    automation_editor_;
  utils::QObjectUniquePtr<structure::arrangement::ChordEditor> chord_editor_;
};

} // namespace zrythm::structure::project
