// SPDX-FileCopyrightText: © 2020-2022, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "zrythm-test-config.h"

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN

#include "utils/string.h"

#include "doctest_wrapper.h"

TEST_SUITE_BEGIN ("utils/string");

TEST_CASE ("get int after last space")
{
  const char * strs[] = {
    "helloЧитатьハロー・ワールド 1",
    "testハロー・ワールドest 22",
    "testハロー22",
    "",
    "testハロー 34 56",
  };

  auto ret = string_get_int_after_last_space (strs[0]);
  REQUIRE_EQ (ret.first, 1);
  ret = string_get_int_after_last_space (strs[1]);
  REQUIRE_EQ (ret.first, 22);
  ret = string_get_int_after_last_space (strs[2]);
  REQUIRE_EQ (ret.first, -1);
  ret = string_get_int_after_last_space (strs[4]);
  REQUIRE_EQ (ret.first, 56);
}

TEST_CASE ("contains substr")
{
  const char * strs[] = {
    "helloЧитатьハロー・ワールド 1",
    "testハロー・ワールドABc 22",
    "testハロー22",
    "",
    "testハロー 34 56",
  };

  REQUIRE (string_contains_substr (strs[0], "Читать"));
  REQUIRE (string_contains_substr_case_insensitive (strs[1], "abc"));
  REQUIRE_FALSE (string_contains_substr (strs[0], "Чатитать"));
  REQUIRE_FALSE (string_contains_substr_case_insensitive (strs[1], "abd"));
}

TEST_CASE ("is equal")
{
  REQUIRE (string_is_equal ("", ""));
  REQUIRE (string_is_equal (nullptr, nullptr));
  REQUIRE (string_is_equal ("abc", "abc"));
  REQUIRE_FALSE (string_is_equal ("abc", "aabc"));
  REQUIRE_FALSE (string_is_equal ("", "aabc"));
  REQUIRE_FALSE (string_is_equal (nullptr, ""));

#if 0
  /* pwgen -c -n -s 48 50 */
  const char * test_str[] = {
    "I4CbyNnEBenmUM7KxIZglH7JQcUMEnyeCZILGbU2tk6MkJUR",
    "R59i3HuJJv29nZEw8Cs2NQq8zn22Fqe66I85Ie7YsVs5rlLu",
    "jDsUMssCSSFxpJlS04HZ1yAed9wUTUXdqRtJcKCmFTGhS0FE",
    "NMltMaCyimcD8yy6wfwbBigO4rVj8372XoOOii4IDuNCy55g",
    "CEv0fpJMSlCCg3iyVL5P52VMxarmnY2N0GMlvhT3ZakDGvpu",
    "Z25v7AZ3ExD8mDZezyYcbIWbiygMECD1b7sb0YPF6dDAtA5O",
    "AvsrfEXSJTvwLWnXJf1g4AypthC1f906Req1TolV3zrmDYqb",
    "iY2Dj0g2vFQXWfOyosIlSxokSgzMzIIY22WPiQAFIw2rPjKN",
    "I5p0eCadiZqfk9fOZ2eKjnGLaLOYqOaiKTb5ES3fiLaCcb1B",
    "AmoTQNeEjCfJFJENC0asQUy1I4HERpczQalPUCI4y9B8E0uZ",
    "HorOLuRZ4F8094VVERwPN1JAwkA2OeiYNpQSrSnGG6Lpfboe",
    "HpYOQdqdYrUV6Fd2ihUmAJ6AN14qfFkiClZjEBW9XlN4Jvuw",
    "mlwyYkJgG05b95rpSvFI7j54hRB92EamyKtdLi8bquwEeQdl",
    "JWgbZN3PzVmoLnHrzHxKycLWP3wDYazaaR3qa5kftETHTG1s",
    "Zx2PkiFvCd7yOY29F4W2pUtLrrgfYvA8oHrKINeWREPmM2pd",
    "l0rjRKrZATtN7xUEmUpfWH95Pd2WyWS4UeFsrNMG3jolRgkU",
    "2hYO198v4WaAHaYPwD1dcGRK4Am0rAr2znLl6Lz12n8Bu5Ml",
    "P7RyKv1tqOiveOZinoefybK48fKJ7IzZo2ST0VcYWKcdXuCt",
    "Df0UyVQLGxFaLmD6Y941xRAcSiBMoUC71FJz96NQQfOaJfWM",
    "7nZHHDZY1rAEczSCYpLKiWbDeeKoGrs4cN5p2ClK1Fiu8gaZ",
    "ahsW8eVkgw7XGmEQn75Ovn5RCyCms0Eq4yoJsa6Bms6WLIXR",
    "vQCSip2ptv27XIr7DRPs86J0Dtf3K09s2rRjy9NX19wCxhD1",
    "ADErMTk4wFqHAHjqHKYoy0TshGgO4LUMKHdicjRdxqj7705l",
    "p2ECxB0FA4gI5ZK1dR8ssIKw3OMZJMclyij7hLaNuYYS2dpU",
    "wRjcENLJddhjg1mqlrA44kfhWjxg8UKuronslU3KMwCF89d5",
    "AWwjwfXdjcxXsNZEx3CJcnMJFRlM6oPY76a2WjQ3HRpLl2fY",
    "21x9uOhJFzEngd9SEfKLwfFUolxVqDqdypTgywGWnpklni6x",
    "EE29kM8e0zyElK8f50FOmUh47u6u4UtuBB64CiZcTa4wajlD",
    "OUUyLnG5GbzpWuWecwSp55VSBtFKugSYKhNOcsaQ2m1c1EMx",
    "bDaEaZd5KyyQjpR5CUeYVAd7G9qFSEn87PuZrh9um46la1t3",
    "nkZt5StpNpO88plWonsMMlfD4xdG2KRFZfkp0EQxjjf7NA1P",
    "RL7Fjl6fERHJ2Tp0HTqaa5FE0e02QQKvvy629nAVC3FUu7mE",
    "8cX5Niz0PzOow1yNRD5G6WP3132GF3c55PnCpsJ9iZZxF4xD",
    "xcBqcX3TpGyq4x4HdegzpYObPG1foTKHtoJBy6sV2OrmmPpV",
    "1kRDzkKuP3I93dWVsBRI2udEyv4TuzZwlsfJftIkHSJtph7b",
    "85MV6iNruekdSmDILmCl2aOS3trqBfsdWn8HMoTyzzXB0AsB",
    "07JIour3vyXQ8sYnKZlPN2zJTso1K1dF1TSqhSfHcPJm8yyf",
    "G1jYR9P8Cg0qLJosfgmAYCf3RCRGQP7hUCJRm9PNutGn8Phg",
    "BHA3FfRK4fteQW9v8kLSOez28thcRndMp9epwPGM72mJNaeI",
    "OiXwFwFX21qVxt6qgni4ppbPZH8wo3sbkP9mJ0pPlw6IJ0tp",
    "Uzcy7sLx62MjkJHGmzCLsPMydEwB3gsrhaK2cBINkOS2IuON",
    "z1nnsrG7ZElNDyxyuBShtWVHb7e3cpIz8HSfH1ewodnYjasW",
    "MjuocPF9lpbkImqi85l9XO4et5nvoGLT2a4hRsKb6Ku7dRvj",
    "4RFeH9FGD5hhZPNEGlnUqbkVuQ9JPlRovYHUXp4wu1CPQFUJ",
    "rymuRj91EcozqKI25PE6R9s9xVx4Uvg778LoUoUF4lb2Hn0f",
    "KDqHXMw6RUbAfe2U7s03edYSHQkrOvjA6NSf1mrQEcFLwk4r",
    "AsEl2GfhqGD4jXfFkuHrsTr0YAbeUx5G93EjhDmnyslkJlSu",
    "DpH0Od367J0R3XSpeHY4VEV6AsrQGXUh5BIDglCSmXHO00wz",
    "2M9ZqPLtUm4GpsuKHy5Bw40yV8XJakx5WBv1UyIC5EOt9URk",
    "ds0gCrUUX7u840LnBdtuPu6IMBEnF3G2Hjq5rV4wd0snqfjg",
    nullptr,
  };

  test_helper_zrythm_init ();

  const char * str;
  int test_times = 40;
  gint64 start = g_get_monotonic_time ();
  for (int j = 0; j < test_times; j++)
    {
      for (size_t i = 0;
           (str = test_str[i]) != NULL;
           i++)
        {
          (void)
          string_is_equal (
            str, "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa");
        }
    }
  gint64 end = g_get_monotonic_time ();
  gint64 orig_time = end - start;
  z_info ("orig: {} us", orig_time);

  start = g_get_monotonic_time ();
  for (int j = 0; j < test_times; j++)
    {
      for (size_t i = 0;
           (str = test_str[i]) != NULL;
           i++)
        {
          (void)
          string_is_equal_fast (
            str, "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa");
        }
    }
  end = g_get_monotonic_time ();
  gint64 fast_time = end - start;
  z_info ("fast: {} us", fast_time);

  REQUIRE_LT (fast_time, orig_time);

  test_helper_zrythm_cleanup ();
#endif
}

