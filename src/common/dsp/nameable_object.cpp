// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "gui/cpp/backend/actions/arranger_selections.h"
#include "gui/cpp/backend/event.h"
#include "gui/cpp/backend/event_manager.h"
#include "gui/cpp/backend/project.h"
#include "gui/cpp/backend/zrythm.h"
#include "gui/cpp/gtk_widgets/zrythm_app.h"

#include "common/dsp/nameable_object.h"
#include "common/utils/rt_thread_id.h"
#include "common/utils/ui.h"
#include <fmt/printf.h>
#include <glibmm.h>

void
NameableObject::gen_escaped_name ()
{
  escaped_name_ = Glib::Markup::escape_text (name_);
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
      EVENTS_PUSH (EventType::ET_ARRANGER_OBJECT_CHANGED, this);
    }
}

void
NameableObject::set_name_with_action (const std::string &name)
{
  /* validate */
  if (!validate_name (name))
    {
      std::string msg = fmt::sprintf (_ ("Invalid object name %s"), name);
      ui_show_error_message (_ ("Invalid Name"), msg.c_str ());
      return;
    }

  auto clone_obj = clone_shared_with_variant<NameableObjectVariant> (this);

  /* prepare the before/after selections to create the undoable action */
  clone_obj->set_name (name, false);

  try
    {
      UNDO_MANAGER->perform (EditArrangerSelectionsAction::create (
        *this, *clone_obj, ArrangerSelectionsAction::EditType::Name, false));
    }
  catch (const ZrythmException &e)
    {
      e.handle (_ ("Failed to rename object"));
    }
}