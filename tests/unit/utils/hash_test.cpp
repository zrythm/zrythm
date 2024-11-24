#include "utils/gtest_wrapper.h"
#include "utils/hash.h"

TEST (HashTest, HashObject)
{
  struct TestStruct
  {
    int    a = 42;
    double b = 3.14;
    char   c[8] = "test";
  } test_struct;

  auto hash1 = zrythm::utils::hash::get_object_hash (test_struct);
  auto hash2 = zrythm::utils::hash::get_object_hash (test_struct);
  EXPECT_EQ (hash1, hash2);

  test_struct.a = 43;
  auto hash3 = zrythm::utils::hash::get_object_hash (test_struct);
  EXPECT_NE (hash1, hash3);
}

TEST (HashTest, HashFile)
{
  auto filepath = fs::path (TEST_WAV_FILE_PATH);
  auto hash1 = zrythm::utils::hash::get_file_hash (filepath);
  auto hash2 = zrythm::utils::hash::get_file_hash (filepath);
  EXPECT_NE (hash1, 0);
  EXPECT_EQ (hash1, hash2);

  auto hash_str = zrythm::utils::hash::to_string (hash1);
  EXPECT_EQ (hash_str.length (), 16);
  EXPECT_EQ (hash_str.find_first_not_of ("0123456789abcdef"), std::string::npos);
}

TEST (HashTest, HashString)
{
  std::string test1 = "Hello World";
  std::string test2 = "Hello World";
  std::string test3 = "Hello world";

  auto hash1 = zrythm::utils::hash::get_string_hash (test1);
  auto hash2 = zrythm::utils::hash::get_string_hash (test2);
  auto hash3 = zrythm::utils::hash::get_string_hash (test3);

  EXPECT_EQ (hash1, hash2);
  EXPECT_NE (hash1, hash3);
}
