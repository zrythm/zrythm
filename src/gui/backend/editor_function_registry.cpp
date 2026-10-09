// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <variant>

#include "dsp/curve.h"
#include "gui/backend/editor_function_registry.h"
#include "structure/arrangement/automation_function.h"
#include "structure/arrangement/midi_function.h"
#include "utils/enum_utils.h"
#include "utils/utf8_string.h"

namespace zrythm::gui
{

namespace
{

using Family = EditorFunctionDefinition::Family;

EditorFunctionParameter::ParameterType
parameter_type_from (structure::arrangement::ParamSpec::Kind kind)
{
  using ParamSpec = structure::arrangement::ParamSpec;
  using ParameterType = EditorFunctionParameter::ParameterType;
  switch (kind)
    {
    case ParamSpec::Kind::Integer:
      return ParameterType::Integer;
    case ParamSpec::Kind::Double:
      return ParameterType::Double;
    case ParamSpec::Kind::Boolean:
      return ParameterType::Boolean;
    case ParamSpec::Kind::Choice:
      return ParameterType::Choice;
    }
  return ParameterType::Integer;
}

QVariant
value_as_variant (const structure::arrangement::ParamValue &value)
{
  return std::visit ([] (const auto &v) { return QVariant (v); }, value);
}

/// Builds the definitions of a function family from its kind variant:
/// each alternative contributes its id, label, selection minimum and
/// parameters.
template <typename KindVariant>
QList<QObject *>
definitions_for_family (EditorFunctionRegistry &registry, Family family)
{
  QList<QObject *> definitions;
  [&]<std::size_t... Is> (std::index_sequence<Is...>) {
    ((
      [&] {
        using K = std::variant_alternative_t<Is, KindVariant>;
        QList<EditorFunctionParameter *> parameters;
        for (const auto &param : K::params ())
          {
            parameters.append (new EditorFunctionParameter (
              param.id.to_qstring (), param.label.to_qstring (),
              parameter_type_from (param.spec.kind), param.spec.minimum,
              param.spec.maximum, value_as_variant (param.spec.default_value),
              param.spec.choices));
          }
        auto * definition = new EditorFunctionDefinition (
          utils::Utf8String::from_utf8_encoded_string (K::kId).to_qstring (),
          K::label (true).to_qstring (), family,
          /* function_type */ 0, K::kMinimumSelectionCount, parameters,
          &registry);
        definitions.append (definition);
      }(),
      ...));
  }(std::make_index_sequence<std::variant_size_v<KindVariant>>{});
  return definitions;
}

} // namespace

EditorFunctionParameter::EditorFunctionParameter (
  QString       key,
  QString       label,
  ParameterType type,
  double        minimum,
  double        maximum,
  QVariant      default_value,
  QVariantList  choices,
  QObject *     parent)
    : QObject (parent), key_ (std::move (key)), label_ (std::move (label)),
      type_ (type), minimum_ (minimum), maximum_ (maximum),
      choices_ (std::move (choices)), default_value_ (std::move (default_value))
{
}

EditorFunctionDefinition::EditorFunctionDefinition (
  QString                          id,
  QString                          label,
  Family                           family,
  int                              function_type,
  int                              minimum_selection_count,
  QList<EditorFunctionParameter *> parameters,
  QObject *                        parent)
    : QObject (parent), id_ (std::move (id)), label_ (std::move (label)),
      family_ (family), function_type_ (function_type),
      minimum_selection_count_ (minimum_selection_count),
      parameters_ (parameters.cbegin (), parameters.cend ())
{
  for (auto * parameter : parameters_)
    {
      parameter->setParent (this);
    }
}

EditorFunctionRegistry::EditorFunctionRegistry (QObject * parent)
    : QObject (parent)
{
  const QVariantList curve_choices = CurveOptions_Algorithm_to_qml_list ();
  const int          superellipse =
    ENUM_VALUE_TO_INT (dsp::CurveOptions::Algorithm::SuperEllipse);

  const auto make_parameter =
    [] (
      const char * key, const QString &label,
      EditorFunctionParameter::ParameterType type, double minimum,
      double maximum, QVariant default_value, QVariantList choices = {}) {
      return new EditorFunctionParameter (
        QString::fromUtf8 (key), label, type, minimum, maximum,
        std::move (default_value), std::move (choices));
    };
  const auto make_definition =
    [this] (
      const char * id, const QString &label,
      EditorFunctionDefinition::Family family, int function_type,
      int minimum_selection_count, QList<EditorFunctionParameter *> parameters) {
      auto * definition = new EditorFunctionDefinition (
        QString::fromUtf8 (id), label, family, function_type,
        minimum_selection_count, std::move (parameters), this);
      for (auto * parameter : definition->parameters ())
        {
          parameter->setParent (definition);
        }
      return definition;
    };

  midi_functions_ = definitions_for_family<
    structure::arrangement::MidiFunction::Kind> (*this, Family::Midi);

  using AutomationType = structure::arrangement::AutomationFunction::Type;
  automation_functions_ = {
    make_definition (
      "flip-horizontal", tr ("Flip Horizontal"),
      EditorFunctionDefinition::Family::Automation,
      ENUM_VALUE_TO_INT (AutomationType::FlipHorizontal), 1, {}),
    make_definition (
      "flip-vertical", tr ("Flip Vertical"),
      EditorFunctionDefinition::Family::Automation,
      ENUM_VALUE_TO_INT (AutomationType::FlipVertical), 1, {}),
    make_definition (
      "flatten", tr ("Flatten"), EditorFunctionDefinition::Family::Automation,
      ENUM_VALUE_TO_INT (AutomationType::Flatten), 1, {}),
  };
}

} // namespace zrythm::gui
