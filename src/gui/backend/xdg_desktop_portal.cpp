// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <algorithm>
#include <ranges>

#include "gui/backend/xdg_desktop_portal.h"
#include "utils/utf8_string.h"

#include <QFile>
#include <QStandardPaths>

using namespace Qt::StringLiterals;

namespace zrythm::gui
{

namespace
{

/// Returns whether a portal backend descriptor lists the file chooser
/// interface. Descriptors use the desktop entry format: keys belong to
/// the group above them, whitespace around '=' is ignored, and values
/// are semicolon-separated lists.
bool
portal_descriptor_serves_file_chooser (const std::filesystem::path &portal_file)
{
  QFile file (utils::Utf8String::from_path (portal_file).to_qstring ());
  if (!file.open (QIODevice::ReadOnly))
    return false;

  bool in_portal_group = false;
  for (const QByteArray &raw_line : file.readAll ().split ('\n'))
    {
      const QString line = QString::fromUtf8 (raw_line).trimmed ();
      if (line.startsWith (u'['))
        {
          in_portal_group = line == u"[portal]"_s;
          continue;
        }
      if (!in_portal_group)
        continue;

      const auto equals_index = line.indexOf (u'=');
      if (
        equals_index < 0
        || line.left (equals_index).trimmed () != u"Interfaces"_s)
        continue;

      const auto interfaces = line.mid (equals_index + 1).split (u';');
      return std::ranges::any_of (interfaces, [] (const auto &interface) {
        return interface.trimmed ()
               == u"org.freedesktop.impl.portal.FileChooser"_s;
      });
    }
  return false;
}

} // namespace

bool
file_chooser_backend_installed (
  std::span<const std::filesystem::path> xdg_data_dirs)
{
  std::error_code ec;
  for (const auto &data_dir : xdg_data_dirs)
    {
      const auto portals_dir = data_dir / "xdg-desktop-portal" / "portals";
      for (
        const auto &entry :
        std::filesystem::directory_iterator{ portals_dir, ec })
        {
          if (
            entry.is_regular_file (ec) && entry.path ().extension () == ".portal"
            && portal_descriptor_serves_file_chooser (entry.path ()))
            return true;
        }
    }
  return false;
}

bool
portal_file_dialogs_available ()
{
  const auto data_dirs =
    QStandardPaths::standardLocations (QStandardPaths::GenericDataLocation)
    | std::views::transform ([] (const QString &dir) {
        return utils::Utf8String::from_qstring (dir).to_path ();
      })
    | std::ranges::to<std::vector> ();
  return file_chooser_backend_installed (data_dirs);
}

}
