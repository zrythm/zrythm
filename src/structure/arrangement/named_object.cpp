// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "structure/arrangement/named_object.h"

#include <nlohmann/json.hpp>

namespace zrythm::structure::arrangement
{

void
init_from (
  ArrangerObjectName       &obj,
  const ArrangerObjectName &other,
  utils::ObjectCloneType    clone_type)
{
  obj.name_ = other.name_;
  obj.escaped_name_ = other.escaped_name_;
}

void
to_json (nlohmann::json &j, const ArrangerObjectName &named_object)
{
  // Serialize directly as a string value, not an object
  j = named_object.name_;
}

void
from_json (const nlohmann::json &j, ArrangerObjectName &named_object)
{
  // Deserialize from string value directly (not from object with "name" key)
  j.get_to (named_object.name_);
  named_object.gen_escaped_name ();
}

void
ArrangerObjectName::gen_escaped_name ()
{
  escaped_name_ = name_.escape_html ();
}

} // namespace zrythm::structure::arrangement
