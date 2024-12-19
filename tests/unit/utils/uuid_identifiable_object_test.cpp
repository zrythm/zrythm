#include "utils/gtest_wrapper.h"
#include "utils/uuid_identifiable_object.h"

using namespace zrythm::utils;

class TestObject
    : public UuidIdentifiableObject,
      public serialization::ISerializable<TestObject>
{
public:
  std::string get_document_type () const override { return "TestObject"; }
  DECLARE_DEFINE_FIELDS_METHOD ()
};

void
TestObject::define_fields (const ISerializableBase::Context &ctx)
{
  UuidIdentifiableObject::define_base_fields (ctx);
}

TEST (UuidIdentifiableObjectTest, Creation)
{
  TestObject obj1;
  TestObject obj2;

  // Each object should get a unique UUID
  EXPECT_NE (obj1.get_uuid (), obj2.get_uuid ());
  EXPECT_FALSE (obj1.get_uuid ().isNull ());
  EXPECT_FALSE (obj2.get_uuid ().isNull ());
}

TEST (UuidIdentifiableObjectTest, Serialization)
{
  TestObject obj1;
  auto       json = obj1.ISerializable<TestObject>::serialize_to_json_string ();

  TestObject obj2;
  obj2.ISerializable<TestObject>::deserialize_from_json_string (json.c_str ());

  EXPECT_EQ (obj2.get_uuid (), obj1.get_uuid ());
}

TEST (UuidIdentifiableObjectTest, CopyAndMove)
{
  TestObject obj1;
  auto       id = obj1.get_uuid ();

  // Test copy
  TestObject obj2 (obj1);
  EXPECT_EQ (obj2.get_uuid (), id);

  // Test move
  TestObject obj3 (std::move (obj2));
  EXPECT_EQ (obj3.get_uuid (), id);

  // Test copy assignment
  TestObject obj4;
  obj4 = obj1;
  EXPECT_EQ (obj4.get_uuid (), id);

  // Test move assignment
  TestObject obj5;
  obj5 = std::move (obj3);
  EXPECT_EQ (obj5.get_uuid (), id);
}
