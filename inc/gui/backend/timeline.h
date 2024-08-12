// SPDX-FileCopyrightText: © 2020, 2023 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * @file
 *
 * Timeline backend.
 */

#ifndef __GUI_BACKEND_TIMELINE_H__
#define __GUI_BACKEND_TIMELINE_H__

#include "gui/backend/editor_settings.h"
#include "utils/icloneable.h"

/**
 * @addtogroup gui_backend
 *
 * @{
 */

#define PRJ_TIMELINE (PROJECT->timeline_)

/**
 * @brief Timeline settings.
 */
class Timeline final
    : public EditorSettings,
      public ICloneable<Timeline>,
      public ISerializable<Timeline>
{
public:
  DECLARE_DEFINE_FIELDS_METHOD ();

void init_after_cloning (const Timeline &other) override { *this = other; }

public:
/** Width of the left side of the timeline panel. */
int tracks_width_ = 0;
};

/**
 * @}
 */

#endif
