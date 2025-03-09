#pragma once

#include "gui/dsp/arranger_object_all.h"
#include "utils/uuid_identifiable_object.h"

/**
 * @brief Track span that offers helper methods on a range of tracks.
 */
template <utils::UuidIdentifiableObjectPtrVariantRange Range>
class ArrangerObjectSpanImpl
    : public utils::
        UuidIdentifiableObjectCompatibleSpan<Range, ArrangerObjectRegistry>
{
public:
  using Base =
    utils::UuidIdentifiableObjectCompatibleSpan<Range, ArrangerObjectRegistry>;
  using VariantType = typename Base::value_type;
  using ArrangerObjectUuid = typename Base::UuidType;
  using Base::Base; // Inherit constructors

  static auto name_projection (const VariantType &port_var)
  {
    return std::visit (
      [] (const auto &port) { return port->get_name (); }, port_var);
  }
};

using ArrangerObjectSpan = ArrangerObjectSpanImpl<
  std::span<const ArrangerObjectWithoutVelocityPtrVariant>>;
using ArrangerObjectRegistrySpan = ArrangerObjectSpanImpl<
  utils::UuidIdentifiableObjectSpan<ArrangerObjectRegistry>>;
extern template class ArrangerObjectSpanImpl<
  std::span<const ArrangerObjectSpan::VariantType>>;
extern template class ArrangerObjectSpanImpl<
  utils::UuidIdentifiableObjectSpan<ArrangerObjectRegistry>>;

static_assert (std::ranges::random_access_range<ArrangerObjectSpan>);
static_assert (std::ranges::random_access_range<ArrangerObjectRegistrySpan>);

using ArrangerObjectSpanVariant =
  std::variant<ArrangerObjectSpan, ArrangerObjectRegistrySpan>;
