// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/cv_port.h"
#include "structure/project/modular_view_state.h"
#include "utils/object_registry.h"
#include "utils/registry_utils.h"

#include "helpers/scoped_qcoreapplication.h"

#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

namespace zrythm::structure::project
{
using namespace ::testing;

class ModularViewStateTest
    : public ::testing::Test,
      public test_helpers::ScopedQCoreApplication
{
protected:
  void SetUp () override
  {
    registry_ = std::make_unique<utils::ObjectRegistry> ();
  }

  void TearDown () override { registry_.reset (); }

  /**
   * @brief Node-position key of the form the Modular projection uses for a
   * registry object.
   */
  static QString node_id_for (const dsp::CVPort &port)
  {
    return u8"track:" + port.raw_uuid ().toString (QUuid::WithoutBraces);
  }

  std::unique_ptr<utils::ObjectRegistry> registry_;
};

TEST_F (ModularViewStateTest, ConstructionHasTrackScopeAndNoSelection)
{
  ModularViewState state (*registry_);
  EXPECT_EQ (state.scope (), ModularViewState::Scope::Track);
  EXPECT_FALSE (state.selected_track ().has_value ());
  EXPECT_FALSE (state.find_node_position (u8"track:x").has_value ());
  EXPECT_FALSE (state.find_viewport (u8"global").has_value ());
}

TEST_F (ModularViewStateTest, NodePositionStoreFindClear)
{
  ModularViewState state (*registry_);
  state.set_node_position (u8"processor:a", QPointF (10.0, 20.0));
  const auto position = state.find_node_position (u8"processor:a");
  ASSERT_TRUE (position.has_value ());
  EXPECT_EQ (*position, QPointF (10.0, 20.0));

  state.set_node_position (u8"processor:a", QPointF (30.0, 5.0));
  EXPECT_EQ (*state.find_node_position (u8"processor:a"), QPointF (30.0, 5.0));

  state.clear_node_position (u8"processor:a");
  EXPECT_FALSE (state.find_node_position (u8"processor:a").has_value ());
}

TEST_F (ModularViewStateTest, NodePositionsRoundtrip)
{
  ModularViewState state (*registry_);
  const auto       live = utils::create_object<dsp::CVPort> (
    *registry_, u8"Live port", dsp::PortFlow::Output);
  const auto *  live_port = live.get_object_as<dsp::CVPort> ();
  const QString live_id = node_id_for (*live_port);
  state.set_node_position (live_id, QPointF (120.5, 340.25));
  state.set_node_position (u8"unmanaged:key", QPointF (90.0, 1400.0));

  nlohmann::json j;
  to_json (j, state);
  ASSERT_TRUE (j.contains (ModularViewState::kNodePositionsKey));

  ModularViewState restored (*registry_);
  from_json (j, restored);
  const auto live_position = restored.find_node_position (live_id);
  ASSERT_TRUE (live_position.has_value ());
  EXPECT_EQ (*live_position, QPointF (120.5, 340.25));
  const auto unmanaged_position =
    restored.find_node_position (u8"unmanaged:key");
  ASSERT_TRUE (unmanaged_position.has_value ());
  EXPECT_EQ (*unmanaged_position, QPointF (90.0, 1400.0));
}

TEST_F (ModularViewStateTest, NodePositionsSkipMalformedEntries)
{
  ModularViewState state (*registry_);

  nlohmann::json j;
  j[ModularViewState::kNodePositionsKey] = nlohmann::json::array (
    {
      nlohmann::json::array (
        { "processor:valid", nlohmann::json{ { "x", 1.0 }, { "y", 2.0 } } }
         ),
      nlohmann::json::array ({ "processor:no-coordinates" }
         ),
      nlohmann::json::array ({ 42, nlohmann::json{ { "x", 1.0 }, { "y", 2.0 } } }
         ),
      nlohmann::json::array (
        { "processor:bad-x",
                        nlohmann::json{ { "x", "not-a-number" }, { "y", 2.0 } } }
         ),
  });
  from_json (j, state);

  EXPECT_FALSE (
    state.find_node_position (u8"processor:no-coordinates").has_value ());
  EXPECT_FALSE (state.find_node_position (u8"processor:bad-x").has_value ());
  const auto valid = state.find_node_position (u8"processor:valid");
  ASSERT_TRUE (valid.has_value ());
  EXPECT_EQ (*valid, QPointF (1.0, 2.0));
}

TEST_F (ModularViewStateTest, ViewportsRoundtripAndSkipMalformedEntries)
{
  ModularViewState state (*registry_);

  nlohmann::json j;
  j[ModularViewState::kViewportsKey] = nlohmann::json::array (
    {
      nlohmann::json::array (
        { "track:valid",
                        nlohmann::json{ { "panX", 1.0 }, { "panY", 2.0 }, { "zoom", 1.5 } }  }
         ),
      nlohmann::json::array (
        { "track:missing-zoom",
                        nlohmann::json{ { "panX", 1.0 }, { "panY", 2.0 } }                   }
         ),
      nlohmann::json::array (
        { "track:negative-zoom",
                        nlohmann::json{ { "panX", 1.0 }, { "panY", 2.0 }, { "zoom", -1.0 } } }
         ),
      nlohmann::json::array (
        { 42,
                        nlohmann::json{ { "panX", 1.0 }, { "panY", 2.0 }, { "zoom", 1.0 } }  }
         ),
      nlohmann::json::array (
        { "track:bad-pan",
                        nlohmann::json{ { "panX", "x" }, { "panY", 2.0 }, { "zoom", 1.0 } }  }
         ),
  });
  from_json (j, state);

  const auto viewport = state.find_viewport (u8"track:valid");
  ASSERT_TRUE (viewport.has_value ());
  EXPECT_DOUBLE_EQ (viewport->pan_x, 1.0);
  EXPECT_DOUBLE_EQ (viewport->pan_y, 2.0);
  EXPECT_DOUBLE_EQ (viewport->zoom, 1.5);
  EXPECT_FALSE (state.find_viewport (u8"track:missing-zoom").has_value ());
  EXPECT_FALSE (state.find_viewport (u8"track:negative-zoom").has_value ());
  EXPECT_FALSE (state.find_viewport (u8"track:bad-pan").has_value ());
}

TEST_F (ModularViewStateTest, ScopeRoundtrip)
{
  ModularViewState state (*registry_);
  state.set_scope (ModularViewState::Scope::Global);

  nlohmann::json j;
  to_json (j, state);
  ModularViewState restored (*registry_);
  from_json (j, restored);
  EXPECT_EQ (restored.scope (), ModularViewState::Scope::Global);

  restored.set_scope (ModularViewState::Scope::Track);
  from_json (j, restored);
  EXPECT_EQ (restored.scope (), ModularViewState::Scope::Global);
}

// Unknown and non-string scope values keep the current scope instead of
// failing the project load.
TEST_F (ModularViewStateTest, InvalidScopeValuesAreIgnored)
{
  ModularViewState state (*registry_);
  nlohmann::json   j;
  j[ModularViewState::kScopeKey] = "nonsense";
  from_json (j, state);
  EXPECT_EQ (state.scope (), ModularViewState::Scope::Track);

  j[ModularViewState::kScopeKey] = 5;
  from_json (j, state);
  EXPECT_EQ (state.scope (), ModularViewState::Scope::Track);

  state.set_scope (ModularViewState::Scope::Global);
  from_json (j, state);
  EXPECT_EQ (state.scope (), ModularViewState::Scope::Global);
}

TEST_F (ModularViewStateTest, SelectedTrackRoundtripAndRejectsNonString)
{
  const auto live = utils::create_object<dsp::CVPort> (
    *registry_, u8"Live port", dsp::PortFlow::Output);
  const ModularViewState::TrackUuid track_uuid (
    live.get_object_as<dsp::CVPort> ()->raw_uuid ());

  ModularViewState state (*registry_);
  state.set_selected_track (track_uuid);

  nlohmann::json j;
  to_json (j, state);
  ASSERT_TRUE (j.contains (ModularViewState::kSelectedTrackKey));
  ModularViewState restored (*registry_);
  from_json (j, restored);
  ASSERT_TRUE (restored.selected_track ().has_value ());
  EXPECT_EQ (*restored.selected_track (), track_uuid);

  // A non-string value keeps the previously stored track.
  j[ModularViewState::kSelectedTrackKey] = 5;
  from_json (j, restored);
  ASSERT_TRUE (restored.selected_track ().has_value ());
  EXPECT_EQ (*restored.selected_track (), track_uuid);
}

// Node positions, viewports and the selected track of removed tracks are
// dropped when saving; entries with other prefixes are kept.
TEST_F (ModularViewStateTest, SavePrunesEntriesOfRemovedObjects)
{
  const auto live = utils::create_object<dsp::CVPort> (
    *registry_, u8"Live port", dsp::PortFlow::Output);
  const QString live_id = node_id_for (*live.get_object_as<dsp::CVPort> ());

  ModularViewState state (*registry_);
  QString          dying_id;
  QString          dying_processor_id;
  {
    const auto dying = utils::create_object<dsp::CVPort> (
      *registry_, u8"Dying port", dsp::PortFlow::Output);
    dying_id = node_id_for (*dying.get_object_as<dsp::CVPort> ());
    dying_processor_id =
      u8"processor:"
      + dying.get_object_as<dsp::CVPort> ()->raw_uuid ().toString (
        QUuid::WithoutBraces);
    state.set_node_position (dying_id, QPointF (10.0, 20.0));
    state.set_node_position (dying_processor_id, QPointF (50.0, 60.0));
    state.set_viewport (dying_id, ModularViewState::Viewport{ 1.0, 2.0, 1.5 });
    state.set_selected_track (
      ModularViewState::TrackUuid (
        dying.get_object_as<dsp::CVPort> ()->raw_uuid ()));
  }
  state.set_node_position (live_id, QPointF (30.0, 40.0));
  state.set_viewport (u8"global", ModularViewState::Viewport{ 5.0, 6.0, 2.0 });

  nlohmann::json j;
  to_json (j, state);

  const auto collect_keys = [] (const nlohmann::json &entries) {
    QStringList keys;
    for (const auto &entry : entries)
      {
        QString key;
        entry[0].get_to (key);
        keys.append (key);
      }
    return keys;
  };
  ASSERT_TRUE (j.contains (ModularViewState::kNodePositionsKey));
  const QStringList saved_positions =
    collect_keys (j.at (ModularViewState::kNodePositionsKey));
  EXPECT_FALSE (saved_positions.contains (dying_id));
  EXPECT_TRUE (saved_positions.contains (dying_processor_id));
  EXPECT_TRUE (saved_positions.contains (live_id));

  ASSERT_TRUE (j.contains (ModularViewState::kViewportsKey));
  const QStringList saved_viewports =
    collect_keys (j.at (ModularViewState::kViewportsKey));
  EXPECT_FALSE (saved_viewports.contains (dying_id));
  EXPECT_TRUE (saved_viewports.contains (u8"global"));

  EXPECT_FALSE (j.contains (ModularViewState::kSelectedTrackKey));
}

} // namespace zrythm::structure::project
