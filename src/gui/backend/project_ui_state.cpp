// SPDX-FileCopyrightText: © 2018-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "gui/backend/project_ui_state.h"

namespace zrythm::gui
{
ProjectUiState::ProjectUiState (utils::QObjectUniquePtr<Project> &&project)
    : project_ (std::move (project)),
      tool_ (new gui::backend::ArrangerTool (this)),
      clip_editor_ (
        utils::make_qobject_unique<ClipEditor> (
          project_->get_arranger_object_registry (),
          [&] (const auto &id) {
            return project_->get_track_registry ().find_by_id_or_throw (id);
          },
          this)),
      timeline_ (
        utils::make_qobject_unique<structure::arrangement::Timeline> (this))
{
  project_->setParent (this);
}

Project *
ProjectUiState::project () const
{
  return project_.get ();
}

structure::arrangement::Timeline *
ProjectUiState::timeline () const
{
  return timeline_.get ();
}

gui::backend::ArrangerTool *
ProjectUiState::tool () const
{
  return tool_.get ();
}

ClipEditor *
ProjectUiState::clipEditor () const
{
  return clip_editor_.get ();
}
}
