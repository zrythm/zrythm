#include "utils/gtest_wrapper.h"
#include "utils/uuid_identifiable_object.h"

using namespace zrythm::utils;

using namespace zrythm;

class BaseTestObject
    : public utils::UuidIdentifiableObject<BaseTestObject>,
      public serialization::ISerializable<BaseTestObject>
{
public:
  BaseTestObject () = default;
  explicit BaseTestObject (Uuid id)
      : UuidIdentifiableObject<BaseTestObject> (id)
  {
  }
  ~BaseTestObject () override = default;
  BaseTestObject (const BaseTestObject &) = default;
  BaseTestObject &operator= (const BaseTestObject &) = default;
  BaseTestObject (BaseTestObject &&) = default;
  BaseTestObject &operator= (BaseTestObject &&) = default;

  std::string get_document_type () const override { return "TestObject"; }
  void        define_fields (const ISerializableBase::Context &ctx)
  {
    UuidIdentifiableObject::define_base_fields (ctx);
  }
};

static_assert (UuidIdentifiable<BaseTestObject>);
static_assert (!UuidIdentifiableQObject<BaseTestObject>);

TEST (UuidIdentifiableObjectTest, Creation)
{
  BaseTestObject obj1;
  BaseTestObject obj2;

  // Each object should get a unique UUID
  EXPECT_NE (obj1.get_uuid (), obj2.get_uuid ());
  EXPECT_FALSE (obj1.get_uuid ().is_null ());
  EXPECT_FALSE (obj2.get_uuid ().is_null ());
}

TEST (UuidIdentifiableObjectTest, Serialization)
{
  BaseTestObject obj1;
  auto json = obj1.ISerializable<BaseTestObject>::serialize_to_json_string ();

  BaseTestObject obj2;
  obj2.ISerializable<BaseTestObject>::deserialize_from_json_string (
    json.c_str ());

  EXPECT_EQ (obj2.get_uuid (), obj1.get_uuid ());
}

TEST (UuidIdentifiableObjectTest, CopyAndMove)
{
  BaseTestObject obj1;
  auto       id = obj1.get_uuid ();

  // Test copy
  BaseTestObject obj2 (obj1);
  EXPECT_EQ (obj2.get_uuid (), id);

  // Test move
  BaseTestObject obj3 (std::move (obj2));
  EXPECT_EQ (obj3.get_uuid (), id);

  // Test copy assignment
  BaseTestObject obj4;
  obj4 = obj1;
  EXPECT_EQ (obj4.get_uuid (), id);

  // Test move assignment
  BaseTestObject obj5;
  obj5 = std::move (obj3);
  EXPECT_EQ (obj5.get_uuid (), id);
}

using TestUuid = BaseTestObject::Uuid;

TEST (UuidIdentifiableObjectTest, UuidTypeOperations)
{
  TestUuid null_uuid;
  EXPECT_TRUE (null_uuid.is_null ());

  TestUuid uuid1{ QUuid::createUuid () };
  TestUuid uuid2{ QUuid::createUuid () };
  TestUuid uuid1_copy{ uuid1 };

  // Comparison operators
  EXPECT_EQ (uuid1, uuid1_copy);
  EXPECT_NE (uuid1, uuid2);
}

class DerivedTestObject
    : public QObject,
      public BaseTestObject,
      public ICloneable<DerivedTestObject>
{
public:
  explicit DerivedTestObject (TestUuid id, std::string name)
      : BaseTestObject (id), name_ (std::move (name))
  {
  }

  [[nodiscard]] std::string name () const { return name_; }

  void
  init_after_cloning (const DerivedTestObject &other, ObjectCloneType clone_type)
    final
  {
    name_ = other.name_;
  }

private:
  std::string name_;
};

// FIXME!!!
// static_assert (UuidIdentifiableQObject<DerivedTestObject>);

using TestVariant = std::variant<DerivedTestObject *>;
using TestRegistry = utils::OwningObjectRegistry<TestVariant, BaseTestObject>;

class UuidIdentifiableObjectRegistryTest : public ::testing::Test
{
protected:
  void SetUp () override
  {
    obj1_ = new DerivedTestObject (TestUuid{ QUuid::createUuid () }, "Object1");
    obj2_ = new DerivedTestObject (TestUuid{ QUuid::createUuid () }, "Object2");
    obj3_ = new DerivedTestObject (TestUuid{ QUuid::createUuid () }, "Object3");

    registry_.register_object (obj1_);
    registry_.register_object (obj2_);
    registry_.register_object (obj3_);
  }

  TestRegistry        registry_;
  DerivedTestObject * obj1_{};
  DerivedTestObject * obj2_{};
  DerivedTestObject * obj3_{};
};

TEST_F (UuidIdentifiableObjectRegistryTest, BasicRegistration)
{
  EXPECT_EQ (registry_.size (), 3);
  EXPECT_TRUE (registry_.contains (obj1_->get_uuid ()));
  EXPECT_FALSE (registry_.contains (TestUuid{}));
}

