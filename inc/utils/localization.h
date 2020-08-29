/*
 * Copyright (C) 2019-2020 Alexandros Theodotou <alex at zrythm dot org>
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

#include <stdbool.h>

#include <gtk/gtk.h>

/**
 * @addtogroup utils
 *
 * @{
 */

typedef enum LocalizationLanguage
{
  LL_AR,
  LL_CS,
  LL_DA,
  LL_DE,
  LL_EN,
  LL_EN_GB,
  LL_EL,
  LL_ES,
  LL_ET,
  LL_FI,
  LL_FR,
  LL_GD,
  LL_GL,
  LL_HI,
  LL_IT,
  LL_JA,
  LL_KO,
  LL_NB_NO,
  LL_NL,
  LL_PL,
  LL_PT,
  LL_PT_BR,
  LL_RU,
  LL_SV,
  LL_ZH_HANS,
  LL_ZH_HANT,
  NUM_LL_LANGUAGES,
} LocalizationLanguage;

static const char * language_strings[] = {
  "ar",
  "cs",
  "da",
  "de",
  "en",
  "en_GB",
  "el",
  "es",
  "et",
  "fi",
  "fr",
  "gd",
  "gl",
  "hi",
  "it",
  "ja",
  "ko",
  "nb_NO",
  "nl",
  "pl",
  "pt",
  "pt_BR",
  "ru",
  "sv",
  "zh_Hans",
  "zh_Hant",
};

static const char * language_strings_full[] = {
  "اَلْعَرَبِيَّةُ‎ [ar]",
  "Czech [cs]",
  "Dansk [da]",
  "Deutsch [de]",
  "English [en]",
  "English UK [en_GB]",
  "Ελληνικά [el]",
  "Español [es]",
  "Eeti [et]",
  "Suomi [fi]",
  "Français [fr]",
  "Gaelic [gd]",
  "Galego [gl]",
  "हिन्दी [hi]",
  "Italiano [it]",
  "日本語 [ja]",
  "한국어 [ko]",
  "Bokmål [nb_NO]",
  "Nederlands [nl]",
  "Polski [pl]",
  "Português [pt]",
  "Português BR [pt_BR]",
  "русский [ru]",
  "Svenska [sv]",
  "简体中文 [zh_Hans]",
  "繁體中文 [zh_Hant]",
};

static inline const char *
localization_get_full_string (
  LocalizationLanguage lang)
{
  g_return_val_if_fail (
    lang >= 0 && lang < NUM_LL_LANGUAGES,
    NULL);

  return language_strings_full[lang];
}

/**
 * Returns the character string code for the
 * language (e.g. "fr").
 */
static inline const char *
localization_get_string_code (
  LocalizationLanguage lang)
{
  g_return_val_if_fail (
    lang >= 0 && lang < NUM_LL_LANGUAGES,
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
 * @param use_locale Use the user's local instead of
 *   the Zrythm settings.
 *
 * Returns if a locale for the selected language
 * exists on the system or not.
 */
bool
localization_init (
  bool use_locale,
  bool print_debug_messages);

/**
 * @}
 */

#endif
