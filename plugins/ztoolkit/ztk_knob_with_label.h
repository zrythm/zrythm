/*
 * Copyright (C) 2019 Alexandros Theodotou <alex at zrythm dot org>
 *
 * This file is part of Zrythm
 *
 * Zrythm is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * Zrythm is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU General Affero Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

/**
 * \file
 *
 * Knob with label on top.
 */

#ifndef __Z_TOOLKIT_ZTK_KNOB_WITH_LABEL_H__
#define __Z_TOOLKIT_ZTK_KNOB_WITH_LABEL_H__

#include "ztoolkit/ztk.h"

typedef struct ZtkKnob ZtkKnob;
typedef struct ZtkLabel ZtkLabel;

/**
 * @addtogroup zplugins
 *
 * @{
 */

/**
 * Knob with a label on top.
 */
typedef struct ZtkKnobWithLabel
{
  /** Base widget. */
  ZtkWidget         base;

  /** Child knob. */
  ZtkKnob *         knob;

  /** Child label. */
  ZtkLabel *        label;

} ZtkKnobWithLabel;

/**
 * Creates a new knob with a label on top.
 *
 * @param rect The rectangle to draw this in. The
 *   knob and label will be adjusted so that they fit
 *   this exactly.
 */
ZtkKnobWithLabel *
ztk_knob_with_label_new (
  PuglRect * rect,
  ZtkKnob *  knob,
  ZtkLabel * label);

/**
 * @}
 */

#endif
