// SPDX-FileCopyrightText: © 2024-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "structure/project/editor_settings.h"

#include <nlohmann/json_fwd.hpp>

namespace zrythm::structure::project
{

/**
 * @brief View state of the audio clip editor.
 *
 * Besides the inherited scroll/zoom settings, holds the time range selected
 * in the editor, which audio functions apply to (the whole clip when no
 * range is selected). The selection is transient: it is not serialized with
 * the project.
 */
class AudioClipEditor : public EditorSettings
{
  Q_OBJECT
  Q_PROPERTY (
    double selectionStartTick READ selectionStartTick WRITE
      setSelectionStartTick NOTIFY selectionStartTickChanged FINAL)
  Q_PROPERTY (
    double selectionEndTick READ selectionEndTick WRITE setSelectionEndTick
      NOTIFY selectionEndTickChanged FINAL)
  QML_ELEMENT

public:
  AudioClipEditor (QObject * parent = nullptr) : EditorSettings (parent) { }

  /**
   * @brief First tick of the selected range in the open clip's content
   * space, or a negative value when no range is selected.
   */
  double selectionStartTick () const { return selection_start_tick_; }
  void   setSelectionStartTick (double tick);

  /**
   * @brief Last tick of the selected range in the open clip's content
   * space (exclusive), or a negative value when no range is selected.
   */
  double selectionEndTick () const { return selection_end_tick_; }
  void   setSelectionEndTick (double tick);

Q_SIGNALS:
  void selectionStartTickChanged (double tick);
  void selectionEndTickChanged (double tick);

private:
  friend void init_from (
    AudioClipEditor       &obj,
    const AudioClipEditor &other,
    utils::ObjectCloneType clone_type);
  friend void to_json (nlohmann::json &j, const AudioClipEditor &editor);
  friend void from_json (const nlohmann::json &j, AudioClipEditor &editor);

  double selection_start_tick_ = -1.0;
  double selection_end_tick_ = -1.0;
};

}
