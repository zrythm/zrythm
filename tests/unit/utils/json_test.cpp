#include "utils/gtest_wrapper.h"
#include "utils/json.h"

TEST (JsonTest, GetString)
{
  // Test complete JSON serialization
  {
    yyjson_doc * doc = yyjson_read (
      R"({"name":"test","value":42})", strlen (R"({"name":"test","value":42})"),
      0);
    ASSERT_TRUE (doc != nullptr);
    yyjson_val * root = yyjson_doc_get_root (doc);
    auto         str = zrythm::utils::json::get_string (root);
    EXPECT_EQ (
      std::string (str.c_str ()),
      "{\n  \"name\": \"test\",\n  \"value\": 42\n}");
    yyjson_doc_free (doc);
  }

  // Test array serialization
  {
    yyjson_doc * doc = yyjson_read ("[1,2,3]", strlen ("[1,2,3]"), 0);
    ASSERT_TRUE (doc != nullptr);
    yyjson_val * root = yyjson_doc_get_root (doc);
    auto         str = zrythm::utils::json::get_string (root);
    EXPECT_EQ (std::string (str.c_str ()), "[\n  1,\n  2,\n  3\n]");
    yyjson_doc_free (doc);
  }
}
