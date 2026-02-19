// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "structure/scenes/scene.h"
#include "structure/tracks/track_collection.h"

#include <QSignalSpy>
#include <QTest>

#include "unit/dsp/graph_helpers.h"
#include <gtest/gtest.h>

namespace zrythm::structure::scenes
{

class SceneTest : public ::testing::Test
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

    // Create a scene
    scene_ = std::make_unique<Scene> (*obj_registry_, *track_collection_);
  }

  void TearDown () override
  {
    scene_.reset ();
    track_collection_.reset ();
    obj_registry_.reset ();
    tempo_map_wrapper_.reset ();
    tempo_map_.reset ();
    track_registry_.reset ();
  }

  std::unique_ptr<Scene>                               scene_;
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

TEST_F (SceneTest, InitialState)
{
  // Initial name should be empty
  EXPECT_TRUE (scene_->name ().isEmpty ());

  // Initial color should be invalid (default constructed)
  EXPECT_FALSE (scene_->color ().isValid ());

  // Should have a clip slot list
  EXPECT_NE (scene_->clipSlots (), nullptr);
}

TEST_F (SceneTest, NameProperty)
{
  // Test setting name
  QSignalSpy nameSpy (scene_.get (), &Scene::nameChanged);
  scene_->setName ("Test Scene");

  EXPECT_EQ (scene_->name (), "Test Scene");
  EXPECT_EQ (nameSpy.count (), 1);
  EXPECT_EQ (nameSpy.at (0).at (0).toString (), "Test Scene");

  // Test setting to same name should not emit signal
  scene_->setName ("Test Scene");
  EXPECT_EQ (nameSpy.count (), 1);

  // Test changing name
  scene_->setName ("New Scene Name");
  EXPECT_EQ (scene_->name (), "New Scene Name");
  EXPECT_EQ (nameSpy.count (), 2);
  EXPECT_EQ (nameSpy.at (1).at (0).toString (), "New Scene Name");
}

TEST_F (SceneTest, ColorProperty)
{
  // Test setting color
  QColor     testColor (255, 0, 0); // Red
  QSignalSpy colorSpy (scene_.get (), &Scene::colorChanged);
  scene_->setColor (testColor);

  EXPECT_EQ (scene_->color (), testColor);
  EXPECT_EQ (colorSpy.count (), 1);
  EXPECT_EQ (colorSpy.at (0).at (0).value<QColor> (), testColor);

  // Test setting to same color should not emit signal
  scene_->setColor (testColor);
  EXPECT_EQ (colorSpy.count (), 1);

  // Test changing color
  QColor newColor (0, 255, 0); // Green
  scene_->setColor (newColor);
  EXPECT_EQ (scene_->color (), newColor);
  EXPECT_EQ (colorSpy.count (), 2);
  EXPECT_EQ (colorSpy.at (1).at (0).value<QColor> (), newColor);
}

TEST_F (SceneTest, ClipSlotsAccess)
{
  // Should have access to clip slots
  auto clipSlots = scene_->clipSlots ();
  EXPECT_NE (clipSlots, nullptr);

  // Should be able to access the underlying clip slots
  auto &clipSlotsRef = scene_->clip_slots ();
  EXPECT_EQ (&clipSlotsRef, &clipSlots->clip_slots ());
}

class SceneListTest : public ::testing::Test
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

    // Create a scene list
    scene_list_ =
      std::make_unique<SceneList> (*obj_registry_, *track_collection_);
  }

  void TearDown () override
  {
    scene_list_.reset ();
    track_collection_.reset ();
    obj_registry_.reset ();
    tempo_map_wrapper_.reset ();
    tempo_map_.reset ();
    track_registry_.reset ();
  }

  std::unique_ptr<SceneList>                           scene_list_;
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

TEST_F (SceneListTest, InitialState)
{
  // Should start empty
  EXPECT_EQ (scene_list_->rowCount (), 0);
  EXPECT_EQ (scene_list_->scenes ().size (), 0);
}

TEST_F (SceneListTest, InsertScene)
{
  // Create a scene
  auto scene = utils::make_qobject_unique<Scene> (
    *obj_registry_, *track_collection_, nullptr);
  scene->setName ("Test Scene");

  // Insert the scene
  scene_list_->insert_scene (std::move (scene), 0);

  // Should have one scene
  EXPECT_EQ (scene_list_->rowCount (), 1);
  EXPECT_EQ (scene_list_->scenes ().size (), 1);

  // Verify the scene is accessible
  auto index = scene_list_->index (0, 0);
  ASSERT_TRUE (index.isValid ());

  auto scenePtr =
    scene_list_->data (index, SceneList::ScenePtrRole).value<Scene *> ();
  EXPECT_NE (scenePtr, nullptr);
  EXPECT_EQ (scenePtr->name (), "Test Scene");
}

