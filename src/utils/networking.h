// SPDX-FileCopyrightText: © 2021, 2023-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <filesystem>
#include <functional>
#include <future>
#include <memory>
#include <string>
#include <variant>

#include "utils/utf8_string.h"

/**
 * @brief Networking utilities.
 */
namespace zrythm::networking
{

class URL final
{
public:
  class MultiPartMimeObject final
  {
  public:
    MultiPartMimeObject (
      std::string           name,
      std::filesystem::path filepath,
      std::string           mimetype);

    friend class URL;

  private:
    std::string           name_;
    std::filesystem::path filepath_;
    std::string           mimetype_;
  };

public:
  URL (const std::string &url);

  ~URL ();

  URL (const URL &) = delete;
  URL &operator= (const URL &) = delete;
  URL (URL &&) noexcept;
  URL &operator= (URL &&) noexcept;

  /**
   * Returns the contents of the page synchronously.
   *
   * @param timeout Timeout in milliseconds. If 0, this will use whatever
   default setting the OS chooses. If a negative number, it will be infinite.
   *
   * @throw ZrythmException on error.
   */
  std::string get_page_contents (int timeout = 0);

  /**
   * Posts the given JSON to the URL without any authentication.
   *
   * @param timeout Timeout, in milliseconds.
   * @param mime_objects Optional files to send as multi-part mime objects.
   *
   * @throw ZrythmException on error.
   */
  utils::Utf8String post_json_no_auth (
    const std::string                         &json_str,
    int                                        timeout,
    std::initializer_list<MultiPartMimeObject> mime_objects);

  using AsyncStringResult = std::variant<std::string, std::exception_ptr>;
  using GetContentsAsyncCallback = std::function<void (AsyncStringResult)>;

  std::future<void>
  get_page_contents_async (int timeout_ms, GetContentsAsyncCallback callback);

private:
  class Impl;
  std::unique_ptr<Impl> impl_;
};

}
