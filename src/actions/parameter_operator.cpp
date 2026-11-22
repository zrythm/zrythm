// SPDX-FileCopyrightText: © 2025-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "actions/parameter_operator.h"
#include "utils/logger.h"
#include "utils/utf8_string.h"

#include "operations/parameter_operations.h"

namespace zrythm::actions
{

namespace
{

QString
error_text (const operations::OperationReport &report)
{
  if (report.applied ())
    return QString ();
  z_warning ("refused parameter edit: {}", report.message);
  return utils::Utf8String::from_utf8_encoded_string (report.message)
    .to_qstring ();
}

} // namespace

ProcessorParameterOperator::ProcessorParameterOperator (
  operations::OperationDispatcher &operation_dispatcher,
  QObject *                        parent)
    : QObject (parent), operation_dispatcher_ (operation_dispatcher)
{
}

QString
ProcessorParameterOperator::setValue (
  dsp::ProcessorParameter * param,
  qreal                     value)
{
  return error_text (operation_dispatcher_.invoke (
    operations::parameter_set_value_operation (), param, value));
}

QString
ProcessorParameterOperator::setLiveValue (
  dsp::ProcessorParameter * param,
  qreal                     value)
{
  return error_text (operation_dispatcher_.invoke (
    operations::parameter_set_value_live_operation (), param, value));
}

} // namespace zrythm::actions
