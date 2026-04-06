// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "plugins/plugin_parameter_list_model.h"

namespace zrythm::plugins
{

PluginParameterListModel::PluginParameterListModel (QObject * parent)
    : QAbstractListModel (parent)
{
}

QHash<int, QByteArray>
PluginParameterListModel::roleNames () const
{
  static const QHash<int, QByteArray> roles = {
    { ParamRole,     "parameter" },
    { ParamTypeRole, "paramType" },
  };
  return roles;
}

int
PluginParameterListModel::rowCount (const QModelIndex &parent) const
{
  if (parent.isValid ())
    return 0;
  return static_cast<int> (cached_params_.size ());
}

QVariant
PluginParameterListModel::data (const QModelIndex &index, int role) const
{
  if (
    !index.isValid ()
    || index.row () >= static_cast<int> (cached_params_.size ()))
    return {};

  auto * param = cached_params_.at (index.row ());

  switch (role)
    {
    case ParamRole:
      return QVariant::fromValue (param);
    case ParamTypeRole:
      return static_cast<int> (param->range ().type_);
    default:
      return {};
    }
}

void
PluginParameterListModel::setPlugin (Plugin * plugin)
{
  if (plugin_ == plugin)
    return;

  if (destroyed_connection_ != nullptr)
    {
      disconnect (destroyed_connection_);
    }

  beginResetModel ();
  plugin_ = plugin;
  if (plugin_ != nullptr)
    {
      destroyed_connection_ =
        connect (plugin_, &QObject::destroyed, this, [this] () {
          beginResetModel ();
          plugin_ = nullptr;
          cached_params_.clear ();
          destroyed_connection_ = {};
          endResetModel ();
          Q_EMIT pluginChanged ();
        });
    }
  rebuild_cache ();
  endResetModel ();

  Q_EMIT pluginChanged ();
}

void
PluginParameterListModel::rebuild_cache ()
{
  cached_params_.clear ();

  if (plugin_ == nullptr)
    return;

  const auto * bypass_param = plugin_->bypassParameter ();
  const auto * gain_param = plugin_->gainParameter ();

  const auto &param_refs = plugin_->get_parameters ();
  cached_params_.reserve (param_refs.size ());

  for (const auto &ref : param_refs)
    {
      auto * param = ref.get_object_as<dsp::ProcessorParameter> ();
      if (
        param != nullptr && !param->hidden () && param != bypass_param
        && param != gain_param)
        {
          cached_params_.push_back (param);
        }
    }
}

} // namespace zrythm::plugins
