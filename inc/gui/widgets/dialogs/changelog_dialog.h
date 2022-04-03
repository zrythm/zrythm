/*
 * Copyright (C) 2021 Alexandros Theodotou <alex at zrythm dot org>
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
 * along with Zrythm.  If not, see <https://www.gnu.org/licenses/>.
 */

/**
 * \file
 *
 * Changelog dialog.
 */

#ifndef __GUI_WIDGETS_CHANGELOG_DIALOG_H__
#define __GUI_WIDGETS_CHANGELOG_DIALOG_H__

#ifdef HAVE_CHANGELOG

#  include <gtk/gtk.h>

/**
 * @addtogroup widgets
 *
 * @{
 */

/**
 * Creates and displays the changelog dialog.
 */
void
changelog_dialog_widget_run (GtkWindow * parent);

/**
 * @}
 */

#endif /* HAVE_CHANGELOG */

#endif
