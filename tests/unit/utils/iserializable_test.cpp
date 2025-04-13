// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "utils/gtest_wrapper.h"
#include "utils/iserializable.h"

using namespace zrythm::utils::serialization;

// Test class with basic types
class SimpleObject
    : public zrythm::utils::serialization::ISerializable<SimpleObject>
{
public:
  int         int_value = 42;
  std::string str_value = "test";
  bool        bool_value = true;
  float       float_value = 3.14f;

  std::string get_document_type () const override { return "SimpleObject"; }

  DECLARE_DEFINE_FIELDS_METHOD ();
};

void
SimpleObject::define_fields (const ISerializableBase::Context &ctx)
{
  serialize_fields (
    ctx, make_field ("int", int_value), make_field ("str", str_value),
    make_field ("bool", bool_value), make_field ("float", float_value));
}

// Test class with containers
class ContainerObject
    : public zrythm::utils::serialization::ISerializable<ContainerObject>
{
public:
  std::vector<int>         int_vec = { 1, 2, 3 };
  std::array<float, 3>     float_arr = { 1.1f, 2.2f, 3.3f };
  std::vector<std::string> str_vec = { "one", "two", "three" };

  std::string get_document_type () const override { return "ContainerObject"; }

  DECLARE_DEFINE_FIELDS_METHOD ();
};

void
ContainerObject::define_fields (const ISerializableBase::Context &ctx)
{
  serialize_fields (
    ctx, make_field ("int_vec", int_vec), make_field ("float_arr", float_arr),
    make_field ("str_vec", str_vec));
}

// Test class with nested objects
class NestedObject
    : public zrythm::utils::serialization::ISerializable<NestedObject>
{
public:
  SimpleObject              simple;
  std::vector<SimpleObject> simple_vec = { SimpleObject (), SimpleObject () };
  std::unique_ptr<SimpleObject> simple_ptr = std::make_unique<SimpleObject> ();

  DECLARE_DEFINE_FIELDS_METHOD ();
  std::string get_document_type () const override { return "NestedObject"; }
};

void
NestedObject::define_fields (const ISerializableBase::Context &ctx)
{
  serialize_fields (
    ctx, make_field ("simple", simple), make_field ("simple_vec", simple_vec),
    make_field ("simple_ptr", simple_ptr));
}

TEST (ISerializableTest, BasicTypes)
{
  SimpleObject obj;
  obj.int_value = 123;
  obj.str_value = "modified";
  obj.bool_value = false;
  obj.float_value = 2.718f;

  auto json = obj.serialize_to_json_string ();

  SimpleObject deserialized;
  deserialized.deserialize_from_json_string (json.c_str ());

  EXPECT_EQ (deserialized.int_value, obj.int_value);
  EXPECT_EQ (deserialized.str_value, obj.str_value);
  EXPECT_EQ (deserialized.bool_value, obj.bool_value);
  EXPECT_FLOAT_EQ (deserialized.float_value, obj.float_value);
}

TEST (ISerializableTest, Containers)
{
  ContainerObject obj;
  auto            json = obj.serialize_to_json_string ();

  ContainerObject deserialized;
  deserialized.deserialize_from_json_string (json.c_str ());

  EXPECT_EQ (deserialized.int_vec, obj.int_vec);
  EXPECT_EQ (deserialized.float_arr, obj.float_arr);
  EXPECT_EQ (deserialized.str_vec, obj.str_vec);
}

TEST (ISerializableTest, NestedObjects)
{
  NestedObject obj;
  obj.simple.int_value = 999;
  obj.simple_vec[1].str_value = "modified vector";
  obj.simple_ptr->bool_value = false;

  auto json = obj.serialize_to_json_string ();

  NestedObject deserialized;
  deserialized.deserialize_from_json_string (json.c_str ());

  EXPECT_EQ (deserialized.simple.int_value, obj.simple.int_value);
  EXPECT_EQ (deserialized.simple_vec[1].str_value, obj.simple_vec[1].str_value);
  EXPECT_EQ (deserialized.simple_ptr->bool_value, obj.simple_ptr->bool_value);
}
