// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "structure/arrangement/named_object.h"

namespace zrythm::structure::arrangement
{
void
ArrangerObjectName::gen_escaped_name ()
{
  escaped_name_ = name_.escape_html ();
}
}
