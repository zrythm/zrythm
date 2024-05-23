/*
 * SPDX-FileCopyrightText: © 2022 Alexandros Theodotou <alex@zrythm.org>
 *
 * SPDX-License-Identifier: LicenseRef-ZrythmLicense
 */

#include "zrythm-config.h"

#include "utils/pango.h"
#include "utils/string.h"
#include "zrythm.h"

#include <fontconfig/fontconfig.h>
#include <pango/pangoft2.h>

PangoLayout *
z_pango_create_layout_from_description (
  GtkWidget *            widget,
  PangoFontDescription * descr)
{
  PangoLayout * layout = NULL;

  char * str = pango_font_description_to_string (descr);
  g_debug ("font description: %s", str);

#ifdef HAVE_BUNDLED_DSEG
  if (string_contains_substr (str, "DSEG"))
    {
      FcConfig * fc_config = FcConfigCreate ();

      /* add fonts/zrythm dir to find DSEG font */
      char * fontdir = zrythm_get_dir (ZRYTHM_DIR_SYSTEM_FONTSDIR);
      FcConfigAppFontAddDir (fc_config, (const unsigned char *) fontdir);
      g_free (fontdir);
      FcConfigBuildFonts (fc_config);

      PangoFontMap * font_map =
        pango_cairo_font_map_new_for_font_type (CAIRO_FONT_TYPE_FT);
      pango_fc_font_map_set_config (PANGO_FC_FONT_MAP (font_map), fc_config);

      PangoContext * context =
        pango_font_map_create_context (PANGO_FONT_MAP (font_map));
      PangoLayout * pangoLayout = pango_layout_new (context);

      FcPattern *   pattern = FcPatternCreate ();
      FcObjectSet * os = FcObjectSetBuild (FC_FAMILY, NULL);
      FcFontSet *   fs = FcFontList (fc_config, pattern, os);
      FcPatternDestroy (pattern);
      FcObjectSetDestroy (os);
      for (int i = 0; i < fs->nfont; i++)
        {
          guchar *               font_name = FcNameUnparse (fs->fonts[i]);
          PangoFontDescription * desc =
            pango_font_description_from_string ((gchar *) font_name);
          pango_font_map_load_font (PANGO_FONT_MAP (font_map), context, desc);
          pango_font_description_free (desc);
          g_debug ("fontname: %s", font_name);
          g_free (font_name);
        }

      layout = pangoLayout;

      g_object_unref (context);
      g_object_unref (font_map);
      FcConfigDestroy (fc_config);
    }
  else
    {
#endif
      layout = gtk_widget_create_pango_layout (widget, NULL);
#ifdef HAVE_BUNDLED_DSEG
    }
#endif
  g_free (str);

  g_return_val_if_fail (layout, NULL);

  pango_layout_set_font_description (layout, descr);

  /* try a small test */
  int test_w, test_h;
  pango_layout_set_markup (layout, "test", -1);
  pango_layout_get_pixel_size (layout, &test_w, &test_h);

  return layout;
}
