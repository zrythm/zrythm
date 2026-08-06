// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <array>
#include <cstddef>
#include <memory>
#include <type_traits>
#include <utility>

#include "utils/inplace_function.h"

#include <gtest/gtest.h>

namespace zrythm::utils
{

class InplaceFunctionTest : public ::testing::Test
{
protected:
  using TestFunction = InplaceFunction<void ()>;
};

namespace
{

struct DestroyTracker
{
  bool * destroyed;

  DestroyTracker (bool * d) : destroyed (d) { }
  DestroyTracker (DestroyTracker &&other) noexcept
      : destroyed (std::exchange (other.destroyed, nullptr))
  {
  }
  DestroyTracker (const DestroyTracker &) = delete;
  DestroyTracker &operator= (const DestroyTracker &) = delete;
  DestroyTracker &operator= (DestroyTracker &&) = delete;
  ~DestroyTracker ()
  {
    if (destroyed != nullptr)
      *destroyed = true;
  }
  void operator() () const { }
};

struct OversizedCallable
{
  std::array<std::byte, 56> payload{};
  void                      operator() () const { }
};

struct OverAlignedCallable
{
  alignas (64) int value{};
  void operator() () const { }
};

struct ThrowingMoveCallable
{
  ThrowingMoveCallable () = default;
  ThrowingMoveCallable (ThrowingMoveCallable &&) noexcept (false) { }
  ThrowingMoveCallable (const ThrowingMoveCallable &) = delete;
  void operator() () const { }
};

struct ThrowingCopyCallable
{
  ThrowingCopyCallable () = default;
  ThrowingCopyCallable (ThrowingCopyCallable &&) noexcept = default;
  ThrowingCopyCallable (const ThrowingCopyCallable &) noexcept (false) { }
  void operator() () const { }
};

struct ExactlySizedCallable
{
  std::array<std::byte, 48> payload{};
  void                      operator() () const { }
};

} // anonymous namespace

using TestInplaceFunction = InplaceFunction<void ()>;

// Compile-time checks
static_assert (!std::is_copy_constructible_v<TestInplaceFunction>);
static_assert (!std::is_copy_assignable_v<TestInplaceFunction>);
static_assert (std::is_nothrow_move_constructible_v<TestInplaceFunction>);
static_assert (std::is_nothrow_move_assignable_v<TestInplaceFunction>);
static_assert (std::is_default_constructible_v<TestInplaceFunction>);
static_assert (std::is_constructible_v<TestInplaceFunction, std::nullptr_t>);
static_assert (
  !std::is_constructible_v<TestInplaceFunction, decltype ([] (int &) { })>);
static_assert (std::is_constructible_v<TestInplaceFunction, decltype ([] { })>);
static_assert (!std::is_constructible_v<TestInplaceFunction, OversizedCallable>);
static_assert (
  !std::is_constructible_v<TestInplaceFunction, OverAlignedCallable>);
static_assert (
  !std::is_constructible_v<TestInplaceFunction, ThrowingMoveCallable>);
// Lvalue arguments are stored via the copy constructor, so callables
// whose copy constructor is deleted or may throw are rejected for
// lvalues even when their move constructor is nothrow
static_assert (!std::is_constructible_v<TestInplaceFunction, DestroyTracker &>);
static_assert (std::is_constructible_v<TestInplaceFunction, DestroyTracker>);
static_assert (
  !std::is_constructible_v<TestInplaceFunction, ThrowingCopyCallable &>);
static_assert (
  std::is_constructible_v<TestInplaceFunction, ThrowingCopyCallable>);
static_assert (
  std::is_constructible_v<TestInplaceFunction, decltype ([] { }) &>);
static_assert (
  std::is_constructible_v<TestInplaceFunction, ExactlySizedCallable>);
static_assert (
  std::is_constructible_v<InplaceFunction<void (), 64>, OversizedCallable>);
static_assert (
  !std::is_constructible_v<InplaceFunction<void (), 47>, ExactlySizedCallable>);

TEST_F (InplaceFunctionTest, DefaultConstructedIsEmpty)
{
  TestFunction func;
  EXPECT_FALSE (func);
  EXPECT_FALSE (static_cast<bool> (func));
}

TEST_F (InplaceFunctionTest, NullptrConstructedIsEmpty)
{
  TestFunction func = nullptr;
  EXPECT_FALSE (func);
}

TEST_F (InplaceFunctionTest, InvokesStoredCallable)
{
  int          called = 0;
  TestFunction func ([&called] { ++called; });
  ASSERT_TRUE (func);
  func ();
  func ();
  EXPECT_EQ (called, 2);
}

TEST_F (InplaceFunctionTest, ReturnValueIsPropagated)
{
  InplaceFunction<int (int)> func ([] (int x) { return x * 3; });
  EXPECT_EQ (func (7), 21);
}

TEST_F (InplaceFunctionTest, ConstructsFromLvalueCallable)
{
  int          called = 0;
  auto         lvalue_lambda = [&called] { ++called; };
  TestFunction func (lvalue_lambda);
  ASSERT_TRUE (func);
  func ();
  EXPECT_EQ (called, 1);
}

TEST_F (InplaceFunctionTest, StoresMoveOnlyCallable)
{
  auto                    ptr = std::make_unique<int> (42);
  InplaceFunction<int ()> func ([p = std::move (ptr)] { return *p; });
  EXPECT_EQ (func (), 42);
}

TEST_F (InplaceFunctionTest, MoveConstructionTransfersCallable)
{
  auto         ptr = std::make_unique<int> (42);
  TestFunction source ([p = std::move (ptr)] { });
  TestFunction target (std::move (source));

  EXPECT_FALSE (source);
  ASSERT_TRUE (target);
  target ();
}

TEST_F (InplaceFunctionTest, MoveAssignmentTransfersCallable)
{
  auto         ptr = std::make_unique<int> (42);
  TestFunction source ([p = std::move (ptr)] { });
  TestFunction target ([] { });
  target = std::move (source);

  EXPECT_FALSE (source);
  ASSERT_TRUE (target);
}

TEST_F (InplaceFunctionTest, SelfMoveAssignmentKeepsCallable)
{
  int          called = 0;
  TestFunction func ([&called] { ++called; });
  auto        &ref = func;
  func = std::move (ref);
  ASSERT_TRUE (func);
  func ();
  EXPECT_EQ (called, 1);
}

TEST_F (InplaceFunctionTest, MutableLambdaIsInvocableThroughConstRef)
{
  const InplaceFunction<int ()> func ([x = 0] () mutable { return ++x; });
  EXPECT_EQ (func (), 1);
  EXPECT_EQ (func (), 2);
}

TEST_F (InplaceFunctionTest, ForwardsMoveOnlyArgumentToCallable)
{
  InplaceFunction<int (std::unique_ptr<int>)> func (
    [] (std::unique_ptr<int> ptr) { return *ptr; });
  EXPECT_EQ (func (std::make_unique<int> (42)), 42);
}

TEST_F (InplaceFunctionTest, LvalueReferenceArgumentBindsToOriginal)
{
  InplaceFunction<void (int &)> func ([] (int &x) { x *= 2; });
  int                           value = 21;
  func (value);
  EXPECT_EQ (value, 42);
}

TEST_F (InplaceFunctionTest, CallableIsDestroyedOnDestruction)
{
  bool destroyed = false;
  {
    TestFunction func (DestroyTracker{ &destroyed });
    EXPECT_FALSE (destroyed);
  }
  EXPECT_TRUE (destroyed);
}

TEST_F (InplaceFunctionTest, CallableIsDestroyedOnReset)
{
  bool         destroyed = false;
  TestFunction func (DestroyTracker{ &destroyed });
  func.reset ();
  EXPECT_TRUE (destroyed);
  EXPECT_FALSE (func);
}

TEST_F (InplaceFunctionTest, CallableIsDestroyedOnNullptrAssignment)
{
  bool         destroyed = false;
  TestFunction func (DestroyTracker{ &destroyed });
  func = nullptr;
  EXPECT_TRUE (destroyed);
  EXPECT_FALSE (func);
}

TEST_F (InplaceFunctionTest, MoveAssignmentDestroysPreviousCallable)
{
  bool         first_destroyed = false;
  bool         second_destroyed = false;
  TestFunction func (DestroyTracker{ &first_destroyed });
  func = TestFunction (DestroyTracker{ &second_destroyed });
  EXPECT_TRUE (first_destroyed);
  EXPECT_FALSE (second_destroyed);
}

TEST_F (InplaceFunctionTest, MovedFromCallableIsNotDoubleDestroyed)
{
  bool         destroyed = false;
  TestFunction source (DestroyTracker{ &destroyed });
  {
    TestFunction target (std::move (source));
    EXPECT_FALSE (destroyed);
  }
  EXPECT_TRUE (destroyed);
}

} // namespace zrythm::utils
