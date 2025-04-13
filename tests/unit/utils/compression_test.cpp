// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "utils/compression.h"
#include "utils/gtest_wrapper.h"

TEST (CompressionTest, CompressDecompress)
{
  // Test basic compression/decompression
  {
    QByteArray original = "Hello, World!";
    auto       compressed =
      zrythm::utils::compression::compress_to_base64_str (original);
    auto decompressed =
      zrythm::utils::compression::decompress_string_from_base64 (compressed);
    ASSERT_TRUE (!decompressed.empty ());
    EXPECT_EQ (original, decompressed.c_str ());
  }

  // Test empty string
  {
    QByteArray empty = "";
    auto compressed = zrythm::utils::compression::compress_to_base64_str (empty);
    auto decompressed =
      zrythm::utils::compression::decompress_string_from_base64 (compressed);
    EXPECT_EQ (empty, decompressed.c_str ());
  }

  // Test long string with repeating patterns
  {
    QByteArray repeating = "AAAAABBBBBCCCCCDDDDD";
    auto       compressed =
      zrythm::utils::compression::compress_to_base64_str (repeating);
    auto decompressed =
      zrythm::utils::compression::decompress_string_from_base64 (compressed);
    EXPECT_EQ (repeating, decompressed.c_str ());
  }

  // Test string with special characters
  {
    QByteArray special = "!@#$%^&*()_+{}[]|\\:;\"'<>,.?/~`";
    auto       compressed =
      zrythm::utils::compression::compress_to_base64_str (special);
    auto decompressed =
      zrythm::utils::compression::decompress_string_from_base64 (compressed);
    EXPECT_EQ (special, decompressed.c_str ());
  }

  // Test Unicode string
  {
    QByteArray unicode = "Hello, 世界! Γεια σας! привет!";
    auto       compressed =
      zrythm::utils::compression::compress_to_base64_str (unicode);
    auto decompressed =
      zrythm::utils::compression::decompress_string_from_base64 (compressed);
    EXPECT_EQ (unicode, decompressed.c_str ());
  }

  // Test large string
  {
    QByteArray large (10000, 'X');
    auto compressed = zrythm::utils::compression::compress_to_base64_str (large);
    auto decompressed =
      zrythm::utils::compression::decompress_string_from_base64 (compressed);
    EXPECT_EQ (large, decompressed.c_str ());
  }

  // Test specific base64 string from original compression.cpp
  {
    constexpr auto base64str =
      "KLUv/WD9AnUOAFZbUyMwbfQIFW0RJmyBF/FDW4mxy3PWcCllsvblWy3R5YSREZHCAUcASwBKAErj+nDQTon7a3So8Xs7RbXhEeJuaDzlvknjr87gSOMqJ7Tu4ye8hPEjaLmVguNeja1HdELc/KhPhgTjQOSwEMrAjV+CWI4EUvHPIe6rnkVZmIUpRllqURRbEkxRCcGckfsqlVCWuk24T8JYslSiFmNN4zwqwRqHITz53PdRVOI0rh85hbiXxTBrOU3TMIZJPIoTjT9yyb0kyuM8bC1lBSSQ+14Y8U8qHs6pgpzQejTt5NzqpUdHdYOP/ODhnFppmrTgIWHkVk9hp0V9CSHo5CKHhXQ2oltpMS3+ORT3PjodDmJhhAxY4QL9B/WI26GhcbXuHxiLj+4qt8Md+DpxX3dI0X+KDPlPG9zvKUca0xmn41aJ1VhCbzxFnFPiZoAJ3yTAATIgAEKEoDsDogrNGAhfeoVFSBqYBQOAp7tzAz1o7qZ9jhxn/kga5jsotLU2USfPjD9svHl1iszWkhCi+H7U8n4CYnsxnCGIlnHYAf3I99SOqDDDIfByVyRuCvgWGCLCKjSwbXilzoaNC2Mdd0X0Aal2QycD4f01Ofy9V4I/AQ==";

    auto decompressed =
      zrythm::utils::compression::decompress_string_from_base64 (base64str);
    ASSERT_TRUE (!decompressed.empty ());
    EXPECT_TRUE (
      std::string (decompressed.c_str ())
        .ends_with (",\"markerTrackVisibilityIndex\":0}}"));
  }
}
