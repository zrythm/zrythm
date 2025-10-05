// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "structure/scenes/clip_launcher.h"
#include "structure/tracks/track_collection.h"

#include <QSignalSpy>
#include <QTest>

#include "unit/dsp/graph_helpers.h"
#include <gtest/gtest.h>

namespace zrythm::structure::scenes
{

class ClipLauncherTest : public ::testing::Test
{
protected:
  void SetUp () override
  {
    // Create track registry
    track_registry_ = std::make_unique<tracks::TrackRegistry> ();

    // Create test dependencies
    tempo_map_ = std::make_unique<dsp::TempoMap> (units::sample_rate (44100.0));
    tempo_map_wrapper_ = std::make_unique<dsp::TempoMapWrapper> (*tempo_map_);

    // Create object registries
    obj_registry_ = std::make_unique<arrangement::ArrangerObjectRegistry> ();

    // Create track collection
    track_collection_ =
      std::make_unique<tracks::TrackCollection> (*track_registry_);

    // Create a clip launcher
    clip_launcher_ =
      std::make_unique<ClipLauncher> (*obj_registry_, *track_collection_);
  }

  void TearDown () override
  {
    clip_launcher_.reset ();
    track_collection_.reset ();
    obj_registry_.reset ();
    tempo_map_wrapper_.reset ();
    tempo_map_.reset ();
    track_registry_.reset ();
  }

