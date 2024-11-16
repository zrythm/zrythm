// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "common/dsp/nameable_object.h"
#include "common/utils/rt_thread_id.h"
#include "common/utils/ui.h"
#include "gui/backend/backend/actions/arranger_selections.h"
#include "gui/backend/backend/project.h"
#include "gui/backend/backend/zrythm.h"

#include <fmt/printf.h>

void
NameableObject::gen_escaped_name ()
{
  escaped_name_ = QString::fromStdString (name_).toHtmlEscaped ().toStdString ();
}

void
NameableObject::init_loaded_base ()
{
  gen_escaped_name ();
}

void
NameableObject::set_name (const std::string &name, bool fire_events)
{
  name_ = name;
  gen_escaped_name ();

  if (fire_events)
    {
      std::visit (
        [&] (auto &&obj) {
          Q_EMIT (obj->nameChanged (QString::fromStdString (name)));
        },
        convert_to_variant<NameableObjectPtrVariant> (this));
    }
}

void
NameableObject::set_name_with_action (const std::string &name)
{
  /* validate */
  if (!validate_name (name))
    {
// TODO
#if 0
      std::string msg =
        fmt::sprintf (QObject::tr ("Invalid object name %s"), name);
      ui_show_error_message (QObject::tr ("Invalid Name"), msg);
#endif
      return;
    }

  std::visit (
    [&] (auto &&self) {
      auto clone_obj = self->clone_unique ();

      /* prepare the before/after selections to create the undoable action */
      clone_obj->set_name (name, false);

      try
        {
          UNDO_MANAGER->perform (EditArrangerSelectionsAction::create (
            *self, *clone_obj, ArrangerSelectionsAction::EditType::Name, false));
        }
      catch (const ZrythmException &e)
        {
          e.handle (QObject::tr ("Failed to rename object"));
        }
    },
    convert_to_variant<NameableObjectPtrVariant> (this));
}