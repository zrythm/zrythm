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
 * Returns the 2-character string code for the
 * language (e.g. "fr").
 *
 * @param str is a preallocated buffer.
 */
void
localization_get_string_code (
  UiLanguage lang,
  char *     str)
{
  switch (lang)
    {
    case UI_ENGLISH:
      str[0] = 'e';
      str[1] = 'n';
      break;
    case UI_GERMAN:
      str[0] = 'd';
      str[1] = 'e';
      break;
    case UI_FRENCH:
      str[0] = 'f';
      str[1] = 'r';
      break;
    case UI_ITALIAN:
      str[0] = 'i';
      str[1] = 't';
      break;
    case UI_NORWEGIAN:
      str[0] = 'n';
      str[1] = 'b';
      break;
    case UI_SPANISH:
      str[0] = 'e';
      str[1] = 's';
      break;
    case UI_JAPANESE:
      str[0] = 'j';
      str[1] = 'a';
      break;
    case UI_PORTUGUESE:
      str[0] = 'p';
      str[1] = 't';
      break;
    case UI_RUSSIAN:
      str[0] = 'r';
      str[1] = 'u';
      break;
    case UI_CHINESE:
      str[0] = 'z';
      str[1] = 'h';
      break;
    default:
      g_warn_if_reached ();
      break;
    }
  str[2] = '\0';
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
  /* get available locales on the system */
  FILE *fp;
  char path[1035];
  char * installed_locales[3000];
  int num_installed_locales = 0;

  /* Open the command for reading. */
  fp = popen("locale -a", "r");
  if (fp == NULL) {
    g_error ("localization_init: popen failed");
    exit(1);
  }

  /* Read the output a line at a time - output it. */
  while (fgets(path, sizeof(path)-1, fp) != NULL) {
    installed_locales[num_installed_locales++] =
      g_strdup (path);
  }

  /* close */
  pclose (fp);

  /* get selected locale */
  GSettings * prefs =
    g_settings_new ("org.zrythm.preferences");
  UiLanguage lang =
    g_settings_get_enum (
      prefs,
      "language");
  g_object_unref (G_OBJECT (prefs));

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

  if (match)
    {
      g_message ("setting locale to %s",
                 match);
      setlocale (LC_ALL, match);
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
