#include <QObject>

#include "utils/gtest_wrapper.h"
#include "utils/icloneable.h"

// Test class implementing ICloneable
class SimpleCloneable final : public ICloneable<SimpleCloneable>
{
public:
  int         value = 42;
  std::string text = "test";

  void
  init_after_cloning (const SimpleCloneable &other, ObjectCloneType clone_type)
    override
  {
    value = other.value;
    text = other.text;
  }
};

// Test class with containers
class ContainerCloneable final : public ICloneable<ContainerCloneable>
{
public:
  std::vector<std::unique_ptr<SimpleCloneable>>   vec;
  std::array<std::unique_ptr<SimpleCloneable>, 2> arr;

  void init_after_cloning (
    const ContainerCloneable &other,
    ObjectCloneType           clone_type) override
  {
    clone_unique_ptr_container (vec, other.vec);
    clone_unique_ptr_array (arr, other.arr);
  }
};

TEST (ICloneableTest, SimpleCloning)
{
  SimpleCloneable original;
  original.value = 100;
  original.text = "modified";

  auto unique_clone = original.clone_unique ();
  EXPECT_EQ (unique_clone->value, 100);
  EXPECT_EQ (unique_clone->text, "modified");

  auto shared_clone = original.clone_shared ();
  EXPECT_EQ (shared_clone->value, 100);
  EXPECT_EQ (shared_clone->text, "modified");

  auto raw_clone = original.clone_raw_ptr ();
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

  auto clone = original.clone_unique ();

  // Verify vector contents
  EXPECT_EQ (clone->vec.size (), 1);
  EXPECT_EQ (clone->vec[0]->value, 200);

  // Verify array contents
  EXPECT_EQ (clone->arr[0]->value, 300);
  EXPECT_EQ (clone->arr[1]->value, 400);
}

class TestQObject : public QObject, public ICloneable<TestQObject>
{
public:
  explicit TestQObject (QObject * parent = nullptr)
      : QObject (parent), value (42)
  {
  }
  int value;

  void init_after_cloning (const TestQObject &other, ObjectCloneType clone_type)
    override
  {
    value = other.value;
  }
};

TEST (ICloneableTest, QObjectCloning)
{
  TestQObject original;
  original.value = 100;
  QObject parent;

  auto qobject_clone = original.clone_qobject (&parent);
  EXPECT_EQ (qobject_clone->parent (), &parent);
  EXPECT_EQ (qobject_clone->value, 100);
}
