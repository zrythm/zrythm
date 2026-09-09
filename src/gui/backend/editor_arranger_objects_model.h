// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "gui/backend/unified_proxy_model.h"
#include "structure/project/clip_editor.h"

#include <QtQmlIntegration/qqmlintegration.h>

namespace zrythm::gui
{

/**
 * @brief Unified model of the arranger objects inside the open clip.
 *
 * Follows a ClipEditor: whenever the open clip changes (including being
 * closed), the model of the clip's arranger objects (MIDI notes, chord
 * objects or automation points, depending on the clip type) replaces
 * the previous source in the unified model. A clip that is destroyed
 * while open is dropped from the unified model before its destruction
 * begins.
 */
class EditorArrangerObjectsModel : public UnifiedProxyModel
{
  Q_OBJECT
  Q_PROPERTY (
    zrythm::structure::project::ClipEditor * clipEditor READ clipEditor WRITE
      setClipEditor NOTIFY clipEditorChanged)
  QML_ELEMENT
  QML_UNCREATABLE ("")

public:
  explicit EditorArrangerObjectsModel (QObject * parent = nullptr);

  zrythm::structure::project::ClipEditor * clipEditor () const;
  void          setClipEditor (zrythm::structure::project::ClipEditor * editor);
  Q_SIGNAL void clipEditorChanged ();

private:
  void sync_with_open_clip ();

  zrythm::structure::project::ClipEditor * clip_editor_ = nullptr;

  structure::arrangement::ArrangerObjectListModel * source_ = nullptr;
};

} // namespace zrythm::gui
