// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "plugins/plugin_group.h"

#include <QSignalSpy>
#include <QTest>

#include <gtest/gtest.h>

namespace zrythm::plugins
{

class DeviceGroupTest : public ::testing::Test
{
protected:
  void SetUp () override
  {
    port_registry_ = std::make_unique<dsp::PortRegistry> ();
    param_registry_ =
      std::make_unique<dsp::ProcessorParameterRegistry> (*port_registry_);
    plugin_registry_ = std::make_unique<PluginRegistry> ();

    device_group_ = std::make_unique<PluginGroup> (
      dsp::ProcessorBase::ProcessorBaseDependencies{
        .port_registry_ = *port_registry_, .param_registry_ = *param_registry_ },
      *plugin_registry_, PluginGroup::DeviceGroupType::Audio,
      PluginGroup::ProcessingTypeHint::Parallel);
  }

  void TearDown () override
  {
    device_group_.reset ();
    plugin_registry_.reset ();
    param_registry_.reset ();
    port_registry_.reset ();
  }

  Plugin * get_plugin_at_idx (const auto idx) const
  {
    return device_group_->element_at_idx (idx).template value<Plugin *> ();
  }

  std::unique_ptr<dsp::PortRegistry>               port_registry_;
  std::unique_ptr<dsp::ProcessorParameterRegistry> param_registry_;
  std::unique_ptr<PluginRegistry>                  plugin_registry_;
  std::unique_ptr<PluginGroup>                     device_group_;
  units::sample_rate_t sample_rate_{ units::sample_rate (48000) };
  nframes_t            max_block_length_{ 1024 };
};

TEST_F (DeviceGroupTest, ConstructionAndBasicProperties)
{
  EXPECT_EQ (device_group_->rowCount (), 0);
}

TEST_F (DeviceGroupTest, AppendPlugin)
{
  // Create a InternalPluginBase
  auto plugin_ref = plugin_registry_->create_object<InternalPluginBase> (
    dsp::ProcessorBase::ProcessorBaseDependencies{
      .port_registry_ = *port_registry_, .param_registry_ = *param_registry_ },
    [] () { return fs::path{ "/tmp/test_state" }; });

  // Create plugin configuration
  auto descriptor = std::make_unique<PluginDescriptor> ();
  descriptor->name_ = u8"Test InternalPluginBase 1";
  descriptor->protocol_ = Protocol::ProtocolType::Internal;
  PluginConfiguration config;
  config.descr_ = std::move (descriptor);

  // Get the plugin and set configuration
  auto * plugin = std::get<InternalPluginBase *> (plugin_ref.get_object ());
  plugin->set_configuration (config);

  // Test signal emission
  QSignalSpy spy (device_group_.get (), &QAbstractItemModel::rowsInserted);

  // Append the plugin
  device_group_->append_plugin (plugin_ref);

  EXPECT_EQ (device_group_->rowCount (), 1);
  EXPECT_EQ (spy.count (), 1);

  // Verify the plugin is correctly stored
  EXPECT_EQ (get_plugin_at_idx (0)->get_uuid (), plugin->get_uuid ());
}

TEST_F (DeviceGroupTest, InsertPluginAtSpecificIndex)
{
  // Create three test plugins
  std::vector<PluginUuidReference> plugins;
  for (int i = 0; i < 3; ++i)
    {
      auto plugin_ref = plugin_registry_->create_object<InternalPluginBase> (
        dsp::ProcessorBase::ProcessorBaseDependencies{
          .port_registry_ = *port_registry_,
          .param_registry_ = *param_registry_ },
        [] () { return fs::path{ "/tmp/test_state" }; });

      auto descriptor = std::make_unique<PluginDescriptor> ();
      descriptor->name_ = utils::Utf8String::from_utf8_encoded_string (
        fmt::format ("Test InternalPluginBase {}", i + 1));
      descriptor->protocol_ = Protocol::ProtocolType::Internal;
      PluginConfiguration config;
      config.descr_ = std::move (descriptor);

      auto * plugin = std::get<InternalPluginBase *> (plugin_ref.get_object ());
      plugin->set_configuration (config);

      plugins.push_back (plugin_ref);
    }

  // Append first two plugins
  device_group_->append_plugin (plugins[0]);
  device_group_->append_plugin (plugins[1]);

  // Insert third plugin at index 1
  QSignalSpy spy (device_group_.get (), &QAbstractItemModel::rowsInserted);
  device_group_->insert_plugin (plugins[2], 1);

  EXPECT_EQ (device_group_->rowCount (), 3);
  EXPECT_EQ (spy.count (), 1);

  // Verify order
  EXPECT_EQ (
    get_plugin_at_idx (0)->get_uuid (),
    std::get<InternalPluginBase *> (plugins[0].get_object ())->get_uuid ());
  EXPECT_EQ (
    get_plugin_at_idx (1)->get_uuid (),
    std::get<InternalPluginBase *> (plugins[2].get_object ())->get_uuid ());
  EXPECT_EQ (
    get_plugin_at_idx (2)->get_uuid (),
    std::get<InternalPluginBase *> (plugins[1].get_object ())->get_uuid ());
}

TEST_F (DeviceGroupTest, InsertPluginAtEnd)
{
  // Create two test plugins
  auto plugin_ref1 = plugin_registry_->create_object<InternalPluginBase> (
    dsp::ProcessorBase::ProcessorBaseDependencies{
      .port_registry_ = *port_registry_, .param_registry_ = *param_registry_ },
    [] () { return fs::path{ "/tmp/test_state" }; });
  auto plugin_ref2 = plugin_registry_->create_object<InternalPluginBase> (
    dsp::ProcessorBase::ProcessorBaseDependencies{
      .port_registry_ = *port_registry_, .param_registry_ = *param_registry_ },
    [] () { return fs::path{ "/tmp/test_state" }; });

  auto descriptor1 = std::make_unique<PluginDescriptor> ();
  descriptor1->name_ = u8"Test InternalPluginBase 1";
  descriptor1->protocol_ = Protocol::ProtocolType::Internal;
  PluginConfiguration config1;
  config1.descr_ = std::move (descriptor1);
  std::get<InternalPluginBase *> (plugin_ref1.get_object ())
    ->set_configuration (config1);

  auto descriptor2 = std::make_unique<PluginDescriptor> ();
  descriptor2->name_ = u8"Test InternalPluginBase 2";
  descriptor2->protocol_ = Protocol::ProtocolType::Internal;
  PluginConfiguration config2;
  config2.descr_ = std::move (descriptor2);
  std::get<InternalPluginBase *> (plugin_ref2.get_object ())
    ->set_configuration (config2);

  // Insert at end (index -1)
  device_group_->insert_plugin (plugin_ref1, -1);
  device_group_->insert_plugin (plugin_ref2, -1);

  EXPECT_EQ (device_group_->rowCount (), 2);
  EXPECT_EQ (
    get_plugin_at_idx (0)->get_uuid (),
    std::get<InternalPluginBase *> (plugin_ref1.get_object ())->get_uuid ());
  EXPECT_EQ (
    get_plugin_at_idx (1)->get_uuid (),
    std::get<InternalPluginBase *> (plugin_ref2.get_object ())->get_uuid ());
}

TEST_F (DeviceGroupTest, RemovePlugin)
{
  // Create a InternalPluginBase
  auto plugin_ref = plugin_registry_->create_object<InternalPluginBase> (
    dsp::ProcessorBase::ProcessorBaseDependencies{
      .port_registry_ = *port_registry_, .param_registry_ = *param_registry_ },
    [] () { return fs::path{ "/tmp/test_state" }; });

  auto descriptor = std::make_unique<PluginDescriptor> ();
  descriptor->name_ = u8"Test InternalPluginBase";
  descriptor->protocol_ = Protocol::ProtocolType::Internal;
  PluginConfiguration config;
  config.descr_ = std::move (descriptor);
  auto * plugin = std::get<InternalPluginBase *> (plugin_ref.get_object ());
  plugin->set_configuration (config);

  // Add plugin
  device_group_->append_plugin (plugin_ref);
  EXPECT_EQ (device_group_->rowCount (), 1);

  // Test signal emission
  QSignalSpy spy (device_group_.get (), &QAbstractItemModel::rowsRemoved);

  // Remove plugin
  auto removed_ref = device_group_->remove_plugin (plugin->get_uuid ());

  EXPECT_EQ (device_group_->rowCount (), 0);
  EXPECT_EQ (spy.count (), 1);
  EXPECT_EQ (removed_ref.id (), plugin->get_uuid ());
}

TEST_F (DeviceGroupTest, RemoveNonExistentPlugin)
{
  // Create a InternalPluginBase
  auto plugin_ref = plugin_registry_->create_object<InternalPluginBase> (
    dsp::ProcessorBase::ProcessorBaseDependencies{
      .port_registry_ = *port_registry_, .param_registry_ = *param_registry_ },
    [] () { return fs::path{ "/tmp/test_state" }; });

  auto descriptor = std::make_unique<PluginDescriptor> ();
  descriptor->name_ = u8"Test InternalPluginBase";
  descriptor->protocol_ = Protocol::ProtocolType::Internal;
  PluginConfiguration config;
  config.descr_ = std::move (descriptor);
  auto * plugin = std::get<InternalPluginBase *> (plugin_ref.get_object ());
  plugin->set_configuration (config);

  // Try to remove non-existent plugin
  EXPECT_THROW (
    device_group_->remove_plugin (plugin->get_uuid ()), std::invalid_argument);
}

TEST_F (DeviceGroupTest, QmlModelInterface)
{
  // Create test plugins
  std::vector<PluginUuidReference> plugins;
  for (int i = 0; i < 3; ++i)
    {
      auto plugin_ref = plugin_registry_->create_object<InternalPluginBase> (
        dsp::ProcessorBase::ProcessorBaseDependencies{
          .port_registry_ = *port_registry_,
          .param_registry_ = *param_registry_ },
        [] () { return fs::path{ "/tmp/test_state" }; });

      auto descriptor = std::make_unique<PluginDescriptor> ();
      descriptor->name_ = utils::Utf8String::from_utf8_encoded_string (
        fmt::format ("Test InternalPluginBase {}", i + 1));
      descriptor->protocol_ = Protocol::ProtocolType::Internal;
      PluginConfiguration config;
      config.descr_ = std::move (descriptor);

      auto * plugin = std::get<InternalPluginBase *> (plugin_ref.get_object ());
      plugin->set_configuration (config);

      plugins.push_back (plugin_ref);
      device_group_->append_plugin (plugin_ref);
    }

  // Test roleNames
  auto roles = device_group_->roleNames ();
  EXPECT_TRUE (roles.contains (PluginGroup::DeviceGroupPtrRole));
  EXPECT_EQ (roles[PluginGroup::DeviceGroupPtrRole], "deviceGroupOrPlugin");

  // Test rowCount
  EXPECT_EQ (device_group_->rowCount (), 3);

  // Test data retrieval
  for (int i = 0; i < 3; ++i)
    {
      QModelIndex index = device_group_->index (i, 0);
      EXPECT_TRUE (index.isValid ());

      // Test PluginVariantRole
      QVariant data =
        device_group_->data (index, PluginGroup::DeviceGroupPtrRole);
      EXPECT_TRUE (data.isValid ());
      EXPECT_TRUE (data.canConvert<InternalPluginBase *> ());

      auto * plugin = data.value<InternalPluginBase *> ();
      EXPECT_EQ (
        plugin->get_name (),
        utils::Utf8String::from_utf8_encoded_string (
          fmt::format ("Test InternalPluginBase {}", i + 1)));
    }

  // Test invalid index
  QModelIndex invalid_index = device_group_->index (-1, 0);
  EXPECT_FALSE (invalid_index.isValid ());
  EXPECT_FALSE (device_group_->data (invalid_index).isValid ());
}

TEST_F (DeviceGroupTest, JsonSerializationRoundtrip)
{
  // Create test plugins
  std::vector<PluginUuidReference> original_plugins;
  for (int i = 0; i < 3; ++i)
    {
      auto plugin_ref = plugin_registry_->create_object<InternalPluginBase> (
        dsp::ProcessorBase::ProcessorBaseDependencies{
          .port_registry_ = *port_registry_,
          .param_registry_ = *param_registry_ },
        [] () { return fs::path{ "/tmp/test_state" }; });

      auto descriptor = std::make_unique<PluginDescriptor> ();
      descriptor->name_ = utils::Utf8String::from_utf8_encoded_string (
        fmt::format ("Test InternalPluginBase {}", i + 1));
      descriptor->protocol_ = Protocol::ProtocolType::Internal;
      PluginConfiguration config;
      config.descr_ = std::move (descriptor);

      auto * plugin = std::get<InternalPluginBase *> (plugin_ref.get_object ());
      plugin->set_configuration (config);

      original_plugins.push_back (plugin_ref);
      device_group_->append_plugin (plugin_ref);
    }

  // Serialize to JSON
  nlohmann::json j = *device_group_;

  // Create new DeviceGroup and deserialize
  PluginGroup deserialized_list (
    dsp::ProcessorBase::ProcessorBaseDependencies{
      .port_registry_ = *port_registry_, .param_registry_ = *param_registry_ },
    *plugin_registry_, PluginGroup::DeviceGroupType::Audio,
    PluginGroup::ProcessingTypeHint::Parallel);
  from_json (j, deserialized_list);

  // Verify deserialized state
  EXPECT_EQ (deserialized_list.rowCount (), 3);

  // Verify plugin order and content
  for (size_t i = 0; i < 3; ++i)
    {
      const auto &original_ref = original_plugins[i];
      const auto &deserialized_id =
        deserialized_list.element_at_idx (i).value<Plugin *> ()->get_uuid ();

      EXPECT_EQ (original_ref.id (), deserialized_id);
    }
}

TEST_F (DeviceGroupTest, EmptyJsonSerialization)
{
  // Serialize empty list
  nlohmann::json j = *device_group_;

  // Create new DeviceGroup and deserialize
  PluginGroup deserialized_list (
    dsp::ProcessorBase::ProcessorBaseDependencies{
      .port_registry_ = *port_registry_, .param_registry_ = *param_registry_ },
    *plugin_registry_, PluginGroup::DeviceGroupType::Audio,
    PluginGroup::ProcessingTypeHint::Parallel);
  from_json (j, deserialized_list);

  EXPECT_EQ (deserialized_list.rowCount (), 0);
}

TEST_F (DeviceGroupTest, ModelSignals)
{
  // Create test plugins
  auto plugin_ref1 = plugin_registry_->create_object<InternalPluginBase> (
    dsp::ProcessorBase::ProcessorBaseDependencies{
      .port_registry_ = *port_registry_, .param_registry_ = *param_registry_ },
    [] () { return fs::path{ "/tmp/test_state" }; });
  auto plugin_ref2 = plugin_registry_->create_object<InternalPluginBase> (
    dsp::ProcessorBase::ProcessorBaseDependencies{
      .port_registry_ = *port_registry_, .param_registry_ = *param_registry_ },
    [] () { return fs::path{ "/tmp/test_state" }; });

  auto descriptor1 = std::make_unique<PluginDescriptor> ();
  descriptor1->name_ = u8"Test InternalPluginBase 1";
  descriptor1->protocol_ = Protocol::ProtocolType::Internal;
  PluginConfiguration config1;
  config1.descr_ = std::move (descriptor1);
  std::get<InternalPluginBase *> (plugin_ref1.get_object ())
    ->set_configuration (config1);

  auto descriptor2 = std::make_unique<PluginDescriptor> ();
  descriptor2->name_ = u8"Test InternalPluginBase 2";
  descriptor2->protocol_ = Protocol::ProtocolType::Internal;
  PluginConfiguration config2;
  config2.descr_ = std::move (descriptor2);
  std::get<InternalPluginBase *> (plugin_ref2.get_object ())
    ->set_configuration (config2);

  // Test rowsInserted signal
  QSignalSpy insert_spy (
    device_group_.get (), &QAbstractItemModel::rowsInserted);
  device_group_->append_plugin (plugin_ref1);
  EXPECT_EQ (insert_spy.count (), 1);

  // Test rowsRemoved signal
  QSignalSpy remove_spy (device_group_.get (), &QAbstractItemModel::rowsRemoved);
  device_group_->remove_plugin (
    std::get<InternalPluginBase *> (plugin_ref1.get_object ())->get_uuid ());
  EXPECT_EQ (remove_spy.count (), 1);
}

TEST_F (DeviceGroupTest, MultipleOperations)
{
  // Create multiple test plugins
  std::vector<PluginUuidReference> plugins;
  for (int i = 0; i < 5; ++i)
    {
      auto plugin_ref = plugin_registry_->create_object<InternalPluginBase> (
        dsp::ProcessorBase::ProcessorBaseDependencies{
          .port_registry_ = *port_registry_,
          .param_registry_ = *param_registry_ },
        [] () { return fs::path{ "/tmp/test_state" }; });

      auto descriptor = std::make_unique<PluginDescriptor> ();
      descriptor->name_ = utils::Utf8String::from_utf8_encoded_string (
        fmt::format ("Test InternalPluginBase {}", i + 1));
      descriptor->protocol_ = Protocol::ProtocolType::Internal;
      PluginConfiguration config;
      config.descr_ = std::move (descriptor);

      auto * plugin = std::get<InternalPluginBase *> (plugin_ref.get_object ());
      plugin->set_configuration (config);

      plugins.push_back (plugin_ref);
    }

  // Perform multiple operations
  device_group_->append_plugin (plugins[0]);
  device_group_->append_plugin (plugins[1]);
  device_group_->insert_plugin (plugins[2], 1);
  device_group_->append_plugin (plugins[3]);
  device_group_->remove_plugin (
    std::get<InternalPluginBase *> (plugins[1].get_object ())->get_uuid ());
  device_group_->insert_plugin (plugins[4], 1);

  // Verify final state
  EXPECT_EQ (device_group_->rowCount (), 4);

  // Verify order: [0, 4, 2, 3]
  EXPECT_EQ (
    get_plugin_at_idx (0)->get_uuid (),
    std::get<InternalPluginBase *> (plugins[0].get_object ())->get_uuid ());
  EXPECT_EQ (
    get_plugin_at_idx (1)->get_uuid (),
    std::get<InternalPluginBase *> (plugins[4].get_object ())->get_uuid ());
  EXPECT_EQ (
    get_plugin_at_idx (2)->get_uuid (),
    std::get<InternalPluginBase *> (plugins[2].get_object ())->get_uuid ());
  EXPECT_EQ (
    get_plugin_at_idx (3)->get_uuid (),
    std::get<InternalPluginBase *> (plugins[3].get_object ())->get_uuid ());
}

} // namespace zrythm::plugins
