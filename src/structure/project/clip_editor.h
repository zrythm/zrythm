// SPDX-FileCopyrightText: © 2019-2021, 2024-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "structure/arrangement/clip.h"
#include "structure/project/audio_clip_editor.h"
#include "structure/project/automation_editor.h"
#include "structure/project/chord_editor.h"
#include "structure/project/midi_editor.h"
#include "structure/tracks/track.h"
#include "structure/tracks/track_fwd.h"
#include "utils/iobject_registry.h"

class ArrangerSelections;

namespace zrythm::structure::project
{

/**
 * @brief Backend for the clip editor part of the UI.
 */
class ClipEditor : public QObject
{
  Q_OBJECT
  Q_PROPERTY (
    zrythm::structure::arrangement::Clip * clipObject READ clip NOTIFY
      clipObjectChanged)
  Q_PROPERTY (QVariant track READ track NOTIFY clipObjectChanged)
  Q_PROPERTY (
    zrythm::structure::project::MidiEditor * midiEditor READ midiEditor CONSTANT
      FINAL)
  Q_PROPERTY (
    zrythm::structure::project::ChordEditor * chordEditor READ chordEditor
      CONSTANT FINAL)
  Q_PROPERTY (
    zrythm::structure::project::AudioClipEditor * audioEditor READ
      audioClipEditor CONSTANT FINAL)
  Q_PROPERTY (
    zrythm::structure::project::AutomationEditor * automationEditor READ
      automationEditor CONSTANT FINAL)
  QML_ELEMENT
  QML_UNCREATABLE ("")

  using TrackUuid = structure::tracks::TrackUuid;
  using ArrangerObject = structure::arrangement::ArrangerObject;
  using Track = structure::tracks::Track;

public:
  ClipEditor (utils::IObjectRegistry &reg, QObject * parent = nullptr);

  // ============================================================================
  // QML Interface
  // ============================================================================

  MidiEditor *      midiEditor () const { return midi_editor_.get (); }
  ChordEditor *     chordEditor () const { return chord_editor_.get (); }
  AudioClipEditor * audioClipEditor () const
  {
    return audio_clip_editor_.get ();
  }
  AutomationEditor * automationEditor () const
  {
    return automation_editor_.get ();
  }

  arrangement::Clip * clip () const;
  QVariant            track () const;
  Q_INVOKABLE void    setClip (QVariant clip, QVariant track);
  Q_INVOKABLE void    unsetClip ();
  Q_SIGNAL void       clipObjectChanged ();

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
  void set_clip (ArrangerObject::Uuid clip_id, TrackUuid track_id);

  bool has_clip () const { return clip_id_.has_value (); }

  std::optional<std::pair<ArrangerObject *, Track *>>
  get_clip_and_track () const;

  friend void init_from (
    ClipEditor            &obj,
    const ClipEditor      &other,
    utils::ObjectCloneType clone_type);

private:
  static constexpr auto kClipIdKey = "clipId"sv;
  static constexpr auto kMidiEditorKey = "midiEditor"sv;
  static constexpr auto kAutomationEditorKey = "automationEditor"sv;
  static constexpr auto kChordEditorKey = "chordEditor"sv;
  static constexpr auto kAudioClipEditorKey = "audioClipEditor"sv;
  friend void           to_json (nlohmann::json &j, const ClipEditor &editor);
  friend void           from_json (const nlohmann::json &j, ClipEditor &editor);

private:
  utils::IObjectRegistry &object_registry_;

  /** Clip currently attached to the clip editor. */
  std::optional<std::pair<ArrangerObject::Uuid, TrackUuid>> clip_id_;

  utils::QObjectUniquePtr<MidiEditor>       midi_editor_;
  utils::QObjectUniquePtr<AudioClipEditor>  audio_clip_editor_;
  utils::QObjectUniquePtr<AutomationEditor> automation_editor_;
  utils::QObjectUniquePtr<ChordEditor>      chord_editor_;
};

} // namespace zrythm::structure::project
