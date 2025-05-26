// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "plugins/plugin_descriptor_list.h"

namespace zrythm::plugins::discovery
{

QVariant
PluginDescriptorList::data (const QModelIndex &index, int role) const
{
  if (index.row () < 0 || index.row () >= known_plugin_list_->getNumTypes ())
    return {};
  if (role == Qt::DisplayRole)
    return at (index.row ())->name_.to_qstring ();
  if (role == DescriptorRole)
    {
      return QVariant::fromValue (at (index.row ()).release ());
    }
  return {};
}
}
