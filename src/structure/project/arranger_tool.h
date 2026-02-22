// SPDX-FileCopyrightText: © 2018-2019, 2024-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "utils/icloneable.h"

#include <QObject>
#include <QtQmlIntegration/qqmlintegration.h>

namespace zrythm::structure::project
{

class ArrangerTool : public QObject
{
  Q_OBJECT
  QML_ELEMENT
  Q_PROPERTY (
    int toolValue READ toolValue WRITE setToolValue NOTIFY toolValueChanged)

public:
  enum ToolType : std::uint8_t
  {
    Select,
    Edit,
    Cut,
    Eraser,
    Ramp,
    Audition,
  };
  Q_ENUM (ToolType)

  ArrangerTool (QObject * parent = nullptr);
  Z_DISABLE_COPY_MOVE (ArrangerTool)
  ~ArrangerTool () override;

  [[nodiscard]] int toolValue () const;
  void              setToolValue (int tool);
  Q_SIGNAL void     toolValueChanged (int tool);

  friend void init_from (
    ArrangerTool          &obj,
    const ArrangerTool    &other,
    utils::ObjectCloneType clone_type);

private:
  friend void to_json (nlohmann::json &j, const ArrangerTool &tool);
  friend void from_json (const nlohmann::json &j, ArrangerTool &tool);

private:
  ToolType tool_{ ToolType::Select };
};

} // namespace zrythm::structure::project
