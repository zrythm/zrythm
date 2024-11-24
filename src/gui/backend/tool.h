// SPDX-FileCopyrightText: © 2018-2019, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef __GUI_BACKEND_TOOL_H__
#define __GUI_BACKEND_TOOL_H__

#include <QObject>
#include <QtQmlIntegration>

#include "utils/icloneable.h"
#include "utils/iserializable.h"

namespace zrythm::gui::backend
{

class Tool
    : public QObject,
      public ICloneable<Tool>,
      public zrythm::utils::serialization::ISerializable<Tool>
{
  Q_OBJECT
  QML_ELEMENT
  Q_PROPERTY (
    int toolValue READ getToolValue WRITE setToolValue NOTIFY toolValueChanged)

public:
  enum ToolType
  {
    Select,
    Edit,
    Cut,
    Eraser,
    Ramp,
    Audition,
  };
  Q_ENUM (ToolType)

  Tool (QObject * parent = nullptr);
  Q_DISABLE_COPY_MOVE (Tool)
  ~Tool () override = default;

  [[nodiscard]] int getToolValue () const;
  void              setToolValue (int tool);
  Q_SIGNAL void     toolValueChanged (int tool);

  void init_after_cloning (const Tool &other) override;

  DECLARE_DEFINE_FIELDS_METHOD ();

private:
  ToolType tool_{ ToolType::Select };
};

}; // namespace zrythm::gui::backend

#endif
