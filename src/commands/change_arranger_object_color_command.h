// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <cassert>
#include <optional>
#include <utility>

#include "structure/arrangement/arranger_object.h"
#include "utils/uuid_reference.h"

#include <QUndoCommand>

namespace zrythm::commands
{
/**
 * @brief Sets or clears an arranger object's user-set color.
 *
 * A nullopt color resets the object to its owner's color.
 *
 * The owning object is pinned with a UUID reference so it cannot be
 * deleted while the command is on the undo stack.
 *
 * @pre The owner carries an ArrangerObjectColor.
 */
class ChangeArrangerObjectColorCommand : public QUndoCommand
{

public:
  ChangeArrangerObjectColorCommand (
    structure::arrangement::ArrangerObject &owner,
    utils::IObjectRegistry                 &registry,
    std::optional<QColor>                   color_after)
      : QUndoCommand (
          color_after ? QObject::tr ("Change Object Color")
                      : QObject::tr ("Reset Object Color")),
        owner_ (owner), color_after_ (std::move (color_after)),
        uuid_ref_ (owner.raw_uuid (), registry)
  {
    assert (owner.color () != nullptr);
  }

  void undo () override { apply (color_before_); }
  void redo () override
  {
    auto * color_obj = owner_.color ();
    if (color_obj == nullptr)
      {
        return;
      }

    color_before_ =
      color_obj->useColor ()
        ? std::optional<QColor> (color_obj->color ())
        : std::nullopt;
    apply (color_after_);
  }

private:
  void apply (const std::optional<QColor> &color)
  {
    auto * color_obj = owner_.color ();
    if (color_obj == nullptr)
      {
        return;
      }

    if (color)
      {
        color_obj->setColor (*color);
      }
    else
      {
        color_obj->unsetColor ();
      }
  }

  structure::arrangement::ArrangerObject &owner_;
  std::optional<QColor>                   color_before_;
  std::optional<QColor>                   color_after_;
  utils::UuidReference                    uuid_ref_;
};

} // namespace zrythm::commands