TEST_F (SceneListTest, RemoveScene)
{
  // Create and insert scenes
  auto scene1 = utils::make_qobject_unique<Scene> (
    *obj_registry_, *track_collection_, nullptr);
  scene1->setName ("Scene 1");
  auto scene2 = utils::make_qobject_unique<Scene> (
    *obj_registry_, *track_collection_, nullptr);
  scene2->setName ("Scene 2");

  scene_list_->insert_scene (std::move (scene1), 0);
  scene_list_->insert_scene (std::move (scene2), 1);

  EXPECT_EQ (scene_list_->rowCount (), 2);

  // Remove the first scene
  scene_list_->removeScene (0);

  // Should have one scene left
  EXPECT_EQ (scene_list_->rowCount (), 1);

  // Verify the remaining scene is the second one
  auto index = scene_list_->index (0, 0);
  ASSERT_TRUE (index.isValid ());

  auto scenePtr =
    scene_list_->data (index, SceneList::ScenePtrRole).value<Scene *> ();
  EXPECT_EQ (scenePtr->name (), "Scene 2");
}

TEST_F (SceneListTest, MoveScene)
{
  // Create and insert scenes
  auto scene1 = utils::make_qobject_unique<Scene> (
    *obj_registry_, *track_collection_, nullptr);
  scene1->setName ("Scene 1");
  auto scene2 = utils::make_qobject_unique<Scene> (
    *obj_registry_, *track_collection_, nullptr);
  scene2->setName ("Scene 2");
  auto scene3 = utils::make_qobject_unique<Scene> (
    *obj_registry_, *track_collection_, nullptr);
  scene3->setName ("Scene 3");

  scene_list_->insert_scene (std::move (scene1), 0);
  scene_list_->insert_scene (std::move (scene2), 1);
  scene_list_->insert_scene (std::move (scene3), 2);

  EXPECT_EQ (scene_list_->rowCount (), 3);

  // Move scene 3 to position 0
  scene_list_->moveScene (2, 0);

  // Verify new order
  auto index0 = scene_list_->index (0, 0);
  auto index1 = scene_list_->index (1, 0);
  auto index2 = scene_list_->index (2, 0);

  auto scenePtr0 =
    scene_list_->data (index0, SceneList::ScenePtrRole).value<Scene *> ();
  auto scenePtr1 =
    scene_list_->data (index1, SceneList::ScenePtrRole).value<Scene *> ();
  auto scenePtr2 =
    scene_list_->data (index2, SceneList::ScenePtrRole).value<Scene *> ();

  EXPECT_EQ (scenePtr0->name (), "Scene 3");
  EXPECT_EQ (scenePtr1->name (), "Scene 1");
  EXPECT_EQ (scenePtr2->name (), "Scene 2");
}

TEST_F (SceneListTest, RoleNames)
{
  auto roles = scene_list_->roleNames ();

  EXPECT_TRUE (roles.contains (SceneList::ScenePtrRole));
  EXPECT_EQ (roles[SceneList::ScenePtrRole], "scene");
}

TEST_F (SceneListTest, DataAccess)
{
  // Create and insert a scene
  auto scene = utils::make_qobject_unique<Scene> (
    *obj_registry_, *track_collection_, nullptr);
  scene->setName ("Test Scene");

  scene_list_->insert_scene (std::move (scene), 0);

  // Test accessing data at valid index
  auto index = scene_list_->index (0, 0);
  ASSERT_TRUE (index.isValid ());

  auto scenePtr =
    scene_list_->data (index, SceneList::ScenePtrRole).value<Scene *> ();
  EXPECT_NE (scenePtr, nullptr);
  EXPECT_EQ (scenePtr->name (), "Test Scene");

  // Test accessing data at invalid index
  auto invalidIndex = scene_list_->index (10, 0);
  EXPECT_FALSE (invalidIndex.isValid ());

  auto invalidData = scene_list_->data (invalidIndex, SceneList::ScenePtrRole);
  EXPECT_FALSE (invalidData.isValid ());
}

TEST_F (SceneListTest, OutOfBoundsOperations)
{
  // Test removing with invalid index
  scene_list_->removeScene (-1);
  scene_list_->removeScene (10);
  EXPECT_EQ (scene_list_->rowCount (), 0);

  // Test moving with invalid indices
  auto scene = utils::make_qobject_unique<Scene> (
    *obj_registry_, *track_collection_, nullptr);
  scene_list_->insert_scene (std::move (scene), 0);

  // These should not crash
  scene_list_->moveScene (-1, 0);
  scene_list_->moveScene (0, -1);
  scene_list_->moveScene (0, 10);
  scene_list_->moveScene (10, 0);

  // Scene should still be at position 0
  EXPECT_EQ (scene_list_->rowCount (), 1);
  auto index = scene_list_->index (0, 0);
  auto scenePtr =
    scene_list_->data (index, SceneList::ScenePtrRole).value<Scene *> ();
  EXPECT_NE (scenePtr, nullptr);
}

} // namespace zrythm::structure::scenes
