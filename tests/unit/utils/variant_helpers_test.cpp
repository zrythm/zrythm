// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "utils/gtest_wrapper.h"
#include "utils/variant_helpers.h"

// using namespace zrythm::utils;

TEST (VariantUtilsTest, BasicTypeConversion)
{
  QVariant qInt (42);
  auto     vInt = qvariantToStdVariant<std::variant<int>> (qInt);
  EXPECT_TRUE (std::holds_alternative<int> (vInt));
  EXPECT_EQ (std::get<int> (vInt), 42);

  QVariant qStr ("test");
  auto     vStr = qvariantToStdVariant<std::variant<QString>> (qStr);
  EXPECT_TRUE (std::holds_alternative<QString> (vStr));
  EXPECT_EQ (std::get<QString> (vStr), "test");
}

TEST (VariantUtilsTest, MultipleTypeConversion)
{
  using TestVariant = std::variant<int, double, QString, bool>;

  QVariant qDouble (3.14);
  auto     vDouble = qvariantToStdVariant<TestVariant> (qDouble);
  EXPECT_TRUE (std::holds_alternative<double> (vDouble));
  EXPECT_NEAR (std::get<double> (vDouble), 3.14, 0.001);

  QVariant qBool (true);
  auto     vBool = qvariantToStdVariant<TestVariant> (qBool);
  EXPECT_TRUE (std::holds_alternative<bool> (vBool));
  EXPECT_TRUE (std::get<bool> (vBool));
}

TEST (VariantUtilsTest, PointerTypeConversion)
{
  struct TestStructA
  {
    int vala;
  };
  struct TestStructB
  {
    int valb;
  };
  TestStructB obj{ 42 };

  using PtrVariant = std::variant<TestStructA *, TestStructB *, int>;

  QVariant qPtr = QVariant::fromValue (&obj);
  auto     vPtr = qvariantToStdVariant<PtrVariant> (qPtr);
  EXPECT_TRUE (std::holds_alternative<TestStructB *> (vPtr));
  EXPECT_EQ (std::get<TestStructB *> (vPtr)->valb, 42);
}

TEST (VariantUtilsTest, InvalidConversion)
{
  using TestVariant = std::variant<int, bool>;

  QVariant qStr ("invalid");
  EXPECT_THROW (qvariantToStdVariant<TestVariant> (qStr), std::runtime_error);
}

TEST (VariantUtilsTest, EdgeCases)
{
  // Empty QVariant
  QVariant empty;
  EXPECT_THROW (
    qvariantToStdVariant<std::variant<int>> (empty), std::runtime_error);

  // Null pointer
  QVariant nullPtr = QVariant::fromValue (nullptr);
  using PtrVariant = std::variant<int *, QString *>;
  EXPECT_THROW (qvariantToStdVariant<PtrVariant> (nullPtr), std::runtime_error);
}