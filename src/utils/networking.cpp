// SPDX-FileCopyrightText: © 2021, 2023-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "zrythm-config.h"

#include "utils/env.h"
#include "utils/exceptions.h"
#include "utils/format_juce.h"
#include "utils/logger.h"
#include "utils/networking.h"

#include <juce_core/juce_core.h>

using zrythm::utils::exceptions::ZrythmException;

namespace zrythm::networking
{

class URL::Impl
{
public:
  explicit Impl (const std::string &url) : url_ (url) { }

  juce::URL url_;
};

URL::URL (const std::string &url) : impl_ (std::make_unique<Impl> (url))
{
  z_debug ("creating URL {}", impl_->url_.toString (true));
}

URL::~URL () = default;

URL::URL (URL &&) noexcept = default;
URL &
URL::operator= (URL &&) noexcept = default;

URL::MultiPartMimeObject::MultiPartMimeObject (
  const std::string           name,
  const std::filesystem::path filepath,
  const std::string           mimetype)
    : name_ (name), filepath_ (filepath), mimetype_ (mimetype)
{
}

std::string
URL::get_page_contents (int timeout)
{
  z_debug ("getting page contents for {}...", impl_->url_.toString (true));

  auto options =
    juce::URL::InputStreamOptions (juce::URL::ParameterHandling::inAddress)
      .withConnectionTimeoutMs (timeout)
      .withExtraHeaders ("User-Agent: zrythm-daw/" PACKAGE_VERSION "\r\n");

  if (auto stream = impl_->url_.createInputStream (options))
    {
      auto res = stream->readEntireStreamAsString ();
      if (res.isEmpty ())
        {
          throw ZrythmException (
            fmt::format (
              "Failed to get page contents for {}.",
              impl_->url_.toString (true)));
        }
      return utils::Utf8String::from_juce_string (res).str ();
    }

  throw ZrythmException (
    fmt::format (
      "Failed to create input stream for {}.", impl_->url_.toString (true)));
}

utils::Utf8String
URL::post_json_no_auth (
  const std::string                         &json_str,
  int                                        timeout,
  std::initializer_list<MultiPartMimeObject> mime_objects)
{
  z_debug ("sending data...");

  juce::String juce_str (json_str);
  auto         url = impl_->url_.withDataToUpload (
    "data", "",
    juce::MemoryBlock (juce_str.toRawUTF8 (), juce_str.getNumBytesAsUTF8 ()),
    "application/json");
  for (const auto &mime_object : mime_objects)
    {
      url = url.withFileToUpload (
        mime_object.name_,
        utils::Utf8String::from_path (mime_object.filepath_).to_juce_file (),
        mime_object.mimetype_);
    }

  int                   status_code = 0;
  juce::StringPairArray response_headers;
  auto                  options =
    juce::URL::InputStreamOptions (juce::URL::ParameterHandling::inPostData)
      .withConnectionTimeoutMs (timeout)
      .withExtraHeaders (
        "User-Agent: zrythm-daw/" PACKAGE_VERSION
        "\r\n"
        "Accept: application/json\r\n"
        "Content-Type: multipart/form-data\r\n")
      .withStatusCode (&status_code)
      .withResponseHeaders (&response_headers);

  if (auto stream = url.createInputStream (options))
    {
      z_info ("Success! status code: {}", status_code);
      auto res = stream->readEntireStreamAsString ();
      return utils::Utf8String::from_juce_string (res);
    }
  else
    {
      // Request failed
      auto errorMessage = fmt::format ("HTTP Status Code: {}", status_code);
      if (response_headers.containsKey ("Error"))
        {
          errorMessage += fmt::format ("\nError: {}", response_headers["Error"]);
        }

      throw ZrythmException (
        fmt::format ("Failed to create input stream:\n{}", errorMessage));
    }
}

std::future<void>
URL::get_page_contents_async (
  const int                timeout_ms,
  GetContentsAsyncCallback callback)
{
  return std::async (std::launch::async, [this, timeout_ms, callback] () {
    try
      {
        auto contents = get_page_contents (timeout_ms);
        callback (contents);
      }
    catch (const ZrythmException &e)
      {
        callback (std::current_exception ());
      }
  });
}

}
