// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "dsp/parameter.h"
#include "plugins/plugin.h"

#include <QAbstractListModel>
#include <QPointer>
#include <QtQmlIntegration/qqmlintegration.h>

namespace zrythm::plugins
{

class PluginParameterListModel : public QAbstractListModel
{
  Q_OBJECT
  Q_PROPERTY (
    zrythm::plugins::Plugin * plugin READ plugin WRITE setPlugin NOTIFY
      pluginChanged)
  QML_ELEMENT

public:
  enum Roles
  {
    ParamRole = Qt::UserRole + 1,
    ParamTypeRole,
  };
  Q_ENUM (Roles)

  explicit PluginParameterListModel (QObject * parent = nullptr);

  QHash<int, QByteArray> roleNames () const override;
  int      rowCount (const QModelIndex &parent = QModelIndex ()) const override;
  QVariant data (const QModelIndex &index, int role) const override;

  Plugin *      plugin () const { return plugin_.get (); }
  void          setPlugin (Plugin * plugin);
  Q_SIGNAL void pluginChanged ();

private:
  void rebuild_cache ();

  QPointer<Plugin>                       plugin_;
  QMetaObject::Connection                destroyed_connection_;
  std::vector<dsp::ProcessorParameter *> cached_params_;
};

} // namespace zrythm::plugins
