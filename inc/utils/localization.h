/*
 * Copyright (C) 2019 Alexandros Theodotou <alex at zrythm dot org>
 *
 * This file is part of Zrythm
 *
 * Zrythm is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Zrythm is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Zrythm.  If not, see <https://www.gnu.org/licenses/>.
 */

/**
 * \file
 *
 * Localization utils.
 *
 */

#ifndef __UTILS_LOCALIZATION_H__
#define __UTILS_LOCALIZATION_H__

typedef enum UiLanguage
{
  UI_ENGLISH,
  UI_GERMAN,
  UI_FRENCH,
  UI_ITALIAN,
  UI_SPANISH,
  UI_JAPANESE,
  NUM_UI_LANGUAGES,
} UiLanguage;

/**
 * Sets the locale to the currently selected one and
 * inits gettext.
 */
void
localization_init ();

#endif
