/*
 * Copyright (C) 2019 Alexandros Theodotou <alex at zrythm dot org>
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
 * Localization utils.
 *
 */

#ifndef __UTILS_LOCALIZATION_H__
#define __UTILS_LOCALIZATION_H__

#include <gtk/gtk.h>

/**
 * @addtogroup utils
 *
 * @{
 */

typedef enum LocalizationLanguage
{
  LL_ENGLISH,
  LL_ENGLISH_UK,
  LL_GERMAN,
  LL_FRENCH,
  LL_ITALIAN,
  LL_NORWEGIAN,
  LL_SPANISH,
  LL_JAPANESE,
  LL_PORTUGUESE,
  LL_RUSSIAN,
  LL_CHINESE,
  NUM_LL_LANGUAGES,
} LocalizationLanguage;

static const char * language_strings[] = {
  "en", "en_GB", "de", "fr", "it", "nb_NO", "es",
  "ja", "pt", "ru", "zh", };

/**
 * Returns the character string code for the
 * language (e.g. "fr").
 */
static inline const char *
localization_get_string_code (
  LocalizationLanguage lang)
{
  g_return_val_if_fail (
    lang >= LL_ENGLISH && lang < NUM_LL_LANGUAGES,
    NULL);

  return language_strings[lang];
}

/**
 * Returns the first locale found matching the given
 * language, or NULL if a locale for the given
 * language does not exist.
 *
 * Must be free'd with g_free().
 */
char *
localization_locale_exists (
  LocalizationLanguage lang);

/**
 * Sets the locale to the currently selected one and
 * inits gettext.
 *
 * Returns if a locale for the selected language
 * exists on the system or not.
 */
int
localization_init (void);

/**
 * @}
 */

#endif