TEST_CASE ("string_get_regex_group")
{
  const char * test_str = "Hello, World! 123";

  auto result = string_get_regex_group (test_str, R"((\w+),\s(\w+)!)", 1);
  REQUIRE_EQ (result, "Hello");

  result = string_get_regex_group (test_str, R"((\w+),\s(\w+)!)", 2);
  REQUIRE_EQ (result, "World");

  result = string_get_regex_group (test_str, R"((\d+))", 1);
  REQUIRE_EQ (result, "123");

  result = string_get_regex_group (test_str, R"(nonexistent)", 1);
  REQUIRE_EMPTY (result);
}

TEST_CASE ("string_get_regex_group_as_int")
{
  const char * test_str = "Age: 30 Years";

  int result = string_get_regex_group_as_int (test_str, R"(Age:\s(\d+))", 1, -1);
  REQUIRE_EQ (result, 30);

  result =
    string_get_regex_group_as_int (test_str, R"(NonExistent:\s(\d+))", 1, -1);
  REQUIRE_EQ (result, -1);
}

TEST_CASE ("string_expand_env_vars")
{
  setenv ("TEST_VAR", "Hello", 1);
  setenv ("ANOTHER_VAR", "World", 1);

  std::string input = "This is a ${TEST_VAR} ${ANOTHER_VAR} test";
  std::string result = string_expand_env_vars (input);
  REQUIRE_EQ (result, "This is a Hello World test");

  input = "No variables here";
  result = string_expand_env_vars (input);
  REQUIRE_EQ (result, input);

  input = "${NONEXISTENT_VAR}";
  result = string_expand_env_vars (input);
  REQUIRE_EMPTY (result);

  unsetenv ("TEST_VAR");
  unsetenv ("ANOTHER_VAR");
}

TEST_CASE ("string_join")
{
  std::vector<std::string> strings = { "Hello", "World", "Test" };

  std::string result = string_join (strings, ", ");
  REQUIRE (result == "Hello, World, Test");

  result = string_join (strings, " - ");
  REQUIRE (result == "Hello - World - Test");

  std::vector<std::string> empty_vec;
  result = string_join (empty_vec, ", ");
  REQUIRE (result.empty ());
}

TEST_SUITE_END;