  std::unique_ptr<ClipLauncher>                        clip_launcher_;
  std::unique_ptr<arrangement::ArrangerObjectRegistry> obj_registry_;
  std::unique_ptr<tracks::TrackCollection>             track_collection_;
  std::unique_ptr<tracks::TrackRegistry>               track_registry_;
  std::unique_ptr<dsp::TempoMap>                       tempo_map_;
  std::unique_ptr<dsp::TempoMapWrapper>                tempo_map_wrapper_;
  dsp::PortRegistry                                    port_registry_;
  dsp::ProcessorParameterRegistry param_registry_{ port_registry_ };
  dsp::FileAudioSourceRegistry    file_audio_source_registry_;
  plugins::PluginRegistry         plugin_registry_;
  dsp::graph_test::MockTransport  transport_;
};

TEST_F (ClipLauncherTest, InitialState)
{
  // Should have a scene list
  EXPECT_NE (clip_launcher_->scenes (), nullptr);

  // Should start with no scenes
  EXPECT_EQ (clip_launcher_->scenes ()->rowCount (), 0);
}

TEST_F (ClipLauncherTest, AddScene)
{
  // Initial scene count
  int initialCount = clip_launcher_->scenes ()->rowCount ();

  // Add a new scene
  auto newScene = clip_launcher_->addScene ();

  // Should have one more scene
  EXPECT_EQ (clip_launcher_->scenes ()->rowCount (), initialCount + 1);

  // The returned scene should be valid
  EXPECT_NE (newScene, nullptr);

  // The new scene should be the last one in the list
  auto lastIndex = clip_launcher_->scenes ()->index (initialCount, 0);
  ASSERT_TRUE (lastIndex.isValid ());

  auto scenePtr =
    clip_launcher_->scenes ()
      ->data (lastIndex, SceneList::ScenePtrRole)
      .value<Scene *> ();
  EXPECT_EQ (scenePtr, newScene);
}

TEST_F (ClipLauncherTest, AddMultipleScenes)
{
  // Initial scene count
  int initialCount = clip_launcher_->scenes ()->rowCount ();

  // Add multiple scenes
  auto scene1 = clip_launcher_->addScene ();
  auto scene2 = clip_launcher_->addScene ();
  auto scene3 = clip_launcher_->addScene ();

  // Should have three more scenes
  EXPECT_EQ (clip_launcher_->scenes ()->rowCount (), initialCount + 3);

  // All returned scenes should be valid and different
  EXPECT_NE (scene1, nullptr);
  EXPECT_NE (scene2, nullptr);
  EXPECT_NE (scene3, nullptr);
  EXPECT_NE (scene1, scene2);
  EXPECT_NE (scene2, scene3);
  EXPECT_NE (scene1, scene3);

  // Verify they're in the correct order
  auto index1 = clip_launcher_->scenes ()->index (initialCount, 0);
  auto index2 = clip_launcher_->scenes ()->index (initialCount + 1, 0);
  auto index3 = clip_launcher_->scenes ()->index (initialCount + 2, 0);

  auto scenePtr1 =
    clip_launcher_->scenes ()
      ->data (index1, SceneList::ScenePtrRole)
      .value<Scene *> ();
  auto scenePtr2 =
    clip_launcher_->scenes ()
      ->data (index2, SceneList::ScenePtrRole)
      .value<Scene *> ();
  auto scenePtr3 =
    clip_launcher_->scenes ()
      ->data (index3, SceneList::ScenePtrRole)
      .value<Scene *> ();

  EXPECT_EQ (scenePtr1, scene1);
  EXPECT_EQ (scenePtr2, scene2);
  EXPECT_EQ (scenePtr3, scene3);
}

TEST_F (ClipLauncherTest, SceneProperties)
{
  // Add a scene first
  auto scene = clip_launcher_->addScene ();
  ASSERT_NE (scene, nullptr);

  // Test setting scene properties
  QSignalSpy nameSpy (scene, &Scene::nameChanged);
  QSignalSpy colorSpy (scene, &Scene::colorChanged);

  scene->setName ("Test Scene");
  scene->setColor (QColor (255, 0, 0)); // Red

  // Verify signals were emitted
  EXPECT_EQ (nameSpy.count (), 1);
  EXPECT_EQ (colorSpy.count (), 1);

  // Verify properties were set
  EXPECT_EQ (scene->name (), "Test Scene");
  EXPECT_EQ (scene->color (), QColor (255, 0, 0));
}

TEST_F (ClipLauncherTest, SceneClipSlots)
{
  // Add a scene first
  auto scene = clip_launcher_->addScene ();
  ASSERT_NE (scene, nullptr);

  // Should have clip slots
  auto clipSlots = scene->clipSlots ();
  EXPECT_NE (clipSlots, nullptr);

  // Should be able to access the underlying clip slots
  auto &clipSlotsRef = scene->clip_slots ();
  EXPECT_EQ (&clipSlotsRef, &clipSlots->clip_slots ());
}

TEST_F (ClipLauncherTest, SceneListRoleNames)
{
  auto roles = clip_launcher_->scenes ()->roleNames ();

  EXPECT_TRUE (roles.contains (SceneList::ScenePtrRole));
  EXPECT_EQ (roles[SceneList::ScenePtrRole], "scene");
}

TEST_F (ClipLauncherTest, SceneListDataAccess)
{
  // Add some scenes first
  clip_launcher_->addScene ();
  clip_launcher_->addScene ();

  // Should have 2 scenes
  EXPECT_EQ (clip_launcher_->scenes ()->rowCount (), 2);

  // Test accessing data at valid indices
  for (int i = 0; i < clip_launcher_->scenes ()->rowCount (); ++i)
    {
      auto index = clip_launcher_->scenes ()->index (i, 0);
      ASSERT_TRUE (index.isValid ());

      auto scene =
        clip_launcher_->scenes ()
          ->data (index, SceneList::ScenePtrRole)
          .value<Scene *> ();
      EXPECT_NE (scene, nullptr);
    }

  // Test accessing data at invalid index
  auto invalidIndex = clip_launcher_->scenes ()->index (100, 0);
  EXPECT_FALSE (invalidIndex.isValid ());

  auto invalidData =
    clip_launcher_->scenes ()->data (invalidIndex, SceneList::ScenePtrRole);
  EXPECT_FALSE (invalidData.isValid ());
}

} // namespace zrythm::structure::scenes