TEST_F (UuidIdentifiableObjectRegistryTest, DuplicateRejection)
{
  auto dupObj =
    std::make_unique<DerivedTestObject> (obj1_->get_uuid (), "Duplicate");
  EXPECT_THROW (registry_.register_object (dupObj.get ()), std::runtime_error);
}

TEST_F (UuidIdentifiableObjectRegistryTest, ObjectLookup)
{
  auto found_var = registry_.find_by_id_or_throw (obj2_->get_uuid ());
  std::visit (
    [&] (auto &&found) { EXPECT_EQ (found->name (), "Object2"); }, found_var);
  EXPECT_THROW (registry_.find_by_id_or_throw (TestUuid{}), std::runtime_error);
}

TEST_F (UuidIdentifiableObjectRegistryTest, SpanIteration)
{
  std::vector<TestUuid> uuids{ obj3_->get_uuid (), obj1_->get_uuid () };
  auto span = utils::UuidIdentifiableObjectSpan<TestRegistry> (registry_, uuids);

  // Test iteration order and content
  std::vector<std::string> names;
  for (const auto &obj : span)
    {
      names.push_back (std::visit ([] (auto * o) { return o->name (); }, obj));
    }

  EXPECT_EQ (names.size (), 2);
  EXPECT_EQ (names[0], "Object3");
  EXPECT_EQ (names[1], "Object1");
}

TEST_F (UuidIdentifiableObjectRegistryTest, UuidListRetrieval)
{
  const auto uuids = registry_.get_uuids ();
  EXPECT_EQ (uuids.size (), 3);
  EXPECT_TRUE (std::ranges::find (uuids, obj2_->get_uuid ()) != uuids.end ());
}

TEST_F (UuidIdentifiableObjectRegistryTest, SpanAccessors)
{
  std::vector<TestUuid> uuids{ obj2_->get_uuid (), obj3_->get_uuid () };
  auto span = utils::UuidIdentifiableObjectSpan<TestRegistry> (registry_, uuids);

  EXPECT_FALSE (span.empty ());
  EXPECT_EQ (span.size (), 2);
  EXPECT_EQ (
    std::visit ([&] (auto &&obj) { return obj->name (); }, span[0]), "Object2");
  EXPECT_EQ (
    std::visit ([&] (auto &&obj) { return obj->name (); }, span.back ()),
    "Object3");
}

TEST_F (UuidIdentifiableObjectRegistryTest, ReferenceCountingLifecycle)
{
  auto ref = std::make_optional (registry_.create_object<DerivedTestObject> (
    TestUuid{ QUuid::createUuid () }, "ReferenceTest"));
  auto id = ref->id ();

  EXPECT_TRUE (registry_.contains (id));
  EXPECT_EQ (registry_.size (), 4);

  {
    auto ref2 = *ref; // Copy increases ref count
    EXPECT_TRUE (registry_.contains (id));
  } // ref2 destroyed - decrease ref count

  // Should still exist after partial release
  EXPECT_TRUE (registry_.contains (id));

  ref.reset (); // Release final reference
  EXPECT_FALSE (registry_.contains (id));
  EXPECT_EQ (registry_.size (), 3);
}

TEST_F (UuidIdentifiableObjectRegistryTest, ObjectParentManagement)
{
  auto * obj =
    new DerivedTestObject (TestUuid{ QUuid::createUuid () }, "Orphan");
  EXPECT_EQ (obj->parent (), nullptr);

  registry_.register_object (obj);
  EXPECT_EQ (obj->parent (), &registry_);

  // LeakSanitizer should complain if the object still exists when tests finishes
}

TEST_F (UuidIdentifiableObjectRegistryTest, SpanEdgeCases)
{
  // Empty span
  std::vector<TestUuid> empty;
  auto                  empty_span =
    utils::UuidIdentifiableObjectSpan<TestRegistry> (registry_, empty);
  EXPECT_TRUE (empty_span.empty ());

  // Invalid access
  std::vector<TestUuid> single{ obj1_->get_uuid () };
  auto                  span =
    utils::UuidIdentifiableObjectSpan<TestRegistry> (registry_, single);
  EXPECT_THROW (span.at (1), std::out_of_range);
}

TEST_F (UuidIdentifiableObjectRegistryTest, ObjectCreation)
{
  auto ref = registry_.create_object<DerivedTestObject> (
    TestUuid{ QUuid::createUuid () }, "FactoryMade");
  auto found = registry_.find_by_id (ref.id ());
  EXPECT_TRUE (found.has_value ());
}

TEST_F (UuidIdentifiableObjectRegistryTest, ObjectCloning)
{
  auto clone_ref = registry_.clone_object (
    *obj1_, TestUuid{ QUuid::createUuid () }, "ClonedObject");
  EXPECT_NE (clone_ref.id (), obj1_->get_uuid ());
  EXPECT_EQ (registry_.size (), 4);
}
