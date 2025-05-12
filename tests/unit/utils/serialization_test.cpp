// SPDX-FileCopyrightText: © 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <numbers>

#include "utils/gtest_wrapper.h"
#include "utils/serialization.h"

using namespace zrythm;
using namespace zrythm::utils::serialization;

// Test class with basic types
class SimpleObject
{
public:
  int         int_value = 42;
  std::string str_value = "test";
  bool        bool_value = true;
  float       float_value = 3.14f;

  std::string get_document_type () const { return "SimpleObject"; }

  NLOHMANN_DEFINE_TYPE_INTRUSIVE (
    SimpleObject,
    int_value,
    str_value,
    bool_value,
    float_value)
};

// Test class with containers
class ContainerObject
{
public:
  std::vector<int>         int_vec = { 1, 2, 3 };
  std::array<float, 3>     float_arr = { 1.1f, 2.2f, 3.3f };
  std::vector<std::string> str_vec = { "one", "two", "three" };

  std::string get_document_type () const { return "ContainerObject"; }

  NLOHMANN_DEFINE_TYPE_INTRUSIVE (ContainerObject, int_vec, float_arr, str_vec)
};

// Test class with nested objects
class NestedObject
{
public:
  SimpleObject              simple;
  std::vector<SimpleObject> simple_vec = { SimpleObject (), SimpleObject () };
  std::unique_ptr<SimpleObject> simple_ptr = std::make_unique<SimpleObject> ();

  std::string get_document_type () const { return "NestedObject"; }

  NLOHMANN_DEFINE_TYPE_INTRUSIVE (NestedObject, simple, simple_vec, simple_ptr)
};

TEST (SerializationTest, BasicTypes)
{
  SimpleObject obj;
  obj.int_value = 123;
  obj.str_value = "modified";
  obj.bool_value = false;
  obj.float_value = std::numbers::e_v<float>;

  nlohmann::json json = obj;

  auto deserialized = json.get<SimpleObject> ();

  EXPECT_EQ (deserialized.int_value, obj.int_value);
  EXPECT_EQ (deserialized.str_value, obj.str_value);
  EXPECT_EQ (deserialized.bool_value, obj.bool_value);
  EXPECT_FLOAT_EQ (deserialized.float_value, obj.float_value);
}

TEST (SerializationTest, Containers)
{
  ContainerObject obj;
  nlohmann::json  json = obj;

  auto deserialized = json.get<ContainerObject> ();

  EXPECT_EQ (deserialized.int_vec, obj.int_vec);
  EXPECT_EQ (deserialized.float_arr, obj.float_arr);
  EXPECT_EQ (deserialized.str_vec, obj.str_vec);
}

TEST (SerializationTest, NestedObjects)
{
  NestedObject obj;
  obj.simple.int_value = 999;
  obj.simple_vec[1].str_value = "modified vector";
  obj.simple_ptr->bool_value = false;

  nlohmann::json json = obj;

  auto deserialized = json.get<NestedObject> ();

  EXPECT_EQ (deserialized.simple.int_value, obj.simple.int_value);
  EXPECT_EQ (deserialized.simple_vec[1].str_value, obj.simple_vec[1].str_value);
  EXPECT_EQ (deserialized.simple_ptr->bool_value, obj.simple_ptr->bool_value);
}

TEST (SerializationTest, QtTypes)
{
  QUuid   uuid = QUuid::createUuid ();
  QString str = "Qt string";

  nlohmann::json json1 = uuid;
  nlohmann::json json2 = str;

  auto deserialized_uuid = json1.get<QUuid> ();
  auto deserialized_str = json2.get<QString> ();

  EXPECT_EQ (deserialized_uuid, uuid);
  EXPECT_EQ (deserialized_str, str);
}

struct SerializationTestVariantBuilder
{
  template <typename T> auto build () const { return std::make_unique<T> (); }
};

TEST (SerializationTest, VariantSerialization)
{
  using VariantT = std::variant<int, std::string, float>;
  VariantT       var = "variant test";
  nlohmann::json json = var;

  VariantT deserialized;
  variant_from_json_with_builder (
    json, deserialized, SerializationTestVariantBuilder{});

  EXPECT_EQ (deserialized.index (), 1); // string index
  EXPECT_EQ (std::get<std::string> (deserialized), std::get<std::string> (var));
}

TEST (SerializationTest, PointerSerialization)
{
  auto ptr = std::make_unique<SimpleObject> ();
  ptr->int_value = 999;

  nlohmann::json json = ptr;

  {
    auto deserialized = json.get<std::unique_ptr<SimpleObject>> ();
    EXPECT_EQ (deserialized->int_value, ptr->int_value);
  }
  {
    std::unique_ptr<SimpleObject> deserialized;
    json.get_to (deserialized);
    EXPECT_EQ (deserialized->int_value, ptr->int_value);
  }

  // check also copy-construction
  {
    auto deserialized = json.get<SimpleObject> ();
    EXPECT_EQ (deserialized.int_value, ptr->int_value);
  }
}

TEST (SerializationTest, NullPointerSerialization)
{
  std::unique_ptr<SimpleObject> ptr;
  nlohmann::json                json = ptr;

  EXPECT_ANY_THROW (
    auto deserialized = json.get<std::unique_ptr<SimpleObject>> (););
}

// StrongTypedef test
struct Meters
    : type_safe::strong_typedef<Meters, float>,
      type_safe::strong_typedef_op::equality_comparison<Meters>
{
  using strong_typedef::strong_typedef;
};

TEST (SerializationTest, StrongTypedef)
{
  Meters         m{ 5.5f };
  nlohmann::json json = m;
  auto           deserialized = json.get<Meters> ();

  EXPECT_FLOAT_EQ (type_safe::get (deserialized), 5.5f);
}