// SPDX-FileCopyrightText: © 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "utils/gtest_wrapper.h"
#include "utils/qt.h"

#include <QCoreApplication>

using namespace zrythm::utils;

class QtTestQObject : public QObject
{
  Q_OBJECT
public:
  explicit QtTestQObject (QObject * parent = nullptr) : QObject (parent) { }
};

TEST (QObjectUniquePtrTest, BasicOwnership)
{
  // Create object with no parent
  auto ptr = make_qobject_unique<QtTestQObject> ();
  EXPECT_NE (ptr.get (), nullptr);

  // Reset should delete the object
  ptr.reset ();
  EXPECT_EQ (ptr.get (), nullptr);
}

TEST (QObjectUniquePtrTest, UnderlyingObjectDeletion)
{
  // Create object with no parent
  auto ptr = make_qobject_unique<QtTestQObject> ();
  EXPECT_NE (ptr.get (), nullptr);

  // Deleting the underlying object should reset the unique ptr
  delete ptr.get ();
  EXPECT_EQ (ptr.get (), nullptr);
}

TEST (QObjectUniquePtrTest, DeletionOfUniquePtrChild)
{
  QObject parent;
  {
    // Create object with parent
    auto ptr = make_qobject_unique<QtTestQObject> (&parent);
    EXPECT_NE (ptr.get (), nullptr);

    // Parent still exists, pointer should be valid
    EXPECT_FALSE (ptr->parent () == nullptr);

    // Parent still exists - object should still exist
    EXPECT_EQ (parent.children ().count (), 1);
  } // ptr goes out of scope

  // Child should be deleted
  EXPECT_EQ (parent.children ().count (), 0);

  // Delete parent - should delete child
  parent.deleteLater ();
  QCoreApplication::processEvents ();
  EXPECT_EQ (parent.children ().count (), 0);
}

TEST (QObjectUniquePtrTest, DeletionOfParent)
{
  auto * parent = new QObject ();

  // Create object with parent
  auto ptr = make_qobject_unique<QtTestQObject> (parent);
  EXPECT_NE (ptr.get (), nullptr);

  // Parent still exists, pointer should be valid
  EXPECT_FALSE (ptr->parent () == nullptr);

  // Parent still exists - object should still exist
  EXPECT_EQ (parent->children ().count (), 1);

  // Delete parent - should delete child
  delete parent;
  EXPECT_EQ (ptr.get (), nullptr);
}

TEST (QObjectUniquePtrTest, MoveSemantics)
{
  auto   ptr1 = make_qobject_unique<QtTestQObject> ();
  auto * rawPtr = ptr1.get ();

  // Move construction
  QObjectUniquePtr<QtTestQObject> ptr2 (std::move (ptr1));
  EXPECT_EQ (ptr1.get (), nullptr);
  EXPECT_EQ (ptr2.get (), rawPtr);

  // Move assignment
  QObjectUniquePtr<QtTestQObject> ptr3;
  ptr3 = std::move (ptr2);
  EXPECT_EQ (ptr2.get (), nullptr);
  EXPECT_EQ (ptr3.get (), rawPtr);
}

TEST (QObjectUniquePtrTest, NullHandling)
{
  QObjectUniquePtr<QtTestQObject> ptr;
  EXPECT_EQ (ptr.get (), nullptr);
  EXPECT_FALSE (static_cast<bool> (ptr));

  ptr.reset (new QtTestQObject);
  EXPECT_NE (ptr.get (), nullptr);
  EXPECT_TRUE (static_cast<bool> (ptr));

  ptr.reset ();
  EXPECT_EQ (ptr.get (), nullptr);
  EXPECT_FALSE (static_cast<bool> (ptr));
}

TEST (QObjectUniquePtrTest, Accessors)
{
  auto ptr = make_qobject_unique<QtTestQObject> ();
  ptr->setObjectName ("Test");

  EXPECT_EQ ((*ptr).objectName (), "Test");
  EXPECT_EQ (ptr->objectName (), "Test");
  EXPECT_EQ (ptr.get ()->objectName (), "Test");
}

inline auto
format_as (const QtTestQObject &obj)
{
  return std::string ("TestObject");
}

TEST (QObjectUniquePtrTest, Formatting)
{
  // Test formatting with valid object
  auto ptr = make_qobject_unique<QtTestQObject> ();

  auto formatted = fmt::format ("{}", ptr);
  EXPECT_TRUE (formatted.find ("TestObject") != std::string::npos);

  // Test formatting with null object
  QObjectUniquePtr<QtTestQObject> null_ptr;
  auto                            null_formatted = fmt::format ("{}", null_ptr);
  EXPECT_EQ (null_formatted, "(null)");
}

#include "qt_test.moc"
