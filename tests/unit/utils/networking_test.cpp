// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <fstream>

#include "utils/gtest_wrapper.h"
#include "utils/networking.h"

TEST (NetworkingTest, BasicUrlConstruction)
{
  EXPECT_NO_THROW ({ networking::URL url ("https://example.com"); });
}

TEST (NetworkingTest, GetPageContents)
{
  networking::URL url ("https://example.com");
  auto            contents = url.get_page_contents ();
  EXPECT_FALSE (contents.empty ());
  // Just verify we got some content back
  EXPECT_TRUE (contents.length () > 0);
}

TEST (NetworkingTest, GetPageContentsWithTimeout)
{
  networking::URL url ("https://example.com");
  // Test with 5 second timeout
  auto contents = url.get_page_contents (5000);
  EXPECT_FALSE (contents.empty ());
}

TEST (NetworkingTest, PostJsonNoAuth)
{
  networking::URL url ("https://httpbin.org/post");
  std::string     json = R"({"test": "value"})";

  // Create a test file for multipart
  auto tmp_dir = fs::temp_directory_path () / "networking_test";
  fs::create_directory (tmp_dir);
  auto test_file = tmp_dir / "test.txt";
  std::ofstream (test_file) << "test content";

  EXPECT_NO_THROW ({
    url.post_json_no_auth (
      json, 5000,
      { networking::URL::MultiPartMimeObject ("file", test_file, "text/plain") });
  });

  fs::remove_all (tmp_dir);
}

TEST (NetworkingTest, AsyncGetContents)
{
  networking::URL    url ("https://example.com");
  std::promise<void> promise;
  auto               future = promise.get_future ();

  url.get_page_contents_async (
    5000, [&promise] (networking::URL::AsyncStringResult result) {
      if (std::holds_alternative<std::string> (result))
        {
          auto &contents = std::get<std::string> (result);
          EXPECT_FALSE (contents.empty ());
          promise.set_value ();
        }
      else
        {
          promise.set_exception (std::get<std::exception_ptr> (result));
        }
    });

  EXPECT_EQ (
    future.wait_for (std::chrono::seconds (10)), std::future_status::ready);
}
