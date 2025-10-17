// SPDX-FileCopyrightText: © 2021, 2023-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <filesystem>

#include "utils/types.h"
#include "utils/utf8_string.h"

#include "juce_wrapper.h"

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
      std::string name,
      fs::path    filepath,
      std::string mimetype);

    friend class URL;

  private:
    std::string name_;
    fs::path    filepath_;
    std::string mimetype_;
  };

public:
  URL (const std::string &url);

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
  juce::URL                url_;
  GetContentsAsyncCallback get_contents_callback_;
};

}
