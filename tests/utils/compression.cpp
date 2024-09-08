// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "zrythm-test-config.h"

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN

#include "utils/compression.h"

#include "tests/helpers/zrythm_helper.h"

TEST_SUITE_BEGIN ("utils/compression");

TEST_CASE ("decompression")
{
  constexpr auto base64str =
    "KLUv/WD9AnUOAFZbUyMwbfQIFW0RJmyBF/FDW4mxy3PWcCllsvblWy3R5YSREZHCAUcASwBKAErj+nDQTon7a3So8Xs7RbXhEeJuaDzlvknjr87gSOMqJ7Tu4ye8hPEjaLmVguNeja1HdELc/KhPhgTjQOSwEMrAjV+CWI4EUvHPIe6rnkVZmIUpRllqURRbEkxRCcGckfsqlVCWuk24T8JYslSiFmNN4zwqwRqHITz53PdRVOI0rh85hbiXxTBrOU3TMIZJPIoTjT9yyb0kyuM8bC1lBSSQ+14Y8U8qHs6pgpzQejTt5NzqpUdHdYOP/ODhnFppmrTgIWHkVk9hp0V9CSHo5CKHhXQ2oltpMS3+ORT3PjodDmJhhAxY4QL9B/WI26GhcbXuHxiLj+4qt8Md+DpxX3dI0X+KDPlPG9zvKUca0xmn41aJ1VhCbzxFnFPiZoAJ3yTAATIgAEKEoDsDogrNGAhfeoVFSBqYBQOAp7tzAz1o7qZ9jhxn/kga5jsotLU2USfPjD9svHl1iszWkhCi+H7U8n4CYnsxnCGIlnHYAf3I99SOqDDDIfByVyRuCvgWGCLCKjSwbXilzoaNC2Mdd0X0Aal2QycD4f01Ofy9V4I/AQ==";

  GError * err = NULL;
  char *   res_str = compression_decompress_from_base64_str (base64str, &err);
  ASSERT_TRUE (
    g_str_has_suffix (res_str, ",\"markerTrackVisibilityIndex\":0}}"));
  g_free (res_str);
}

TEST_SUITE_END;