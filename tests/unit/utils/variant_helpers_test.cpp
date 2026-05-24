// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "utils/variant_helpers.h"

#include "./variant_helpers_test.h"
#include <gtest/gtest.h>

namespace zrythm::utils
{

struct Base
{
  virtual ~Base () = default;
};
struct DerivedA : Base
{
};
struct DerivedB : Base
{
};

using PtrVariant = std::variant<DerivedA *, DerivedB *>;

TEST (ConvertToVariantTest, NullPointerReturnsDefaultVariant)
{
  auto result =
    convert_to_variant<PtrVariant> (static_cast<const Base *> (nullptr));
  EXPECT_EQ (result.index (), 0);
  EXPECT_EQ (std::get<DerivedA *> (result), nullptr);
}

TEST (ConvertToVariantTest, MatchesFirstType)
{
  DerivedA     a;
  const Base * ptr = &a;
  auto         result = convert_to_variant<PtrVariant> (ptr);
  ASSERT_EQ (result.index (), 0);
  EXPECT_EQ (std::get<DerivedA *> (result), &a);
}

TEST (ConvertToVariantTest, MatchesSecondType)
{
  DerivedB     b;
  const Base * ptr = &b;
  auto         result = convert_to_variant<PtrVariant> (ptr);
  ASSERT_EQ (result.index (), 1);
  EXPECT_EQ (std::get<DerivedB *> (result), &b);
}

TEST (ConvertToVariantTest, ThrowsOnNoMatch)
{
  struct Other : Base
  {
  };
  Other        other;
  const Base * ptr = &other;
  EXPECT_THROW (convert_to_variant<PtrVariant> (ptr), std::runtime_error);
}

using QObjectPtrVariant =
  std::variant<ConvertToVariantQObjBaseA *, ConvertToVariantQObjBaseB *>;

TEST (ConvertToVariantQObjTest, NullPointerReturnsDefaultVariant)
{
  auto result = convert_to_variant_qobj<QObjectPtrVariant> (
    static_cast<const ConvertToVariantQObjBase *> (nullptr));
  EXPECT_EQ (result.index (), 0);
  EXPECT_EQ (std::get<ConvertToVariantQObjBaseA *> (result), nullptr);
}

TEST (ConvertToVariantQObjTest, MatchesFirstType)
{
  ConvertToVariantQObjBaseA        a;
  const ConvertToVariantQObjBase * ptr = &a;
  auto result = convert_to_variant_qobj<QObjectPtrVariant> (ptr);
  ASSERT_EQ (result.index (), 0);
  EXPECT_EQ (std::get<ConvertToVariantQObjBaseA *> (result), &a);
}

TEST (ConvertToVariantQObjTest, MatchesSecondType)
{
  ConvertToVariantQObjBaseB        b;
  const ConvertToVariantQObjBase * ptr = &b;
  auto result = convert_to_variant_qobj<QObjectPtrVariant> (ptr);
  ASSERT_EQ (result.index (), 1);
  EXPECT_EQ (std::get<ConvertToVariantQObjBaseB *> (result), &b);
}

TEST (ConvertToVariantQObjTest, ThrowsOnNoMatch)
{
  ConvertToVariantQObjOther        other;
  const ConvertToVariantQObjBase * ptr = &other;
  EXPECT_THROW (
    convert_to_variant_qobj<QObjectPtrVariant> (ptr), std::runtime_error);
}

} // namespace zrythm::utils
