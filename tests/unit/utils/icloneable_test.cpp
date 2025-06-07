// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "utils/gtest_wrapper.h"
#include "utils/icloneable.h"

#include <QObject>

// Test class implementing ICloneable
class SimpleCloneable final
{
public:
  int         value = 42;
  std::string text = "test";

  friend void init_from (
    SimpleCloneable       &obj,
    const SimpleCloneable &other,
    utils::ObjectCloneType clone_type)

  {
    obj.value = other.value;
    obj.text = other.text;
  }
};

// Test class with containers
class ContainerCloneable final
{
public:
  std::vector<std::unique_ptr<SimpleCloneable>>   vec;
  std::array<std::unique_ptr<SimpleCloneable>, 2> arr;

  friend void init_from (
    ContainerCloneable       &obj,
    const ContainerCloneable &other,
    utils::ObjectCloneType    clone_type)
  {
    utils::clone_unique_ptr_container (obj.vec, other.vec);
    utils::clone_unique_ptr_array (obj.arr, other.arr);
  }
};

TEST (ICloneableTest, SimpleCloning)
{
  SimpleCloneable original;
  original.value = 100;
  original.text = "modified";

  auto unique_clone = utils::clone_unique (original);
  EXPECT_EQ (unique_clone->value, 100);
  EXPECT_EQ (unique_clone->text, "modified");

  auto shared_clone = utils::clone_shared (original);
  EXPECT_EQ (shared_clone->value, 100);
  EXPECT_EQ (shared_clone->text, "modified");

  auto raw_clone = utils::clone_raw_ptr (original);
  EXPECT_EQ (raw_clone->value, 100);
  EXPECT_EQ (raw_clone->text, "modified");
  delete raw_clone;
}

TEST (ICloneableTest, ContainerCloning)
{
  ContainerCloneable original;

  // Setup vector
  original.vec.push_back (std::make_unique<SimpleCloneable> ());
  original.vec[0]->value = 200;

  // Setup array
  original.arr[0] = std::make_unique<SimpleCloneable> ();
  original.arr[0]->value = 300;
  original.arr[1] = std::make_unique<SimpleCloneable> ();
  original.arr[1]->value = 400;

  auto clone = utils::clone_unique (original);

  // Verify vector contents
  EXPECT_EQ (clone->vec.size (), 1);
  EXPECT_EQ (clone->vec[0]->value, 200);

  // Verify array contents
  EXPECT_EQ (clone->arr[0]->value, 300);
  EXPECT_EQ (clone->arr[1]->value, 400);
}

class ICloneableTestQObject : public QObject
{
public:
  explicit ICloneableTestQObject (QObject * parent = nullptr)
      : QObject (parent), value (42)
  {
  }
  int value;

  friend void init_from (
    ICloneableTestQObject       &obj,
    const ICloneableTestQObject &other,
    utils::ObjectCloneType       clone_type)
  {
    obj.value = other.value;
  }
};

TEST (ICloneableTest, QObjectCloning)
{
  ICloneableTestQObject original;
  original.value = 100;
  QObject parent;

  auto qobject_clone = utils::clone_qobject (original, &parent);
  EXPECT_EQ (qobject_clone->parent (), &parent);
  EXPECT_EQ (qobject_clone->value, 100);

  auto qobject_unique_ptr_clone =
    utils::clone_unique_qobject (original, &parent);
  EXPECT_EQ (qobject_unique_ptr_clone->parent (), &parent);
  EXPECT_EQ (qobject_unique_ptr_clone->value, 100);
}
