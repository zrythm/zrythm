// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "plugins/plugin_group.h"
#include "utils/serialization.h"

namespace zrythm::plugins
{

struct PluginGroup::DeviceGroupImpl
{
  DeviceGroupImpl (PluginGroup &owner) : owner_ (owner) { }

  PluginGroup &owner_;
  std::vector<std::variant<
    utils::QObjectUniquePtr<PluginGroup>,
    plugins::PluginUuidReference>>
    devices_;
};

PluginGroup::PluginGroup (
  dsp::ProcessorBase::ProcessorBaseDependencies dependencies,
  plugins::PluginRegistry                      &plugin_registry,
  DeviceGroupType                               type,
  ProcessingTypeHint                            processing_type,
  QObject *                                     parent)
    : QAbstractListModel (parent), dependencies_ (dependencies),
      plugin_registry_ (plugin_registry), processing_type_ (processing_type),
      type_ (type), pimpl_ (std::make_unique<DeviceGroupImpl> (*this))
{
  fader_ = utils::make_qobject_unique<dsp::Fader> (
    dependencies,
    type == DeviceGroupType::MIDI ? dsp::PortType::Midi : dsp::PortType::Audio,
    false, false, [] () { return u8"Device Group"; },
    [] (bool fader_solo_status) { return false; }, this);
}

// ============================================================================
// QML Interface
// ============================================================================
void
PluginGroup::setName (const QString &name)
{
  if (name == name_)
    return;

  name_ = name;
  Q_EMIT nameChanged (name);
}

QHash<int, QByteArray>
PluginGroup::roleNames () const
{
  QHash<int, QByteArray> roles;
  roles[DeviceGroupPtrRole] = "deviceGroupOrPlugin";
  return roles;
}
int
PluginGroup::rowCount (const QModelIndex &parent) const
{
  return static_cast<int> (pimpl_->devices_.size ());
}
QVariant
PluginGroup::data (const QModelIndex &index, int role) const
{
  const auto index_int = index.row ();
  if (
    !index.isValid ()
    || index_int >= static_cast<int> (pimpl_->devices_.size ()))
    return {};

  if (role == DeviceGroupPtrRole)
    {
      const auto &var = pimpl_->devices_.at (index_int);
      if (var.index () == 0)
        {
          return QVariant::fromValue (std::get<0> (var).get ());
        }
      if (var.index () == 1)
        {
          return QVariant::fromValue (std::get<1> (var).get_object_base ());
        }
    }

  return {};
}
// ============================================================================

void
PluginGroup::insert_plugin (plugins::PluginUuidReference plugin_ref, int index)
{
  index = std::clamp (index, -1, rowCount ());

  if (index == -1)
    {
      beginInsertRows ({}, rowCount (), rowCount ());
      pimpl_->devices_.emplace_back (plugin_ref);
    }
  else
    {
      beginInsertRows ({}, index, index);
      pimpl_->devices_.insert (
        std::ranges::next (pimpl_->devices_.begin (), index), plugin_ref);
    }
  endInsertRows ();
}

plugins::PluginUuidReference
PluginGroup::remove_plugin (const plugins::Plugin::Uuid &plugin_id)
{
  auto it =
    std::ranges::find_if (pimpl_->devices_, [plugin_id] (const auto &element) {
      return element.index () == 1 && std::get<1> (element).id () == plugin_id;
    });
  if (it == pimpl_->devices_.end ())
    throw std::invalid_argument ("Plugin ID not found");

  const auto index = std::ranges::distance (pimpl_->devices_.begin (), it);
  auto       ret = std::get<1> (*it);
  beginRemoveRows ({}, static_cast<int> (index), static_cast<int> (index));
  pimpl_->devices_.erase (it);
  endRemoveRows ();

  return ret;
}

QVariant
PluginGroup::element_at_idx (size_t idx) const
{
  return data (index (static_cast<int> (idx)), DeviceGroupPtrRole);
}

struct BuilderForDeserialization
{
  BuilderForDeserialization (
    dsp::ProcessorBase::ProcessorBaseDependencies dependencies,
    PluginRegistry                               &plugin_registry)
      : dependencies_ (dependencies), plugin_registry_ (plugin_registry)
  {
  }

  template <typename T> std::unique_ptr<T> build () const
  {
    if constexpr (std::is_same_v<T, plugins::PluginUuidReference>)
      {
        auto pl_ref_ptr =
          std::make_unique<PluginUuidReference> (plugin_registry_);
        return pl_ref_ptr;
      }
    else
      {
        return std::make_unique<utils::QObjectUniquePtr<PluginGroup>> (
          utils::make_qobject_unique<PluginGroup> (
            dependencies_, plugin_registry_, PluginGroup::DeviceGroupType::Audio,
            PluginGroup::ProcessingTypeHint::Parallel));
      }
  }

  dsp::ProcessorBase::ProcessorBaseDependencies dependencies_;
  PluginRegistry                               &plugin_registry_;
};

void
PluginGroup::get_plugins (std::vector<plugins::PluginPtrVariant> &plugins) const
{
  for (const auto &var : pimpl_->devices_)
    {
      std::visit (
        [&] (const auto &p) {
          using Type = base_type<decltype (p)>;
          if constexpr (
            std::is_same_v<Type, utils::QObjectUniquePtr<PluginGroup>>)
            {
              p->get_plugins (plugins);
            }
          else
            {
              plugins.push_back (p.get_object ());
            }
        },
        var);
    }
}

void
to_json (nlohmann::json &j, const PluginGroup &l)
{
  j[PluginGroup::kDeviceGroupsKey] = l.pimpl_->devices_;
  j[PluginGroup::kFaderKey] = *l.fader_;
}
void
from_json (const nlohmann::json &j, PluginGroup &l)
{
  for (const auto &device_group_json : j.at (PluginGroup::kDeviceGroupsKey))
    {
      std::variant<
        utils::QObjectUniquePtr<PluginGroup>, plugins::PluginUuidReference>
        var;
      utils::serialization::variant_from_json_with_builder (
        device_group_json, var,
        BuilderForDeserialization{ l.dependencies_, l.plugin_registry_ });
      l.pimpl_->devices_.emplace_back (std::move (var));
    }
  j.at (PluginGroup::kFaderKey).get_to (*l.fader_);
}

PluginGroup::~PluginGroup () = default;
}
