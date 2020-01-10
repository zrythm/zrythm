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

#include "config.h"

#include <locale.h>

#include "settings/settings.h"
#include "utils/localization.h"
#include "utils/string.h"
#include "zrythm.h"

#include <glib/gi18n.h>

/**
 * Returns the character string code for the
 * language (e.g. "fr").
 */
char *
localization_get_string_code (
  UiLanguage lang)
{
  switch (lang)
    {
    case UI_ENGLISH:
      return g_strdup ("en");
    case UI_GERMAN:
      return g_strdup ("de");
    case UI_FRENCH:
      return g_strdup ("fr");
    case UI_ITALIAN:
      return g_strdup ("it");
    case UI_NORWEGIAN:
      return g_strdup ("nb_NO");
    case UI_SPANISH:
      return g_strdup ("es");
    case UI_JAPANESE:
      return g_strdup ("ja");
    case UI_PORTUGUESE:
      return g_strdup ("pt");
    case UI_RUSSIAN:
      return g_strdup ("ru");
    case UI_CHINESE:
      return g_strdup ("zh");
    default:
      g_return_val_if_reached (NULL);
    }
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
  UiLanguage lang)
{
  /* get available locales on the system */
  FILE *fp;
  char path[1035];
  char * installed_locales[3000];
  int num_installed_locales = 0;

  /* Open the command for reading. */
  fp = popen ("locale -a", "r");
  if (fp == NULL)
    {
      g_error ("localization_init: popen failed");
      exit (1);
    }

  /* Read the output a line at a time - output it. */
  while (fgets(path, sizeof(path)-1, fp) != NULL) {
    installed_locales[num_installed_locales++] =
      g_strdup_printf (
        "%s", g_strchomp (path));
  }

  /* close */
  pclose (fp);

#define IS_MATCH(caps,code) \
  case UI_##caps: \
    match = string_array_contains_substr ( \
          installed_locales, \
          num_installed_locales, \
          code); \
    break;

  char * match = NULL;
  switch (lang)
    {
    IS_MATCH(ENGLISH,"en_");
    IS_MATCH(GERMAN,"de_");
    IS_MATCH(FRENCH,"fr_");
    IS_MATCH(ITALIAN,"it_");
    IS_MATCH(NORWEGIAN,"nb_");
    IS_MATCH(SPANISH,"es_");
    IS_MATCH(JAPANESE,"ja_");
    IS_MATCH(PORTUGUESE,"pt_");
    IS_MATCH(RUSSIAN,"ru_");
    IS_MATCH(CHINESE,"zh_");
    default:
      g_warn_if_reached ();
      break;
    }

#undef IS_MATCH

  return match;
}

/**
 * Sets the locale to the currently selected one and
 * inits gettext.
 *
 * Returns if a locale for the selected language
 * exists on the system or not.
 */
int
localization_init ()
{
#ifdef _WIN32
  /* TODO */
  setlocale (LC_ALL, "C");
#else

  /* get selected locale */
  GSettings * prefs =
    g_settings_new ("org.zrythm.Zrythm.preferences");
  UiLanguage lang =
    g_settings_get_enum (
      prefs,
      "language");
  g_object_unref (G_OBJECT (prefs));

  char * match =
    localization_locale_exists (lang);

  if (match)
    {
      g_message ("setting locale to %s",
                 match);
      setlocale (LC_ALL, match);
      g_free (match);
    }
  else
    {
      g_warning ("No locale for selected language is "
                 "installed, using default");
      setlocale (LC_ALL, "C");
    }
#endif

#ifdef _WIN32
  bindtextdomain (
    GETTEXT_PACKAGE, "share/locale");
#else
  bindtextdomain (
    GETTEXT_PACKAGE, CONFIGURE_DATADIR "/locale");
#endif
  bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
  textdomain (GETTEXT_PACKAGE);

#ifdef _WIN32
  return 1;
#else
  return match != NULL;
#endif
}
