/*
 * Copyright (C) 2018-2020 Alexandros Theodotou <alex at zrythm dot org>
 *
 * This file is part of Zrythm
 *
 * Zrythm is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Zrythm is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

/**
 * \file
 *
 * Draw functions for ArrangerWidget - split to make
 * file smaller.
 *
 * TODO
 */

#ifndef __GUI_WIDGETS_ARRANGER_DRAW_H__
#define __GUI_WIDGETS_ARRANGER_DRAW_H__

#include "gui/widgets/arranger.h"

#include <gtk/gtk.h>

/**
 * @addtogroup widgets
 *
 * @{
 */

void
arranger_snapshot (GtkWidget * widget, GtkSnapshot * snapshot);

/**
 * @}
 */

#endif
