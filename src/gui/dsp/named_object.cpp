// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "gui/backend/backend/actions/arranger_selections_action.h"
#include "gui/backend/backend/project.h"
#include "gui/backend/backend/zrythm.h"
#include "gui/backend/ui.h"
#include "gui/dsp/named_object.h"
#include "utils/rt_thread_id.h"

#include <fmt/printf.h>

using namespace zrythm;

void
NamedObject::gen_escaped_name ()
{
  escaped_name_ = QString::fromStdString (name_).toHtmlEscaped ().toStdString ();
}

void
NamedObject::init_loaded_base ()
{
  gen_escaped_name ();
}

void
NamedObject::set_name (const std::string &name, bool fire_events)
{
  name_ = name;
  gen_escaped_name ();

  if (fire_events)
    {
      std::visit (
        [&] (auto &&obj) {
          Q_EMIT (obj->nameChanged (QString::fromStdString (name)));
        },
        convert_to_variant<NamedObjectPtrVariant> (this));
    }
}

void
NamedObject::set_name_with_action (const std::string &name)
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
          UNDO_MANAGER->perform (
            gui::actions::EditArrangerSelectionsAction::create (
              ArrangerObjectSpan{ self }, ArrangerObjectSpan{ clone_obj.get () },
              gui::actions::ArrangerSelectionsAction::EditType::Name, false)
              .release ());
        }
      catch (const ZrythmException &e)
        {
          e.handle (QObject::tr ("Failed to rename object"));
        }
    },
    convert_to_variant<NamedObjectPtrVariant> (this));
}
