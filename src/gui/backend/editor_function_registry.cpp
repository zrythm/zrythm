// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <variant>

#include "dsp/curve.h"
#include "gui/backend/editor_function_registry.h"
#include "structure/arrangement/audio_function.h"
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
              param.spec.choices, param.spec.step));
          }
        auto * definition = new EditorFunctionDefinition (
          utils::Utf8String::from_utf8_encoded_string (K::kId).to_qstring (),
          K::label (true).to_qstring (), family, K::kMinimumSelectionCount,
          parameters, &registry);
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
  double        step,
  QObject *     parent)
    : QObject (parent), key_ (std::move (key)), label_ (std::move (label)),
      type_ (type), minimum_ (minimum), maximum_ (maximum),
      choices_ (std::move (choices)), default_value_ (std::move (default_value))
{
  // A declared step of 0 derives the increment: whole units for Integer
  // parameters, a hundredth of the range otherwise
  step_ =
    step > 0
      ? step
      : (type_ == ParameterType::Integer
           ? 1
           : (maximum_ > minimum_ ? (maximum_ - minimum_) / 100.0 : 0.1));
}

EditorFunctionDefinition::EditorFunctionDefinition (
  QString                          id,
  QString                          label,
  Family                           family,
  int                              minimum_selection_count,
  QList<EditorFunctionParameter *> parameters,
  QObject *                        parent)
    : QObject (parent), id_ (std::move (id)), label_ (std::move (label)),
      family_ (family), minimum_selection_count_ (minimum_selection_count),
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
  midi_functions_ = definitions_for_family<
    structure::arrangement::MidiFunction::Kind> (*this, Family::Midi);
  automation_functions_ =
    definitions_for_family<structure::arrangement::AutomationFunction::Kind> (
      *this, Family::Automation);
  audio_functions_ = definitions_for_family<
    structure::arrangement::AudioFunction::Entry> (*this, Family::Audio);
}

} // namespace zrythm::gui
