// SPDX-FileCopyrightText: © 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <chrono>
#include <fstream>
#include <thread>

#include "utils/gtest_wrapper.h"
#include "utils/io_utils.h"
#include "utils/networking.h"
#include "utils/utf8_string.h"

#include <httplib.h>

using namespace std::chrono_literals;
namespace zrythm::networking
{

class NetworkingTest : public ::testing::Test
{
protected:
  void SetUp () override
  {
    std::promise<int> port_promise;
    auto              port_future = port_promise.get_future ();
    server_thread_ = std::thread ([this, &port_promise] () mutable {
      int port = svr_.bind_to_any_port ("0.0.0.0");
      port_promise.set_value (port);
      svr_.listen_after_bind ();
    });
    port_ = port_future.get ();
  }

  void TearDown () override
  {
    svr_.stop ();
    if (server_thread_.joinable ())
      server_thread_.join ();
  }

  httplib::Server  svr_;
  std::atomic<int> port_;
  std::thread      server_thread_;

  std::string get_base_url () const
  {
    return fmt::format ("http://127.0.0.1:{}", port_.load ());
  }
};

TEST_F (NetworkingTest, BasicUrlConstruction)
{
  EXPECT_NO_THROW ({ networking::URL url ("https://example.com"); });
}

TEST_F (NetworkingTest, GetPageContents)
{
  svr_.Get ("/", [] (const httplib::Request &, httplib::Response &res) {
    res.set_content ("mock page contents", "text/plain");
  });

  networking::URL url (get_base_url ());
  auto            contents = url.get_page_contents ();
  EXPECT_EQ (contents, "mock page contents");
}

TEST_F (NetworkingTest, GetPageContentsWithTimeout)
{
  svr_.Get ("/timeout", [] (const httplib::Request &, httplib::Response &res) {
    res.set_content ("timeout test", "text/plain");
  });

  networking::URL url (get_base_url () + "/timeout");
  auto            contents = url.get_page_contents (5000);
  EXPECT_EQ (contents, "timeout test");
}

TEST_F (NetworkingTest, PostJsonNoAuth)
{
  svr_.Post ("/post", [] (const httplib::Request &req, httplib::Response &res) {
    res.set_content ("{\"received\": true}", "application/json");
  });

  // Create temporary test directory using thread-safe temporary directory
  auto tmp_dir_obj = zrythm::utils::io::make_tmp_dir ();
  auto tmp_dir =
    utils::Utf8String::from_qstring (tmp_dir_obj->path ()).to_path ();
  auto test_file = tmp_dir / "test.txt";
  std::ofstream (test_file) << "test content";

  networking::URL url (get_base_url () + "/post");
  std::string     json = R"({"test": "value"})";

  auto response = url.post_json_no_auth (
    json, 5000,
    { networking::URL::MultiPartMimeObject ("file", test_file, "text/plain") });
  EXPECT_EQ (response, R"({"received": true})");

  // QTemporaryDir auto-removes when destroyed, no manual cleanup needed
}

TEST_F (NetworkingTest, AsyncGetContents)
{
  svr_.Get ("/async", [] (const httplib::Request &, httplib::Response &res) {
    res.set_content ("async test", "text/plain");
  });

  networking::URL    url (get_base_url () + "/async");
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

  EXPECT_EQ (future.wait_for (10s), std::future_status::ready);
}
}
