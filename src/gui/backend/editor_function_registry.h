// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <QList>
#include <QObject>
#include <QString>
#include <QVariantList>
#include <QtQmlIntegration/qqmlintegration.h>

namespace zrythm::gui
{

/**
 * @brief One typed parameter of an editor function.
 *
 * The generic function dialog renders a control per parameter: a slider
 * with an editable value readout for Integer and Double parameters, a
 * checkbox for Boolean parameters and a combo box for Choice parameters
 * (whose entries are given by @ref choices as {display, value} maps).
 */
class EditorFunctionParameter : public QObject
{
  Q_OBJECT
  Q_PROPERTY (QString key READ key CONSTANT FINAL)
  Q_PROPERTY (QString label READ label CONSTANT FINAL)
  Q_PROPERTY (ParameterType type READ type CONSTANT FINAL)
  Q_PROPERTY (double minimum READ minimum CONSTANT FINAL)
  Q_PROPERTY (double maximum READ maximum CONSTANT FINAL)
  Q_PROPERTY (double step READ step CONSTANT FINAL)
  Q_PROPERTY (QVariantList choices READ choices CONSTANT FINAL)
  Q_PROPERTY (QVariant defaultValue READ defaultParameterValue CONSTANT FINAL)
  QML_ELEMENT
  QML_UNCREATABLE ("")
public:
  enum class ParameterType
  {
    Integer,
    Double,
    Boolean,
    Choice,
  };
  Q_ENUM (ParameterType)

  EditorFunctionParameter (
    QString       key,
    QString       label,
    ParameterType type,
    double        minimum,
    double        maximum,
    QVariant      default_value,
    QVariantList  choices = {},
    double        step = 0,
    QObject *     parent = nullptr);

  const QString &key () const { return key_; }
  const QString &label () const { return label_; }
  ParameterType  type () const { return type_; }
  double         minimum () const { return minimum_; }
  double         maximum () const { return maximum_; }

  /// Value increment the dialog's slider snaps to; never smaller than 1
  /// for Integer parameters.
  double              step () const { return step_; }
  const QVariantList &choices () const { return choices_; }
  QVariant            defaultParameterValue () const { return default_value_; }

private:
  QString       key_;
  QString       label_;
  ParameterType type_;
  double        minimum_ = 0;
  double        maximum_ = 0;
  double        step_ = 0;
  QVariantList  choices_;
  QVariant      default_value_;
};

/**
 * @brief A transformative editor function the user can apply to the
 * selected material of an editor.
 *
 * @ref id identifies the function when applying it. @ref parameters
 * lists the parameters the generic function dialog offers; functions
 * without parameters are applied immediately when triggered.
 */
class EditorFunctionDefinition : public QObject
{
  Q_OBJECT
  Q_PROPERTY (QString id READ id CONSTANT FINAL)
  Q_PROPERTY (QString label READ label CONSTANT FINAL)
  Q_PROPERTY (Family family READ family CONSTANT FINAL)
  // Only the audio family uses this; the other families dispatch by id.
  Q_PROPERTY (int functionType READ functionType CONSTANT FINAL)
  Q_PROPERTY (
    int minimumSelectionCount READ minimumSelectionCount CONSTANT FINAL)
  Q_PROPERTY (QList<QObject *> parameters READ parameters CONSTANT FINAL)
  Q_PROPERTY (bool hasParameters READ hasParameters CONSTANT FINAL)
  QML_ELEMENT
  QML_UNCREATABLE ("")
public:
  enum class Family
  {
    Midi,
    Automation,
    Audio,
  };
  Q_ENUM (Family)

  EditorFunctionDefinition (
    QString                          id,
    QString                          label,
    Family                           family,
    int                              function_type,
    int                              minimum_selection_count,
    QList<EditorFunctionParameter *> parameters,
    QObject *                        parent = nullptr);

  const QString &id () const { return id_; }
  const QString &label () const { return label_; }
  Family         family () const { return family_; }
  int            functionType () const { return function_type_; }
  int minimumSelectionCount () const { return minimum_selection_count_; }
  QList<QObject *> parameters () const { return parameters_; }
  bool             hasParameters () const { return !parameters_.isEmpty (); }

private:
  QString          id_;
  QString          label_;
  Family           family_;
  int              function_type_;
  int              minimum_selection_count_;
  QList<QObject *> parameters_;
};

/**
 * @brief The editor functions available in each editor family.
 *
 * Built-in functions are registered at construction. Adding a function
 * means adding a definition here and its computation to the structure
 * layer; the function dialog and the editor menus pick it up
 * automatically.
 */
class EditorFunctionRegistry : public QObject
{
  Q_OBJECT
  Q_PROPERTY (QList<QObject *> midiFunctions READ midiFunctions CONSTANT FINAL)
  Q_PROPERTY (
    QList<QObject *> automationFunctions READ automationFunctions CONSTANT FINAL)
  Q_PROPERTY (QList<QObject *> audioFunctions READ audioFunctions CONSTANT FINAL)
  QML_ELEMENT
  QML_UNCREATABLE ("")
public:
  explicit EditorFunctionRegistry (QObject * parent = nullptr);

  QList<QObject *> midiFunctions () const { return midi_functions_; }
  QList<QObject *> automationFunctions () const
  {
    return automation_functions_;
  }
  QList<QObject *> audioFunctions () const { return audio_functions_; }

private:
  QList<QObject *> midi_functions_;
  QList<QObject *> automation_functions_;
  QList<QObject *> audio_functions_;
};

} // namespace zrythm::gui
