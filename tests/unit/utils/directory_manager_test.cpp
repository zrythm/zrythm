#include "utils/directory_manager.h"
#include "utils/gtest_wrapper.h"

TEST (DirectoryManagerTest, TestingDirCreation)
{
  TestingDirectoryManager mgr;
  auto                    testing_dir = mgr.get_testing_dir ();
  EXPECT_FALSE (testing_dir.empty ());
  EXPECT_TRUE (fs::exists (testing_dir));
}

TEST (DirectoryManagerTest, TestingDirRemoval)
{
  fs::path dir_path;
  {
    TestingDirectoryManager mgr;
    dir_path = mgr.get_testing_dir ();
    EXPECT_TRUE (fs::exists (dir_path));
  }
  // Directory should be removed when mgr goes out of scope
  EXPECT_FALSE (fs::exists (dir_path));
}

TEST (DirectoryManagerTest, TestingUserDir)
{
  TestingDirectoryManager mgr;
  auto                    user_dir = mgr.get_user_dir (false);
  EXPECT_EQ (user_dir, mgr.get_testing_dir ());
}

TEST (DirectoryManagerTest, TestingGetDir)
{
  TestingDirectoryManager mgr;
  auto                    testing_dir = mgr.get_testing_dir ();

  // Test system dirs
  EXPECT_EQ (
    mgr.get_dir (DirectoryManager::DirectoryType::SYSTEM_PREFIX), ZRYTHM_PREFIX);
  EXPECT_EQ (
    mgr.get_dir (DirectoryManager::DirectoryType::SYSTEM_BINDIR),
    fs::path (ZRYTHM_PREFIX) / BINDIR_NAME);
  EXPECT_EQ (
    mgr.get_dir (DirectoryManager::DirectoryType::SYSTEM_PARENT_DATADIR),
    fs::path (ZRYTHM_PREFIX) / DATADIR_NAME);

  // Test user dirs
  EXPECT_EQ (
    mgr.get_dir (DirectoryManager::DirectoryType::USER_TOP), testing_dir);
  EXPECT_EQ (
    mgr.get_dir (DirectoryManager::DirectoryType::USER_PROJECTS),
    testing_dir / "projects");
  EXPECT_EQ (
    mgr.get_dir (DirectoryManager::DirectoryType::USER_TEMPLATES),
    testing_dir / "templates");
  EXPECT_EQ (
    mgr.get_dir (DirectoryManager::DirectoryType::USER_LOG),
    testing_dir / "log");
  EXPECT_EQ (
    mgr.get_dir (DirectoryManager::DirectoryType::USER_BACKTRACE),
    testing_dir / "backtraces");
}
