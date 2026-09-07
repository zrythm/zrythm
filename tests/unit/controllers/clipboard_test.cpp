// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <random>

#include "controllers/clipboard.h"
#include "structure/project/clipboard_payload.h"
#include "utils/exceptions.h"
#include "utils/serialization.h"

#include "helpers/scoped_qcoreapplication.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace zrythm::controllers
{

class ClipboardTest
    : public ::testing::Test,
      public test_helpers::ScopedQCoreApplication
{
protected:
  // A fake OS clipboard: reading returns whatever was last written to it,
  // unless the test overrides the text to simulate an external write.
  QString     system_clipboard_text_;
  int         system_write_count_{ 0 };
  QStringList written_texts_;

  std::unique_ptr<Clipboard> make_clipboard ()
  {
    return std::make_unique<Clipboard> (
      [this] () { return system_clipboard_text_; },
      [this] (const QString &text) {
        system_clipboard_text_ = text;
        ++system_write_count_;
        written_texts_.append (text);
      });
  }

  static structure::project::ClipboardPayload
  make_payload (structure::project::ClipboardPayload::Type type)
  {
    nlohmann::json j;
    j[utils::serialization::kDocumentTypeKey] =
      structure::project::ClipboardPayload::kDocumentType;
    j["formatVersion"] = structure::project::ClipboardPayload::kFormatVersion;
    j["payloadType"] = [payload_type = type] {
      switch (payload_type)
        {
        case structure::project::ClipboardPayload::Type::ArrangerObjects:
          return std::string{ "arrangerObjects" };
        case structure::project::ClipboardPayload::Type::Tracks:
          return std::string{ "tracks" };
        case structure::project::ClipboardPayload::Type::Plugins:
          return std::string{ "plugins" };
        }
      throw std::invalid_argument{ "unreachable" };
    }();
    j["sourceProjectId"] = "project-a";
    j["metadata"] = nlohmann::json::object ();
    nlohmann::json registry = nlohmann::json::object ();
    registry[structure::project::ProjectRegistry::kPortsKey] =
      nlohmann::json::array ();
    registry[structure::project::ProjectRegistry::kParametersKey] =
      nlohmann::json::array ();
    registry[structure::project::ProjectRegistry::kPluginsKey] =
      nlohmann::json::array ();
    registry[structure::project::ProjectRegistry::kTracksKey] =
      nlohmann::json::array ();
    registry[structure::project::ProjectRegistry::kArrangerObjectsKey] =
      nlohmann::json::array ();
    registry[structure::project::ProjectRegistry::kFileAudioSourcesKey] =
      nlohmann::json::array ();
    j["registry"] = std::move (registry);
    j["roots"] = nlohmann::json::array ();
    return j.get<structure::project::ClipboardPayload> ();
  }
};

TEST_F (ClipboardTest, SetPayloadStoresAndWritesSystemClipboard)
{
  auto clipboard = make_clipboard ();
  EXPECT_FALSE (clipboard->payload ().has_value ());
  EXPECT_FALSE (clipboard->hasArrangerObjects ());
  EXPECT_FALSE (clipboard->hasTracks ());
  EXPECT_FALSE (clipboard->hasPlugins ());

  int                     changes = 0;
  QMetaObject::Connection conn = QObject::connect (
    clipboard.get (), &Clipboard::payloadChanged, clipboard.get (),
    [&changes] () { ++changes; });

  clipboard->setPayload (
    make_payload (structure::project::ClipboardPayload::Type::ArrangerObjects));
  EXPECT_EQ (changes, 1);
  EXPECT_TRUE (clipboard->hasArrangerObjects ());
  EXPECT_FALSE (clipboard->hasTracks ());
  EXPECT_FALSE (clipboard->hasPlugins ());
  ASSERT_TRUE (clipboard->payload ().has_value ());
  EXPECT_EQ (
    clipboard->payload ()->type (),
    structure::project::ClipboardPayload::Type::ArrangerObjects);
  EXPECT_EQ (system_write_count_, 1);
  EXPECT_TRUE (system_clipboard_text_.startsWith (
    structure::project::ClipboardPayload::kTextPrefix.data ()));

  QObject::disconnect (conn);
}

TEST_F (ClipboardTest, SetPayloadRefusesTextBeyondDecodeCap)
{
  auto clipboard = make_clipboard ();
  clipboard->setPayload (
    make_payload (structure::project::ClipboardPayload::Type::ArrangerObjects));
  ASSERT_TRUE (clipboard->payload ().has_value ());
  EXPECT_EQ (system_write_count_, 1);

  // Incompressible filler with a fixed seed: the compressed and encoded
  // text stays past the decode cap, so the copy could never be adopted
  // by another instance
  std::mt19937                       rng{ 42 };
  std::uniform_int_distribution<int> pick{ 0, 63 };
  constexpr std::string_view         alphabet{
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"
  };
  std::string filler (20LL * 1024 * 1024, ' ');
  for (char &c : filler)
    c = alphabet[static_cast<std::size_t> (pick (rng))];

  nlohmann::json j =
    make_payload (structure::project::ClipboardPayload::Type::ArrangerObjects);
  j["metadata"]["filler"] = filler;

  EXPECT_THROW (
    clipboard->setPayload (j.get<structure::project::ClipboardPayload> ()),
    utils::ZrythmException);

  // The previous payload and the OS clipboard are untouched
  ASSERT_TRUE (clipboard->payload ().has_value ());
  EXPECT_EQ (system_write_count_, 1);
}

TEST_F (ClipboardTest, ReloadIgnoresOwnWrites)
{
  auto clipboard = make_clipboard ();
  clipboard->setPayload (
    make_payload (structure::project::ClipboardPayload::Type::Tracks));
  EXPECT_TRUE (clipboard->hasTracks ());

  // The dataChanged handler firing for our own write must not emit or
  // replace anything
  int                           changes = 0;
  const QMetaObject::Connection conn = QObject::connect (
    clipboard.get (), &Clipboard::payloadChanged, clipboard.get (),
    [&changes] () { ++changes; });
  clipboard->reloadFromSystemClipboard ();
  EXPECT_EQ (changes, 0);
  EXPECT_TRUE (clipboard->hasTracks ());
  QObject::disconnect (conn);
}

TEST_F (ClipboardTest, ReloadAdoptsValidExternalPayload)
{
  auto clipboard = make_clipboard ();
  clipboard->setPayload (
    make_payload (structure::project::ClipboardPayload::Type::Tracks));

  // Simulate another Zrythm instance writing an arranger-objects payload
  const auto external =
    make_payload (structure::project::ClipboardPayload::Type::Plugins);
  system_clipboard_text_ = external.encode_to_clipboard_text ();

  clipboard->reloadFromSystemClipboard ();
  EXPECT_TRUE (clipboard->hasPlugins ());
  EXPECT_FALSE (clipboard->hasTracks ());
  ASSERT_TRUE (clipboard->payload ().has_value ());
  EXPECT_EQ (
    clipboard->payload ()->type (),
    structure::project::ClipboardPayload::Type::Plugins);
}

TEST_F (ClipboardTest, ReloadKeepsPayloadOnUnrelatedText)
{
  auto clipboard = make_clipboard ();
  clipboard->setPayload (
    make_payload (structure::project::ClipboardPayload::Type::Tracks));
  EXPECT_TRUE (clipboard->hasTracks ());

  system_clipboard_text_ = QStringLiteral ("just some text");
  clipboard->reloadFromSystemClipboard ();
  EXPECT_TRUE (clipboard->hasTracks ());
}

TEST_F (ClipboardTest, PayloadStoredDuringTextFetchWinsOverFetchedText)
{
  // The text fetch can spin a nested event loop in which a new payload
  // is stored: the fetched text is stale then and must not overwrite it
  Clipboard * clipboard_ptr = nullptr;
  bool        stored_during_fetch = false;
  auto        clipboard = std::make_unique<Clipboard> (
    [this, &clipboard_ptr, &stored_during_fetch] () {
      if (clipboard_ptr != nullptr && !stored_during_fetch)
        {
          stored_during_fetch = true;
          clipboard_ptr->setPayload (
            make_payload (structure::project::ClipboardPayload::Type::Tracks));
        }
      return system_clipboard_text_;
    },
    [] (const QString &) { });
  clipboard_ptr = clipboard.get ();

  system_clipboard_text_ =
    make_payload (structure::project::ClipboardPayload::Type::Plugins)
      .encode_to_clipboard_text ();
  clipboard->reloadFromSystemClipboard ();

  ASSERT_TRUE (clipboard->payload ().has_value ());
  EXPECT_EQ (
    clipboard->payload ()->type (),
    structure::project::ClipboardPayload::Type::Tracks);
  EXPECT_FALSE (clipboard->hasPlugins ());
}

TEST_F (ClipboardTest, NestedReloadDuringTextFetchIsDropped)
{
  // A reload re-entered from inside the text fetch returns immediately;
  // the outer reload proceeds unaffected
  Clipboard * clipboard_ptr = nullptr;
  bool        reentered = false;
  auto        clipboard = std::make_unique<Clipboard> (
    [this, &clipboard_ptr, &reentered] () {
      if (clipboard_ptr != nullptr && !reentered)
        {
          reentered = true;
          clipboard_ptr->reloadFromSystemClipboard ();
        }
      return system_clipboard_text_;
    },
    [] (const QString &) { });
  clipboard_ptr = clipboard.get ();

  system_clipboard_text_ =
    make_payload (structure::project::ClipboardPayload::Type::Plugins)
      .encode_to_clipboard_text ();
  clipboard->reloadFromSystemClipboard ();

  ASSERT_TRUE (clipboard->payload ().has_value ());
  EXPECT_EQ (
    clipboard->payload ()->type (),
    structure::project::ClipboardPayload::Type::Plugins);
}

} // namespace zrythm::controllers
