// SPDX-FileCopyrightText: © 2018-2019, 2024-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "utils/icloneable.h"

#include <QObject>
#include <QtQmlIntegration/qqmlintegration.h>

#include <nlohmann/json_fwd.hpp>

namespace zrythm::structure::project
{

class ArrangerTool : public QObject
{
  Q_OBJECT
  QML_ELEMENT
  Q_PROPERTY (
    int toolValue READ toolValue WRITE setToolValue NOTIFY toolValueChanged)
  Q_PROPERTY (
    ToolType effectiveToolValue READ effectiveToolValue NOTIFY
      effectiveToolValueChanged FINAL)

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
  Q_DISABLE_COPY_MOVE (ArrangerTool)
  ~ArrangerTool () override;

  [[nodiscard]] int toolValue () const;
  void              setToolValue (int tool);
  Q_SIGNAL void     toolValueChanged (int tool);

  /**
   * @brief The tool currently in effect.
   *
   * Equal to the override set via @ref setToolValueOverride if any,
   * otherwise @ref toolValue.
   */
  [[nodiscard]] ToolType effectiveToolValue () const;

  /**
   * @brief Sets a transient override of the selected tool (e.g. temporary
   * scissors while a modifier is held).
   *
   * @param tool A @ref ToolType value. Not serialized or cloned.
   */
  Q_INVOKABLE void setToolValueOverride (int tool);

  /// Clears the override set via @ref setToolValueOverride.
  Q_INVOKABLE void clearToolValueOverride ();

  Q_SIGNAL void effectiveToolValueChanged ();

  friend void init_from (
    ArrangerTool          &obj,
    const ArrangerTool    &other,
    utils::ObjectCloneType clone_type);

private:
  friend void to_json (nlohmann::json &j, const ArrangerTool &tool);
  friend void from_json (const nlohmann::json &j, ArrangerTool &tool);

private:
  ToolType tool_{ ToolType::Select };

  // Transient UI state (not serialized or cloned).
  std::optional<ToolType> tool_value_override_;
};

} // namespace zrythm::structure::project
